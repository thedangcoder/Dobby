#include <stdio.h>

#include <windows.h>

#include "logging/logging.h"
#include "logging/check_logging.h"
#include "PlatformUnifiedInterface/platform.h"

static int GetProtectionFromMemoryPermission(MemoryPermission access) {
  int prot = PAGE_NOACCESS;
  if (access & MemoryPermission::kExecute) {
    if (access & MemoryPermission::kWrite) {
      prot = PAGE_EXECUTE_READWRITE;
    } else if (access & MemoryPermission::kRead) {
      prot = PAGE_EXECUTE_READ;
    } else {
      prot = PAGE_EXECUTE;
    }
  } else {
    if (access & MemoryPermission::kWrite) {
      prot = PAGE_READWRITE;
    } else if (access & MemoryPermission::kRead) {
      prot = PAGE_READONLY;
    }
  }
  return prot;
}

static int AllocPageSize() {
  static int lastRet = -1;
  if (lastRet == -1) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    lastRet = si.dwAllocationGranularity; // should be used with VirtualAlloc(MEM_RESERVE)
  }
  return lastRet;
}

int OSMemory::PageSize() {
  static int lastRet = -1;
  if (lastRet == -1) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    lastRet = si.dwPageSize;
  }
  return lastRet;
}

void *OSMemory::Allocate(size_t size, MemoryPermission access) {
  return OSMemory::Allocate(size, access, nullptr);
}

void *OSMemory::Allocate(size_t size, MemoryPermission access, void *fixed_address) {
  int prot = GetProtectionFromMemoryPermission(access);

  void *result = VirtualAlloc(fixed_address, size, MEM_COMMIT | MEM_RESERVE, prot);
  if (result == nullptr)
    return nullptr;

  return result;
}

bool OSMemory::Free(void *address, size_t size) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % PageSize());

  return VirtualFree(address, 0, MEM_RELEASE) != 0;
}

bool OSMemory::Release(void *address, size_t size) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % PageSize());

  return OSMemory::Free(address, size);
}

bool OSMemory::SetPermission(void *address, size_t size, MemoryPermission access) {
  DCHECK_EQ(0, reinterpret_cast<uintptr_t>(address) % PageSize());

  int prot = GetProtectionFromMemoryPermission(access);

  DWORD oldProtect;
  return VirtualProtect(address, size, prot, &oldProtect) != 0;
}

// =====

void OSPrint::Print(const char *format, ...) {
  va_list args;
  va_start(args, format);
  VPrint(format, args);
  va_end(args);
}

void OSPrint::VPrint(const char *format, va_list args) {
  vprintf(format, args);
}
