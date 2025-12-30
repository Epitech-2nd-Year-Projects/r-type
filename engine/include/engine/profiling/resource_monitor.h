#ifndef ENGINE_PROFILING_RESOURCE_MONITOR_H_
#define ENGINE_PROFILING_RESOURCE_MONITOR_H_

#include <chrono>
#include <cstdint>
#include <memory>

namespace engine::profiling {

struct ResourceStats {
  float cpu_usage_percent{0.0f};
  float memory_usage_mb{0.0f};
  std::uint64_t memory_usage_bytes{0};
};

class ResourceMonitorImpl;

class ResourceMonitor {
 public:
  ResourceMonitor();
  ~ResourceMonitor();

  ResourceMonitor(const ResourceMonitor&) = delete;
  ResourceMonitor& operator=(const ResourceMonitor&) = delete;
  ResourceMonitor(ResourceMonitor&&) noexcept;
  ResourceMonitor& operator=(ResourceMonitor&&) noexcept;

  void Update();
  void SetUpdateInterval(std::chrono::milliseconds interval);

  ResourceStats GetStats() const;
  float cpu_usage_percent() const;
  float memory_usage_mb() const;

 private:
  std::unique_ptr<ResourceMonitorImpl> impl_;
  std::chrono::steady_clock::time_point last_update_;
  std::chrono::milliseconds update_interval_{500};
  ResourceStats cached_stats_;
};

}  // namespace engine::profiling

#endif  // ENGINE_PROFILING_RESOURCE_MONITOR_H_
