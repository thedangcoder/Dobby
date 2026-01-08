#include "platform_detect_macro.h"
#if TARGET_ARCH_IA32

#include "core/assembler/assembler-ia32.h"

using namespace zz::x86;

void Assembler::jmp(Immediate imm) {
  code_buffer_.Emit<int8_t>(0xE9);
  code_buffer_.Emit<int32_t>((int)imm.value());
}

addr32_t TurboAssembler::CurrentIP() {
  return pc_offset() + (addr_t)fixed_addr;
}

void PseudoLabel::link_confused_instructions(CodeMemBuffer *buffer) {
  for (auto &ref_label_insn : ref_insts) {
    int64_t new_offset = pos - ref_label_insn.inst_offset;

    if (ref_label_insn.link_type == kDisp32_off_7) {
      // why 7 ?
      // use `call` and `pop` get the runtime ip register
      // but the ip register not the real call next insn
      // it need add two insn length == 7
      int disp32_fix_pos = ref_label_insn.inst_offset - sizeof(int32_t);
      buffer->Store<int32_t>(disp32_fix_pos, (int32_t)(new_offset + 7));
    }
  }
}

#endif