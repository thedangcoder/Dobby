#include "platform_detect_macro.h"
#if defined(TARGET_ARCH_IA32)

#include "dobby/dobby_internal.h"

#include "core/assembler/assembler-ia32.h"

#include "TrampolineBridge/ClosureTrampolineBridge/ClosureTrampoline.h"
#include "TrampolineBridge/ClosureTrampolineBridge/common_bridge_handler.h"

using namespace zz;
using namespace zz::x86;

extern asm_func_t get_closure_bridge_addr();

ClosureTrampoline *GenerateClosureTrampoline(void *carry_data, void *carry_handler) {
#define _ turbo_assembler_.
  TurboAssembler turbo_assembler_(0);

  // placeholder for closure_tramp pointer - will be patched later
  _ sub(esp, Immediate(4, 32));
  _ mov(Address(esp, 4 * 0), Immediate(0, 32)); // placeholder
  _ jmp(Immediate(0, 32)); // placeholder for relative jump

  _ relocDataLabels();

  auto tramp_buffer = turbo_assembler_.code_buffer();
  auto tramp_block = tramp_buffer->dup();

  // Allocate executable memory and copy
  auto exec_block = MemoryAllocator::Shared()->allocExecBlock(tramp_block.size + 16);
  if (exec_block.addr() == 0) {
    return nullptr;
  }
  memcpy((void *)exec_block.addr(), (void *)tramp_block.addr(), tramp_block.size);

  auto closure_tramp = new ClosureTrampoline(TRAMPOLINE_UNKNOWN, exec_block, carry_data, carry_handler);

  // Patch the mov immediate with closure_tramp pointer
  // mov [esp], imm32 is encoded as: C7 04 24 <imm32>
  // sub esp, 4 is 3 bytes, so mov starts at offset 3
  const uint32_t mov_imm_off = 3 + 3; // sub(3) + mov opcode(3)
  uint32_t *mov_imm_addr = (uint32_t *)((addr_t)exec_block.addr() + mov_imm_off);
  *mov_imm_addr = (uint32_t)(uintptr_t)closure_tramp;

  // Patch the jmp relative offset
  // jmp rel32 is at offset 3 + 7 = 10 (sub + mov)
  const uint32_t jmp_off = 3 + 7; // sub(3) + mov(7)
  int32_t *jmp_rel_addr = (int32_t *)((addr_t)exec_block.addr() + jmp_off + 1); // +1 for opcode
  addr_t jmp_next_ip = exec_block.addr() + jmp_off + 5; // ip after jmp instruction
  *jmp_rel_addr = (int32_t)((addr_t)get_closure_bridge_addr() - jmp_next_ip);

  ClearCache((void *)exec_block.addr(), (void *)(exec_block.addr() + exec_block.size));

  DEBUG_LOG("[closure trampoline] closure trampoline addr: %p, size: %d", closure_tramp->addr(), closure_tramp->size());
  return closure_tramp;
#undef _
}

#endif
