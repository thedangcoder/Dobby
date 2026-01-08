#include "platform_detect_macro.h"
#if TARGET_ARCH_ARM

#include "core/assembler/assembler-arm.h"

void PseudoLabel::link_confused_instructions(CodeMemBuffer *buffer) {
  for (auto &ref_label_insn : ref_insts) {
    arm_inst_t inst = buffer->LoadARMInst(ref_label_insn.inst_offset);
    if (ref_label_insn.link_type == kLdrLiteral) {
      int64_t pc = ref_label_insn.inst_offset + ARM_PC_OFFSET;
      assert(pc % 4 == 0);
      int32_t imm12 = pos - pc;
      if (imm12 > 0) {
        set_bit(inst, 23, 1);
      } else {
        set_bit(inst, 23, 0);
        imm12 = -imm12;
      }
      set_bits(inst, 0, 11, imm12);
    }
    buffer->RewriteARMInst(ref_label_insn.inst_offset, inst);
  }
}

namespace zz {
namespace arm {

void Assembler::EmitARMInst(arm_inst_t instr) {
  code_buffer_.EmitARMInst(instr);
}

void Assembler::EmitAddress(uint32_t value) {
  code_buffer_.Emit<int32_t>(value);
}

} // namespace arm
} // namespace zz

#endif
