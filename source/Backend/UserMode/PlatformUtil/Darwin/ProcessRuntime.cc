#include "dobby/dobby_internal.h"
#include "dobby/platform_mutex.h"

#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <mach/mach_init.h>
#include <mach-o/dyld.h>
#include <mach-o/getsect.h>
#include <mach-o/dyld_images.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>

#include <AvailabilityMacros.h>

#include <libkern/OSAtomic.h>
#include <mach/mach.h>
#include <mach/semaphore.h>
#include <mach/task.h>
#include <mach/vm_statistics.h>

#include "UnifiedInterface/platform-darwin/mach_vm.h"
#include "PlatformUtil/ProcessRuntime.h"

// Cache configuration
static const int CACHE_TTL_MS = 100; // Cache valid for 100ms

// Get current time in milliseconds
static uint64_t get_time_ms() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

static bool memory_region_comparator(MemRegion a, MemRegion b) {
  return (a.addr() < b.addr());
}

static DobbyMutex g_regions_mutex;
static stl::vector<MemRegion> *regions = nullptr;
static uint64_t g_regions_cache_time = 0;
static bool g_regions_cache_valid = false;

void ProcessRuntime::invalidateMemoryLayoutCache() {
  DobbyLockGuard lock(g_regions_mutex);
  g_regions_cache_valid = false;
}

const stl::vector<MemRegion> &ProcessRuntime::getMemoryLayout(bool force_refresh) {
  DobbyLockGuard lock(g_regions_mutex);
  if (regions == nullptr) {
    regions = new stl::vector<MemRegion>();
  }

  // Check if cache is still valid
  uint64_t now = get_time_ms();
  if (!force_refresh && g_regions_cache_valid && (now - g_regions_cache_time) < CACHE_TTL_MS) {
    return *regions;
  }

  regions->clear();

  vm_region_submap_info_64 region_submap_info;
  mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;
  mach_vm_address_t addr = 0;
  mach_vm_size_t size = 0;
  natural_t depth = 0;
  while (true) {
    count = VM_REGION_SUBMAP_INFO_COUNT_64;
    kern_return_t kr = mach_vm_region_recurse(mach_task_self(), (mach_vm_address_t *)&addr, (mach_vm_size_t *)&size,
                                              &depth, (vm_region_recurse_info_t)&region_submap_info, &count);
    if (kr != KERN_SUCCESS) {
      if (kr == KERN_INVALID_ADDRESS) {
        break;
      } else {
        break;
      }
    }

    if (region_submap_info.is_submap) {
      depth++;
    } else {
      MemoryPermission perm = kNoAccess;
      auto prot = region_submap_info.protection;
      if (prot & VM_PROT_READ) {
        perm = (MemoryPermission)(perm | kRead);
      }
      if (prot & VM_PROT_WRITE) {
        perm = (MemoryPermission)(perm | kWrite);
      }
      if (prot & VM_PROT_EXECUTE) {
        perm = (MemoryPermission)(perm | kExecute);
      }
      // INFO_LOG("%p --- %p --- %p --- %d", addr, addr + size, size, region_submap_info.protection);

      MemRegion region = MemRegion(addr, size, perm);
      regions->push_back(region);
      addr += size;
    }
  }

  // std::sort(ProcessMemoryLayout.begin(), ProcessMemoryLayout.end(), memory_region_comparator);

  // Update cache timestamp
  g_regions_cache_time = get_time_ms();
  g_regions_cache_valid = true;

  return *regions;
}

static DobbyMutex g_modules_mutex;
static stl::vector<RuntimeModule> *modules = nullptr;
static uint64_t g_modules_cache_time = 0;
static bool g_modules_cache_valid = false;

void ProcessRuntime::invalidateModuleMapCache() {
  DobbyLockGuard lock(g_modules_mutex);
  g_modules_cache_valid = false;
}

const stl::vector<RuntimeModule> &ProcessRuntime::getModuleMap(bool force_refresh) {
  DobbyLockGuard lock(g_modules_mutex);
  if (modules == nullptr) {
    modules = new stl::vector<RuntimeModule>();
  }

  // Check if cache is still valid
  uint64_t now = get_time_ms();
  if (!force_refresh && g_modules_cache_valid && (now - g_modules_cache_time) < CACHE_TTL_MS) {
    return *modules;
  }

  modules->clear();

  kern_return_t kr;
  task_dyld_info_data_t task_dyld_info;
  mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
  kr = task_info(mach_task_self_, TASK_DYLD_INFO, (task_info_t)&task_dyld_info, &count);
  if (kr != KERN_SUCCESS) {
    return *modules;
  }

  struct dyld_all_image_infos *infos = (struct dyld_all_image_infos *)task_dyld_info.all_image_info_addr;
  const struct dyld_image_info *infoArray = infos->infoArray;
  uint32_t infoArrayCount = infos->infoArrayCount;

  RuntimeModule module = {0};
  strncpy(module.path, "dummy-placeholder-module", sizeof(module.path) - 1);
  module.base = 0;
  modules->push_back(module);

  strncpy(module.path, infos->dyldPath, sizeof(module.path) - 1);
  module.base = (void *)infos->dyldImageLoadAddress;
  modules->push_back(module);

  for (int i = 0; i < infoArrayCount; ++i) {
    const struct dyld_image_info *info = &infoArray[i];

    {
      strncpy(module.path, info->imageFilePath, sizeof(module.path) - 1);
      module.base = (void *)info->imageLoadAddress;
      modules->push_back(module);
    }
  }

  modules->sort([](const RuntimeModule &a, const RuntimeModule &b) -> int { return a.base < b.base; });

  // Update cache timestamp
  g_modules_cache_time = get_time_ms();
  g_modules_cache_valid = true;

  return *modules;
}

RuntimeModule ProcessRuntime::getModule(const char *name) {
  auto modules = getModuleMap();
  for (auto module : modules) {
    if (strstr(module.path, name) != 0) {
      return module;
    }
  }
  return RuntimeModule{0};
}
