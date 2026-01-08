#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_ARM)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-arm.h"

#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"

using namespace zz;
using namespace zz::arm;

void closure_bridge_init_impl() {

#define _ turbo_assembler_.
  TurboAssembler turbo_assembler_(0);

  _ sub(sp, sp, Operand(14 * 4));
  _ str(lr, MemOperand(sp, 13 * 4));
  _ str(r12, MemOperand(sp, 12 * 4));
  _ str(r11, MemOperand(sp, 11 * 4));
  _ str(r10, MemOperand(sp, 10 * 4));
  _ str(r9, MemOperand(sp, 9 * 4));
  _ str(r8, MemOperand(sp, 8 * 4));
  _ str(r7, MemOperand(sp, 7 * 4));
  _ str(r6, MemOperand(sp, 6 * 4));
  _ str(r5, MemOperand(sp, 5 * 4));
  _ str(r4, MemOperand(sp, 4 * 4));
  _ str(r3, MemOperand(sp, 3 * 4));
  _ str(r2, MemOperand(sp, 2 * 4));
  _ str(r1, MemOperand(sp, 1 * 4));
  _ str(r0, MemOperand(sp, 0 * 4));

  // store sp
  _ add(r0, sp, Operand(14 * 4));
  _ sub(sp, sp, Operand(8));
  _ str(r0, MemOperand(sp, 4));

  // stack align
  _ sub(sp, sp, Operand(8));

  _ mov(r0, Operand(sp));
  _ mov(r1, Operand(r12));
  _ CallFunction(ExternalReference((void *)common_closure_bridge_handler));

  // stack align
  _ add(sp, sp, Operand(8));

  // restore sp placeholder stack
  _ add(sp, sp, Operand(8));

  _ ldr(r0, MemOperand(sp, 4, PostIndex));
  _ ldr(r1, MemOperand(sp, 4, PostIndex));
  _ ldr(r2, MemOperand(sp, 4, PostIndex));
  _ ldr(r3, MemOperand(sp, 4, PostIndex));
  _ ldr(r4, MemOperand(sp, 4, PostIndex));
  _ ldr(r5, MemOperand(sp, 4, PostIndex));
  _ ldr(r6, MemOperand(sp, 4, PostIndex));
  _ ldr(r7, MemOperand(sp, 4, PostIndex));
  _ ldr(r8, MemOperand(sp, 4, PostIndex));
  _ ldr(r9, MemOperand(sp, 4, PostIndex));
  _ ldr(r10, MemOperand(sp, 4, PostIndex));
  _ ldr(r11, MemOperand(sp, 4, PostIndex));
  _ ldr(r12, MemOperand(sp, 4, PostIndex));
  _ ldr(lr, MemOperand(sp, 4, PostIndex));

  // auto switch A32 & T32 with `least significant bit`, refer `docs/A32_T32_states_switch.md`
  _ mov(pc, Operand(r12));

  auto code = AssemblerCodeBuilder::FinalizeFromTurboAssembler(static_cast<AssemblerBase *>(&turbo_assembler_));
  closure_bridge_addr = (asm_func_t)code.addr();

  DEBUG_LOG("[closure bridge] closure bridge at %p", closure_bridge_addr);
#undef _
}

asm_func_t get_closure_bridge_addr() {
  closure_bridge_init();
  return closure_bridge_addr;
}

#endif
