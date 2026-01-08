#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_IA32)

#include "dobby/dobby_internal.h"

void set_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address) {
  addr_t esp = ctx->esp;

  addr_t entry_placeholder_stack_addr = esp - 4;
  *(addr_t *)entry_placeholder_stack_addr = (addr_t)address;
}

void get_routing_bridge_next_hop(DobbyRegisterContext *ctx, void *address) {
}

void *get_func_ret_address(DobbyRegisterContext *ctx) {
  // On x86, return address is at [esp] (top of stack at function entry)
  // But in closure bridge context, it's stored at esp - 4 placeholder
  addr_t esp = ctx->esp;
  addr_t entry_placeholder_stack_addr = esp - 4;
  return *(void **)entry_placeholder_stack_addr;
}

void set_func_ret_address(DobbyRegisterContext *ctx, void *address) {
  // On x86, we use the same placeholder as set_routing_bridge_next_hop
  addr_t esp = ctx->esp;
  addr_t entry_placeholder_stack_addr = esp - 4;
  *(addr_t *)entry_placeholder_stack_addr = (addr_t)address;
}

#endif