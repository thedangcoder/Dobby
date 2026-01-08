#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_ARM)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-arm.h"

#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"
#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"

using namespace zz;
using namespace zz::arm;

extern asm_func_t get_closure_bridge_addr();

ClosureTrampoline *GenerateClosureTrampoline(void *carry_data, void *carry_handler) {
  if (!closure_bridge_addr) {
    closure_bridge_init();
  }

#define _ turbo_assembler_.
  TurboAssembler turbo_assembler_(0);

  PseudoLabel entry_label(0);
  PseudoLabel forward_bridge_label(0);

  _ Ldr(r12, &entry_label);
  _ Ldr(pc, &forward_bridge_label);
  _ bindLabel(&entry_label);
  _ EmitAddress((uint32_t)(uintptr_t)0); // placeholder for closure_tramp pointer
  _ bindLabel(&forward_bridge_label);
  _ EmitAddress((uint32_t)(uintptr_t)get_closure_bridge_addr());

  auto tramp_block = AssemblerCodeBuilder::FinalizeFromTurboAssembler(static_cast<AssemblerBase *>(&turbo_assembler_));

  auto closure_tramp = new ClosureTrampoline(CLOSURE_TRAMPOLINE_ARM, tramp_block, carry_data, carry_handler);

  // Patch the entry_label data to point to closure_tramp
  const uint32_t entry_label_off = 2 * 4; // after 2 instructions
  uint32_t *entry_addr = (uint32_t *)((addr_t)tramp_block.addr() + entry_label_off);
  *entry_addr = (uint32_t)(uintptr_t)closure_tramp;
  ClearCache((void *)tramp_block.addr(), (void *)(tramp_block.addr() + tramp_block.size));

  DEBUG_LOG("[closure trampoline] closure trampoline addr: %p, size: %d", closure_tramp->addr(), closure_tramp->size());
  return closure_tramp;
#undef _
}

#endif
