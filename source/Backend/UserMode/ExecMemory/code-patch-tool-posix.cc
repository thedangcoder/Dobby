
#include "dobby/dobby_internal.h"

#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#if !defined(__APPLE__)
PUBLIC int DobbyCodePatch(void *address, uint8_t *buffer, uint32_t buffer_size) {
#if defined(__ANDROID__) || defined(__linux__)
  if (!address || !buffer || buffer_size == 0) {
    ERROR_LOG("DobbyCodePatch: invalid parameters (address=%p, buffer=%p, size=%u)", address, buffer, buffer_size);
    DOBBY_RETURN_ERROR(kDobbyErrorInvalidArgument);
  }

  // Check for integer overflow: address + buffer_size
  if ((uintptr_t)address > UINTPTR_MAX - buffer_size) {
    ERROR_LOG("DobbyCodePatch: address + size overflow (address=%p, size=%u)", address, buffer_size);
    DOBBY_RETURN_ERROR(kDobbyErrorInvalidArgument);
  }

  int page_size = (int)sysconf(_SC_PAGESIZE);
  if (page_size <= 0) {
    ERROR_LOG("DobbyCodePatch: failed to get page size");
    DOBBY_RETURN_ERROR(kDobbyErrorMemoryOperation);
  }

  uintptr_t patch_page = ALIGN_FLOOR(address, page_size);
  uintptr_t patch_end_page = ALIGN_FLOOR((uintptr_t)address + buffer_size, page_size);

  // change page permission as rwx
  if (mprotect((void *)patch_page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
    ERROR_LOG("DobbyCodePatch: mprotect RWX failed for page %p: %s", (void *)patch_page, strerror(errno));
    DOBBY_RETURN_ERROR(kDobbyErrorMemoryProtection);
  }
  if (patch_page != patch_end_page) {
    if (mprotect((void *)patch_end_page, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
      ERROR_LOG("DobbyCodePatch: mprotect RWX failed for end page %p: %s", (void *)patch_end_page, strerror(errno));
      // Try to restore first page before returning
      mprotect((void *)patch_page, page_size, PROT_READ | PROT_EXEC);
      DOBBY_RETURN_ERROR(kDobbyErrorMemoryProtection);
    }
  }

  // patch buffer
  memcpy(address, buffer, buffer_size);

  // restore page permission
  int restore_error = 0;
  if (mprotect((void *)patch_page, page_size, PROT_READ | PROT_EXEC) != 0) {
    ERROR_LOG("DobbyCodePatch: mprotect RX restore failed for page %p: %s", (void *)patch_page, strerror(errno));
    restore_error = 1;
  }
  if (patch_page != patch_end_page) {
    if (mprotect((void *)patch_end_page, page_size, PROT_READ | PROT_EXEC) != 0) {
      ERROR_LOG("DobbyCodePatch: mprotect RX restore failed for end page %p: %s", (void *)patch_end_page, strerror(errno));
      restore_error = 1;
    }
  }

  addr_t clear_start_ = (addr_t)address;
  ClearCache((void *)clear_start_, (void *)(clear_start_ + buffer_size));

  if (restore_error) {
    DOBBY_RETURN_ERROR(kDobbyErrorMemoryProtection);
  }
  DobbySetLastError(kDobbySuccess);
  return kDobbySuccess;
#else
  DobbySetLastError(kDobbySuccess);
  return kDobbySuccess;
#endif
}

#endif