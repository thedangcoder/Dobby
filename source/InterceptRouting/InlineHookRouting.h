#pragma once

#include "dobby/common.h"
#include "InterceptRouting/InterceptRouting.h"
#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"

struct InlineHookRouting : InterceptRouting {
  addr_t fake_func;

  InlineHookRouting(Interceptor::Entry *entry, addr_t fake_func) : InterceptRouting(entry), fake_func(fake_func) {
  }

  ~InlineHookRouting() = default;

  addr_t TrampolineTarget() override {
    return fake_func;
  }

  void BuildRouting() {
    __FUNC_CALL_TRACE__();

    GenerateTrampoline();

    GenerateRelocatedCode();

    BackupOriginCode();
  }
};

PUBLIC inline int DobbyHook(void *address, void *fake_func, void **out_origin_func) {
  __FUNC_CALL_TRACE__();
  if (!address) {
    ERROR_LOG("address is 0x0");
    DOBBY_RETURN_ERROR(kDobbyErrorInvalidArgument);
  }

  features::apple::arm64e_pac_strip(address);
  features::apple::arm64e_pac_strip(fake_func);
  features::android::make_memory_readable(address, 4);

  DEBUG_LOG("----- [DobbyHook: %p] -----", address);

  auto entry = gInterceptor.find((addr_t)address);
  if (entry) {
    ERROR_LOG("%p already been hooked.", address);
    DOBBY_RETURN_ERROR(kDobbyErrorAlreadyExists);
  }

  entry = new Interceptor::Entry((addr_t)address);
  entry->fake_func_addr = (addr_t)fake_func;

  auto routing = new InlineHookRouting(entry, (addr_t)fake_func);
  routing->BuildRouting();
  routing->Active();
  entry->routing = routing;

  if (DOBBY_FAILED(routing->error)) {
    ERROR_LOG("build routing error: %s", DobbyErrorString(routing->error));
    DobbyError err = routing->error;
    delete entry; // This also deletes routing via Entry destructor
    DOBBY_RETURN_ERROR(err);
  }

  if (out_origin_func) {
    *out_origin_func = (void *)entry->relocated.addr();
    features::apple::arm64e_pac_strip_and_sign(*out_origin_func);
  }

  gInterceptor.add(entry);

  DobbySetLastError(kDobbySuccess);
  return kDobbySuccess;
}
