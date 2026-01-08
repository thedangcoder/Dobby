#pragma once

#include "dobby/dobby_internal.h"
#include "dobby/platform_mutex.h"

#include "Interceptor.h"
#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"

inline asm_func_t closure_bridge_addr = nullptr;
inline DobbyMutex closure_bridge_mutex;

void closure_bridge_init_impl();

inline void closure_bridge_init() {
  DobbyLockGuard lock(closure_bridge_mutex);
  if (!closure_bridge_addr) {
    closure_bridge_init_impl();
  }
}

void get_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address);

void set_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address);

void *get_func_ret_address(DobbyRegisterContext *ctx);

void set_func_ret_address(DobbyRegisterContext *ctx, void *address);

PUBLIC extern "C" inline void common_closure_bridge_handler(DobbyRegisterContext *ctx, ClosureTrampoline *tramp) {
  typedef void (*routing_handler_t)(Interceptor::Entry *, DobbyRegisterContext *);
  auto routing_handler = (routing_handler_t)features::apple::arm64e_pac_strip_and_sign(tramp->carry_handler);
  routing_handler((Interceptor::Entry *)tramp->carry_data, ctx);
  return;
}
