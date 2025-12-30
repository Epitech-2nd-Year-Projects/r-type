#include "engine/profiling/resource_monitor.h"

#include <cstdint>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <psapi.h>
#include <windows.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>

#include <fstream>
#include <string>
#endif

namespace engine::profiling {

class ResourceMonitorImpl {
 public:
  ResourceMonitorImpl() { InitCpuTiming(); }

  ResourceStats Query() {
    ResourceStats stats;
    stats.cpu_usage_percent = QueryCpuUsage();
    stats.memory_usage_bytes = QueryMemoryBytes();
    stats.memory_usage_mb =
        static_cast<float>(stats.memory_usage_bytes) / (1024.0f * 1024.0f);
    return stats;
  }

 private:
  void InitCpuTiming() {
#if defined(_WIN32)
    FILETIME creation, exit, kernel, user;
    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel,
                        &user)) {
      last_kernel_time_ = FileTimeToUint64(kernel);
      last_user_time_ = FileTimeToUint64(user);
    }
    FILETIME idle, kern, usr;
    if (GetSystemTimes(&idle, &kern, &usr)) {
      last_system_idle_ = FileTimeToUint64(idle);
      last_system_kernel_ = FileTimeToUint64(kern);
      last_system_user_ = FileTimeToUint64(usr);
    }
#elif defined(__linux__)
    std::ifstream stat("/proc/self/stat");
    if (stat) {
      std::string line;
      std::getline(stat, line);
      ParseProcStat(line);
    }
    ReadSystemCpuTime();
#endif
  }

  float QueryCpuUsage() {
#if defined(_WIN32)
    FILETIME creation, exit, kernel, user;
    if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel,
                         &user)) {
      return 0.0f;
    }
    std::uint64_t kernel_now = FileTimeToUint64(kernel);
    std::uint64_t user_now = FileTimeToUint64(user);
    std::uint64_t proc_delta =
        (kernel_now - last_kernel_time_) + (user_now - last_user_time_);

    FILETIME idle, kern, usr;
    if (!GetSystemTimes(&idle, &kern, &usr)) {
      return 0.0f;
    }
    std::uint64_t idle_now = FileTimeToUint64(idle);
    std::uint64_t kern_now = FileTimeToUint64(kern);
    std::uint64_t usr_now = FileTimeToUint64(usr);
    std::uint64_t sys_delta = (kern_now - last_system_kernel_) +
                              (usr_now - last_system_user_) -
                              (idle_now - last_system_idle_);

    last_kernel_time_ = kernel_now;
    last_user_time_ = user_now;
    last_system_idle_ = idle_now;
    last_system_kernel_ = kern_now;
    last_system_user_ = usr_now;

    if (sys_delta == 0) return 0.0f;
    return static_cast<float>(proc_delta) / static_cast<float>(sys_delta) *
           100.0f;
#elif defined(__linux__)
    std::ifstream stat("/proc/self/stat");
    if (!stat) return 0.0f;

    std::string line;
    std::getline(stat, line);
    std::uint64_t utime = 0, stime = 0;
    if (!ParseProcStatTimes(line, utime, stime)) {
      return 0.0f;
    }

    std::uint64_t proc_time = utime + stime;
    std::uint64_t proc_delta = proc_time - last_proc_time_;
    last_proc_time_ = proc_time;

    std::uint64_t total = 0, idle = 0;
    ReadSystemCpuTime(&total, &idle);
    std::uint64_t sys_delta = total - last_system_total_;
    last_system_total_ = total;

    if (sys_delta == 0) return 0.0f;
    return static_cast<float>(proc_delta) / static_cast<float>(sys_delta) *
           100.0f;
#else
    return 0.0f;
#endif
  }

  std::uint64_t QueryMemoryBytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
      return pmc.WorkingSetSize;
    }
    return 0;
#elif defined(__linux__)
    std::ifstream status("/proc/self/status");
    if (!status) return 0;

    std::string line;
    while (std::getline(status, line)) {
      if (line.compare(0, 6, "VmRSS:") == 0) {
        std::uint64_t kb = 0;
        std::sscanf(line.c_str(), "VmRSS: %lu", &kb);
        return kb * 1024;
      }
    }
    return 0;
#else
    return 0;
#endif
  }

#if defined(_WIN32)
  static std::uint64_t FileTimeToUint64(const FILETIME& ft) {
    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;
    return li.QuadPart;
  }

  std::uint64_t last_kernel_time_{0};
  std::uint64_t last_user_time_{0};
  std::uint64_t last_system_idle_{0};
  std::uint64_t last_system_kernel_{0};
  std::uint64_t last_system_user_{0};
#elif defined(__linux__)
  // In /proc/self/stat, after the command name (field 2 in parentheses),
  // utime and stime are fields 14 and 15. We skip 11 fields after ')' to reach
  // them.
  static constexpr int kProcStatFieldsBeforeUtime = 11;

  void ParseProcStat(const std::string& line) {
    std::uint64_t utime = 0, stime = 0;
    if (ParseProcStatTimes(line, utime, stime)) {
      last_proc_time_ = utime + stime;
    }
  }

  bool ParseProcStatTimes(const std::string& line, std::uint64_t& utime,
                          std::uint64_t& stime) {
    std::size_t pos = line.rfind(')');
    if (pos == std::string::npos) return false;

    const char* p = line.c_str() + pos + 2;
    int field = 0;
    while (*p && field < kProcStatFieldsBeforeUtime) {
      while (*p == ' ') ++p;
      while (*p && *p != ' ') ++p;
      ++field;
    }
    return std::sscanf(p, "%lu %lu", &utime, &stime) == 2;
  }

  void ReadSystemCpuTime(std::uint64_t* total = nullptr,
                         std::uint64_t* idle_out = nullptr) {
    std::ifstream stat("/proc/stat");
    if (!stat) return;

    std::string line;
    std::getline(stat, line);
    if (line.compare(0, 3, "cpu") != 0) return;

    std::uint64_t user, nice, system, idle, iowait, irq, softirq;
    std::sscanf(line.c_str() + 4, "%lu %lu %lu %lu %lu %lu %lu", &user, &nice,
                &system, &idle, &iowait, &irq, &softirq);

    std::uint64_t t = user + nice + system + idle + iowait + irq + softirq;
    if (total) *total = t;
    if (idle_out) *idle_out = idle + iowait;
    if (!total) last_system_total_ = t;
  }

  std::uint64_t last_proc_time_{0};
  std::uint64_t last_system_total_{0};
#endif
};

ResourceMonitor::ResourceMonitor()
    : impl_(std::make_unique<ResourceMonitorImpl>()),
      last_update_(std::chrono::steady_clock::now()) {}

ResourceMonitor::~ResourceMonitor() = default;

ResourceMonitor::ResourceMonitor(ResourceMonitor&&) noexcept = default;
ResourceMonitor& ResourceMonitor::operator=(ResourceMonitor&&) noexcept =
    default;

void ResourceMonitor::Update() {
  auto now = std::chrono::steady_clock::now();
  if (now - last_update_ < update_interval_) {
    return;
  }
  last_update_ = now;
  cached_stats_ = impl_->Query();
}

void ResourceMonitor::SetUpdateInterval(std::chrono::milliseconds interval) {
  update_interval_ = interval;
}

ResourceStats ResourceMonitor::GetStats() const { return cached_stats_; }

float ResourceMonitor::cpu_usage_percent() const {
  return cached_stats_.cpu_usage_percent;
}

float ResourceMonitor::memory_usage_mb() const {
  return cached_stats_.memory_usage_mb;
}

}  // namespace engine::profiling
