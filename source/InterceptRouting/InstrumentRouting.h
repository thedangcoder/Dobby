#pragma once

#include "dobby/common.h"
#include "InterceptRouting/InterceptRouting.h"
#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"

struct InstrumentRouting : InterceptRouting {
  ClosureTrampoline *instrument_tramp = nullptr;
  ClosureTrampoline *epilogue_tramp = nullptr;

  InstrumentRouting(Interceptor::Entry *entry, dobby_instrument_callback_t pre_handler,
                    dobby_instrument_callback_t post_handler)
      : InterceptRouting(entry) {
  }

  ~InstrumentRouting() {
    if (instrument_tramp) {
      gMemoryAllocator.freeMemBlock(instrument_tramp->buffer);
      delete instrument_tramp;
      instrument_tramp = nullptr;
    }
    if (epilogue_tramp) {
      gMemoryAllocator.freeMemBlock(epilogue_tramp->buffer);
      delete epilogue_tramp;
      epilogue_tramp = nullptr;
    }
  }

  addr_t TrampolineTarget() override {
    return instrument_tramp->addr();
  }

  void GenerateInstrumentClosureTrampoline() {
    __FUNC_CALL_TRACE__();
    instrument_tramp = ::GenerateInstrumentClosureTrampoline(entry);
  }

  void GenerateEpilogueClosureTrampoline() {
    __FUNC_CALL_TRACE__();
    if (entry->post_handler) {
      epilogue_tramp = ::GenerateEpilogueClosureTrampoline(entry);
      entry->epilogue_dispatch_bridge = epilogue_tramp->addr();
    }
  }

  void BuildRouting() {
    __FUNC_CALL_TRACE__();

    GenerateInstrumentClosureTrampoline();

    GenerateEpilogueClosureTrampoline();

    GenerateTrampoline();

    GenerateRelocatedCode();

    BackupOriginCode();
  }
};

PUBLIC inline int DobbyInstrumentEx(void *address, dobby_instrument_callback_t pre_handler,
                                     dobby_instrument_callback_t post_handler) {
  __FUNC_CALL_TRACE__();
  if (!address) {
    ERROR_LOG("address is 0x0.");
    DOBBY_RETURN_ERROR(kDobbyErrorInvalidArgument);
  }

  if (!pre_handler && !post_handler) {
    ERROR_LOG("both pre_handler and post_handler are null.");
    DOBBY_RETURN_ERROR(kDobbyErrorInvalidArgument);
  }

  features::apple::arm64e_pac_strip(address);
  features::android::make_memory_readable(address, 4);

  DEBUG_LOG("----- [DobbyInstrument: %p] -----", address);

  auto entry = gInterceptor.find((addr_t)address);
  if (entry) {
    ERROR_LOG("%p already been instrumented.", address);
    DOBBY_RETURN_ERROR(kDobbyErrorAlreadyExists);
  }

  entry = new Interceptor::Entry((addr_t)address);
  entry->pre_handler = pre_handler;
  entry->post_handler = post_handler;

  auto routing = new InstrumentRouting(entry, pre_handler, post_handler);
  routing->BuildRouting();
  routing->Active();
  entry->routing = routing;

  if (DOBBY_FAILED(routing->error)) {
    ERROR_LOG("build routing error: %s", DobbyErrorString(routing->error));
    DobbyError err = routing->error;
    delete entry; // This also deletes routing via Entry destructor
    DOBBY_RETURN_ERROR(err);
  }

  gInterceptor.add(entry);

  DobbySetLastError(kDobbySuccess);
  return kDobbySuccess;
}

// Backward compatible version with only pre_handler
PUBLIC inline int DobbyInstrument(void *address, dobby_instrument_callback_t pre_handler) {
  return DobbyInstrumentEx(address, pre_handler, nullptr);
}
