#include "dobby/dobby_internal.h"

#include <windows.h>

using namespace zz;

PUBLIC int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t buffer_size) {
  if (!address || !buffer || buffer_size == 0) {
    ERROR_LOG("DobbyCodePatch: invalid parameters (address=%p, buffer=%p, size=%u)", address, buffer, buffer_size);
    DOBBY_RETURN_ERROR(kDobbyErrorInvalidArgument);
  }

  // Check for integer overflow: address + buffer_size
  if ((uintptr_t)address > UINTPTR_MAX - buffer_size) {
    ERROR_LOG("DobbyCodePatch: address + size overflow (address=%p, size=%u)", address, buffer_size);
    DOBBY_RETURN_ERROR(kDobbyErrorInvalidArgument);
  }

  DWORD oldProtect;
  DWORD oldProtect2;

  // Get page size
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  int page_size = si.dwPageSize;

  void *addressPageAlign = (void *)ALIGN_FLOOR(address, page_size);
  void *endAddressPageAlign = (void *)ALIGN_FLOOR((uintptr_t)address + buffer_size, page_size);

  // Calculate total size to protect (may span multiple pages)
  size_t protect_size = (uintptr_t)endAddressPageAlign - (uintptr_t)addressPageAlign + page_size;

  if (!VirtualProtect(addressPageAlign, protect_size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
    DWORD err = GetLastError();
    ERROR_LOG("DobbyCodePatch: VirtualProtect RWX failed for %p (size=%zu): error %lu", addressPageAlign, protect_size, err);
    DOBBY_RETURN_ERROR(kDobbyErrorMemoryProtection);
  }

  memcpy(address, buffer, buffer_size);

  if (!VirtualProtect(addressPageAlign, protect_size, oldProtect, &oldProtect2)) {
    DWORD err = GetLastError();
    ERROR_LOG("DobbyCodePatch: VirtualProtect restore failed for %p: error %lu", addressPageAlign, err);
    DOBBY_RETURN_ERROR(kDobbyErrorMemoryProtection);
  }

  // Flush instruction cache
  FlushInstructionCache(GetCurrentProcess(), address, buffer_size);

  DobbySetLastError(kDobbySuccess);
  return kDobbySuccess;
}
