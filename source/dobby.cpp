#include "dobby.h"
#include "dobby/common.h"
#include "Interceptor.h"
#include "InterceptRouting/InlineHookRouting.h"
#include "InterceptRouting/InstrumentRouting.h"
#include "InterceptRouting/NearBranchTrampoline/NearBranchTrampoline.h"
#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"
#include "MemoryAllocator/NearMemoryAllocator.h"
#include <stdint.h>

#if defined(_MSC_VER)
// MSVC: Use CRT initialization
#pragma section(".CRT$XCU", read)
static void __cdecl ctor(void);
__declspec(allocate(".CRT$XCU")) static void(__cdecl *ctor_ptr)(void) = ctor;
static void __cdecl ctor(void)
#else
__attribute__((constructor)) static void ctor()
#endif
{
  DEBUG_LOG("================================");
  DEBUG_LOG("Dobby");
  DEBUG_LOG("dobby in debug log mode, disable with cmake flag \"-DDOBBY_DEBUG=OFF\"");
  DEBUG_LOG("================================");
}

PUBLIC int DobbyDestroy(void *address) {
  __FUNC_CALL_TRACE__();
  if (!address) {
    ERROR_LOG("address is 0x0");
    DOBBY_RETURN_ERROR(kDobbyErrorInvalidArgument);
  }

  features::arm_thumb_fix_addr(address);
  features::apple::arm64e_pac_strip(address);

  auto entry = gInterceptor.find((addr_t)address);
  if (entry) {
    gInterceptor.remove((addr_t)address);
    entry->restore_orig_code();
    delete entry;
    DobbySetLastError(kDobbySuccess);
    return kDobbySuccess;
  }

  DOBBY_RETURN_ERROR(kDobbyErrorNotFound);
}

PUBLIC void dobby_set_options(bool enable_near_trampoline, dobby_alloc_near_code_callback_t alloc_near_code_callback) {
  dobby_set_near_trampoline(enable_near_trampoline);
  dobby_register_alloc_near_code_callback(alloc_near_code_callback);
}

PUBLIC uintptr_t placeholder() {
  uintptr_t x = 0;
  x += (uintptr_t)&DobbyHook;
  x += (uintptr_t)&DobbyInstrument;
  x += (uintptr_t)&DobbyInstrumentEx;
  x += (uintptr_t)&dobby_set_near_trampoline;
  x += (uintptr_t)&common_closure_bridge_handler;
  x += (uintptr_t)&dobby_register_alloc_near_code_callback;
  return x;
}

#ifndef __DOBBY_BUILD_VERSION__
#define __DOBBY_BUILD_VERSION__ "Dobby-unknown"
#endif

PUBLIC const char *DobbyGetVersion() {
  return __DOBBY_BUILD_VERSION__;
}
