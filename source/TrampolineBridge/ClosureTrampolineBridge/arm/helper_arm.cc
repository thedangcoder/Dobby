#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_ARM)

#include "dobby/dobby_internal.h"

void set_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address) {
  *reinterpret_cast<void **>(&ctx->general.regs.r12) = address;
}

void get_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address) {
}

void *get_func_ret_address(DobbyRegisterContext *ctx) {
  return (void *)(uintptr_t)ctx->lr;
}

void set_func_ret_address(DobbyRegisterContext *ctx, void *address) {
  ctx->lr = (uint32_t)(uintptr_t)address;
}

#endif