#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_X64)

#include "dobby/dobby_internal.h"

void set_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address) {
  ctx->ret = (uint64_t)address;
}

void get_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address) {
}

void *get_func_ret_address(DobbyRegisterContext *ctx) {
  // On x64, when the target function is called, the return address is at [rsp].
  // ctx->general.regs.rsp contains the original rsp value at function entry.
  // The return address is at that location.
  uint64_t *ret_addr_ptr = (uint64_t *)ctx->general.regs.rsp;
  return (void *)*ret_addr_ptr;
}

void set_func_ret_address(DobbyRegisterContext *ctx, void *address) {
  // Modify the return address on the actual stack
  uint64_t *ret_addr_ptr = (uint64_t *)ctx->general.regs.rsp;
  *ret_addr_ptr = (uint64_t)address;
}

#endif