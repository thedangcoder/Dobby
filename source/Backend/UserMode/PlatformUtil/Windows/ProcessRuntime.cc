#include "PlatformUtil/ProcessRuntime.h"
#include "dobby/platform_mutex.h"

#include <vector>

#include <windows.h>

#define LINE_MAX 2048

// Cache configuration
static const int CACHE_TTL_MS = 100; // Cache valid for 100ms

// Get current time in milliseconds
static uint64_t get_time_ms() {
  return GetTickCount64();
}

static bool memory_region_comparator(MemRegion a, MemRegion b) {
  return (a.start() < b.start());
}

// https://gist.github.com/jedwardsol/9d4fe1fd806043a5767affbd200088ca

static DobbyMutex g_memory_layout_mutex;
static stl::vector<MemRegion> ProcessMemoryLayout;
static uint64_t g_memory_layout_cache_time = 0;
static bool g_memory_layout_cache_valid = false;

void ProcessRuntime::invalidateMemoryLayoutCache() {
  DobbyLockGuard lock(g_memory_layout_mutex);
  g_memory_layout_cache_valid = false;
}

const stl::vector<MemRegion> &ProcessRuntime::getMemoryLayout(bool force_refresh) {
  DobbyLockGuard lock(g_memory_layout_mutex);

  // Check if cache is still valid
  uint64_t now = get_time_ms();
  if (!force_refresh && g_memory_layout_cache_valid && (now - g_memory_layout_cache_time) < CACHE_TTL_MS) {
    return ProcessMemoryLayout;
  }

  ProcessMemoryLayout.clear();

  char *address{nullptr};
  MEMORY_BASIC_INFORMATION region;

  while (VirtualQuery(address, &region, sizeof(region))) {
    address += region.RegionSize;
    if (!(region.State & (MEM_COMMIT | MEM_RESERVE))) {
      continue;
    }

    MemoryPermission permission = MemoryPermission::kNoAccess;
    auto mask = PAGE_GUARD | PAGE_NOCACHE | PAGE_WRITECOMBINE;
    switch (region.Protect & ~mask) {
    case PAGE_NOACCESS:
    case PAGE_READONLY:
      break;

    case PAGE_EXECUTE:
    case PAGE_EXECUTE_READ:
      permission = MemoryPermission::kReadExecute;
      break;

    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
      permission = MemoryPermission::kReadWrite;
      break;

    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
      permission = MemoryPermission::kReadWriteExecute;
      break;
    }

    ProcessMemoryLayout.push_back(MemRegion((addr_t)region.BaseAddress, region.RegionSize, permission));
  }

  // Update cache timestamp
  g_memory_layout_cache_time = get_time_ms();
  g_memory_layout_cache_valid = true;

  return ProcessMemoryLayout;
}

static DobbyMutex g_module_map_mutex;
static stl::vector<RuntimeModule> ProcessModuleMap;
static uint64_t g_module_map_cache_time = 0;
static bool g_module_map_cache_valid = false;

void ProcessRuntime::invalidateModuleMapCache() {
  DobbyLockGuard lock(g_module_map_mutex);
  g_module_map_cache_valid = false;
}

const stl::vector<RuntimeModule> &ProcessRuntime::getModuleMap(bool force_refresh) {
  DobbyLockGuard lock(g_module_map_mutex);

  // Check if cache is still valid
  uint64_t now = get_time_ms();
  if (!force_refresh && g_module_map_cache_valid && (now - g_module_map_cache_time) < CACHE_TTL_MS) {
    return ProcessModuleMap;
  }

  ProcessModuleMap.clear();

  // Update cache timestamp
  g_module_map_cache_time = get_time_ms();
  g_module_map_cache_valid = true;

  return ProcessModuleMap;
}

RuntimeModule ProcessRuntime::getModule(const char *name) {
  const stl::vector<RuntimeModule> &modules = getModuleMap();
  for (auto module : modules) {
    if (strstr(module.path, name) != 0) {
      return module;
    }
  }
  return RuntimeModule{0};
}