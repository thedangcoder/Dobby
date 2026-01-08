#pragma once

#include "common/linear_allocator.h"
#include "PlatformUnifiedInterface/platform.h"
#include "dobby/platform_mutex.h"
#include <new>

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

struct MemRange {
  addr_t start_;
  size_t size;

  MemRange() : start_(0), size(0) {
  }

  MemRange(addr_t start, size_t size) : start_(start), size(size) {
  }

  addr_t start() const {
    return start_;
  }

  addr_t addr() const {
    return start_;
  }

  addr_t end() const {
    return start_ + size;
  }

  void resize(size_t in_size) {
    size = in_size;
  }

  void reset(addr_t in_start, size_t in_size) {
    start_ = in_start;
    size = in_size;
  }

  MemRange intersect(const MemRange &other) const {
    auto start = max(this->addr(), other.addr());
    auto end = min(this->end(), other.end());
    if (start < end)
      return MemRange(start, end - start);
    else
      return MemRange{};
  }
};

struct MemBlock : MemRange {
  MemBlock() : MemRange() {
  }

  MemBlock(addr_t start, size_t size) : MemRange(start, size) {
  }
};

using CodeMemBlock = MemBlock;
using DataMemBlock = MemBlock;

// Page info structure to track allocator and its backing memory
struct PageAllocatorInfo {
  linear_allocator_t *allocator;
  void *page_base;
  size_t page_size;
  bool is_exec;

  PageAllocatorInfo(linear_allocator_t *alloc, void *base, size_t size, bool exec)
      : allocator(alloc), page_base(base), page_size(size), is_exec(exec) {}

  bool contains(addr_t addr) const {
    return addr >= (addr_t)page_base && addr < (addr_t)page_base + page_size;
  }
};

struct MemoryAllocator {
  mutable DobbyMutex mutex;
  stl::vector<PageAllocatorInfo> page_allocators;

  inline static MemoryAllocator *Shared();

  // Find the allocator that contains the given address
  PageAllocatorInfo *findAllocatorForAddress(addr_t addr) {
    for (auto &info : page_allocators) {
      if (info.contains(addr)) {
        return &info;
      }
    }
    return nullptr;
  }

  // Free a previously allocated memory block
  // Memory is marked as free and can be reused by subsequent allocations
  void freeMemBlock(MemBlock block) {
    if (block.addr() == 0 || block.size == 0) {
      return;
    }

    DobbyLockGuard lock(mutex);

    auto *info = findAllocatorForAddress(block.addr());
    if (!info || !info->allocator) {
      ERROR_LOG("freeMemBlock: address %p not found in any allocator", (void *)block.addr());
      return;
    }

    info->allocator->free((uint8_t *)block.addr());
  }

  MemBlock allocMemBlock(size_t in_size, bool is_exec = true) {
    DobbyLockGuard lock(mutex);

    if (in_size == 0) {
      ERROR_LOG("allocMemBlock: invalid size 0");
      return {};
    }

    if (in_size > OSMemory::PageSize()) {
      ERROR_LOG("allocMemBlock: size too large: %zu (max: %zu)", in_size, OSMemory::PageSize());
      return {};
    }

    uint8_t *result = nullptr;

    // Try existing allocators of the same type first
    for (auto &info : page_allocators) {
      if (info.is_exec == is_exec && info.allocator) {
        result = info.allocator->alloc((uint32_t)in_size);
        if (result)
          break;
      }
    }

    if (!result) {
      auto page = OSMemory::Allocate(OSMemory::PageSize(), kNoAccess);
      if (!page) {
        ERROR_LOG("allocMemBlock: OSMemory::Allocate failed for %s block (size: %zu)",
                  is_exec ? "exec" : "data", in_size);
        return {};
      }

      // For exec blocks, we need RWX initially to allow writing code.
      // The page will remain RWX (required for dynamic code generation).
      // For data blocks, RW is sufficient.
      if (!OSMemory::SetPermission(page, OSMemory::PageSize(), is_exec ? kReadWriteExecute : kReadWrite)) {
        ERROR_LOG("allocMemBlock: OSMemory::SetPermission failed for %s block", is_exec ? "exec" : "data");
        OSMemory::Free(page, OSMemory::PageSize());
        return {};
      }

      auto page_allocator = new (std::nothrow) linear_allocator_t((uint8_t *)page, (uint32_t)OSMemory::PageSize());
      if (!page_allocator) {
        ERROR_LOG("allocMemBlock: failed to create page allocator");
        OSMemory::Free(page, OSMemory::PageSize());
        return {};
      }

      page_allocators.push_back(PageAllocatorInfo(page_allocator, page, OSMemory::PageSize(), is_exec));

      result = page_allocator->alloc((uint32_t)in_size);
    }

    if (!result) {
      ERROR_LOG("allocMemBlock: allocation failed for %s block (size: %zu)", is_exec ? "exec" : "data", in_size);
      return {};
    }

    return MemBlock((addr_t)result, in_size);
  }

  MemBlock allocExecBlock(size_t size) {
    return allocMemBlock(size, true);
  }

  MemBlock allocDataBlock(size_t size) {
    return allocMemBlock(size, false);
  }
};

inline static MemoryAllocator gMemoryAllocator;
MemoryAllocator *MemoryAllocator::Shared() {
  return &gMemoryAllocator;
}

#undef min
#undef max