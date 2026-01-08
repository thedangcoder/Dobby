#pragma once

#include "MemoryAllocator/MemoryAllocator.h"
#include "PlatformUnifiedInterface/platform.h"

struct RuntimeModule {
  void *base;
  char path[1024];
};

#define MEM_PERM_R 0x1
#define MEM_PERM_W 0x2
#define MEM_PERM_X 0x4
struct MemRegion : MemRange {
  int perm;

  MemRegion(addr_t addr, size_t size, int perm) : MemRange(addr, size), perm(perm) {
  }
};

class ProcessRuntime {
public:
  // Get memory layout (cached for performance, ~100ms TTL)
  // @param force_refresh If true, ignores cache and re-reads from system
  static const stl::vector<MemRegion> &getMemoryLayout(bool force_refresh = false);

  // Invalidate the memory layout cache
  static void invalidateMemoryLayoutCache();

  // Get module map (cached for performance, ~100ms TTL)
  // @param force_refresh If true, ignores cache and re-reads from system
  static const stl::vector<RuntimeModule> &getModuleMap(bool force_refresh = false);

  // Invalidate the module map cache
  static void invalidateModuleMapCache();

  static RuntimeModule getModule(const char *name);
};