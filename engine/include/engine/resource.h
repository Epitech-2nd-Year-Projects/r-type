#ifndef ENGINE_ENGINE_RESOURCE_H_
#define ENGINE_ENGINE_RESOURCE_H_

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace engine::resource {

/**
 * @brief Abstract loader interface for a given resource type.
 *
 * Implementations perform the actual loading (e.g. from disk, network,
 * memory). The ResourceManager relies on this interface to create resources
 * on-demand.
 */
template <typename Resource>
class IResourceLoader {
 public:
  virtual ~IResourceLoader() = default;

  /**
   * @brief Returns true if the loader can handle the provided identifier.
   */
  virtual bool CanLoad(const std::string& resource_id) const = 0;

  /**
   * @brief Load a resource identified by resource_id.
   * @return Shared pointer to the loaded resource.
   *
   * @throws std::runtime_error on loading failures.
   */
  virtual std::shared_ptr<Resource> Load(const std::string& resource_id) = 0;
};

/**
 * @brief Resource cache statistics.
 */
struct ResourceStatistics {
  /// Number of entries currently tracked in the cache.
  std::size_t cached_entries = 0;
  /// Number of live resources (weak_ptr lock succeeds).
  std::size_t live_resources = 0;
  /// Total number of load operations performed.
  std::size_t total_loads = 0;
};

/**
 * @brief Generic resource manager with caching and lifetime tracking.
 *
 * @details
 * - Resources are identified by string IDs.
 * - Resources are loaded lazily through an IResourceLoader<T>.
 * - Lifetime is reference-counted through shared_ptr.
 * - Cached entries are stored as weak_ptr to avoid preventing destruction.
 *
 * Thread-safety: no internal synchronization; callers must guard access when
 * used across threads.
 */
template <typename Resource>
class ResourceManager {
 public:
  using ResourcePtr = std::shared_ptr<Resource>;

  explicit ResourceManager(
      std::shared_ptr<IResourceLoader<Resource>> loader) noexcept(false)
      : loader_(std::move(loader)) {
    if (loader_ == nullptr) {
      throw std::invalid_argument("ResourceManager requires a loader");
    }
  }

  /**
   * @brief Acquire a resource by identifier.
   *
   * @details
   * - Returns a shared_ptr that keeps the resource alive.
   * - If cached and still alive, returns the cached instance.
   * - Otherwise loads via the loader and caches the result.
   */
  ResourcePtr Acquire(const std::string& resource_id) {
    if (resource_id.empty()) {
      throw std::invalid_argument("Resource id cannot be empty");
    }

    auto it = cache_.find(resource_id);
    if (it != cache_.end()) {
      auto cached = it->second.weak.lock();
      if (cached) {
        ++it->second.hits;
        return cached;
      }
    }

    if (!loader_->CanLoad(resource_id)) {
      throw std::runtime_error("No loader available for resource id: " +
                               resource_id);
    }

    auto loaded = loader_->Load(resource_id);
    if (loaded == nullptr) {
      throw std::runtime_error("Loader returned null resource for id: " +
                               resource_id);
    }

    ResourceStatistics& stats = stats_;
    ++stats.total_loads;

    CacheEntry entry{};
    entry.weak = loaded;
    entry.hits = 1;

    if (it == cache_.end()) {
      cache_.emplace(resource_id, std::move(entry));
    } else {
      it->second = std::move(entry);
    }

    return loaded;
  }

  /**
   * @brief Remove expired cache entries.
   *
   * Does not affect live resources already held by callers.
   */
  void PruneExpired() {
    for (auto it = cache_.begin(); it != cache_.end();) {
      if (it->second.weak.expired()) {
        it = cache_.erase(it);
      } else {
        ++it;
      }
    }
  }

  /**
   * @brief Clear all cached entries, including live weak handles.
   *
   * Live shared_ptr instances held by clients remain valid.
   */
  void Clear() { cache_.clear(); }

  /**
   * @brief Return current cache statistics (computed on demand).
   */
  ResourceStatistics GetStatistics() const {
    ResourceStatistics out = stats_;
    out.cached_entries = cache_.size();
    out.live_resources = 0;
    for (const auto& [_, entry] : cache_) {
      if (!entry.weak.expired()) {
        ++out.live_resources;
      }
    }
    return out;
  }

  /**
   * @brief Returns true if the resource exists in cache and is still alive.
   */
  bool IsLoaded(const std::string& resource_id) const {
    auto it = cache_.find(resource_id);
    if (it == cache_.end()) {
      return false;
    }
    return !it->second.weak.expired();
  }

 private:
  struct CacheEntry {
    std::weak_ptr<Resource> weak;
    std::size_t hits = 0;
  };

  std::shared_ptr<IResourceLoader<Resource>> loader_;
  std::unordered_map<std::string, CacheEntry> cache_;
  ResourceStatistics stats_;
};

}  // namespace engine::resource

#endif /* !ENGINE_ENGINE_RESOURCE_H_ */
