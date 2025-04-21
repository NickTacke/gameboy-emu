#include "gb/cpu.h"
#include "gb/mmu.h"

#include <iostream>
#include <array>

namespace gb {

void CPU::Reset() {
  pc_ = 0x0100;
  sp_ = 0xFFFE;
  ime_ = false;

  // Reset registers
  a_ = f_ = b_ = c_ = d_ = e_ = h_ = l_ = 0;
}

uint8_t CPU::FetchOpcode() {
  uint8_t opcode = MMU::Instance().Read(pc_);
  pc_++;
  return opcode;
}

uint8_t CPU::Fetch8() {
  return FetchOpcode();
}

uint16_t CPU::Fetch16() {
  uint16_t low = FetchOpcode();
  uint16_t high = FetchOpcode();
  return static_cast<uint16_t>(high) << 8 | low;
}

void CPU::Execute(uint8_t opcode) {
  static const std::array<void (CPU::*)(), 256> kDispatch = [] {
    std::array<void (CPU::*)(), 256> t{};
#define OPCODE(name, code) t[code] = &CPU::Op##name;
#include "gb/opcode_list.h"
#undef OPCODE
    return t;
  }();

  (this->*kDispatch[opcode])();
}

void CPU::Step() {
  uint8_t opcode = FetchOpcode();
  Execute(opcode);
}

// Per-opcode handlers
void CPU::OpNOP() {}

void CPU::OpLD_BC_d16() {
  uint16_t val = Fetch16();
  b_ = static_cast<uint8_t>(val >> 8);
  c_ = static_cast<uint8_t>(val & 0xFF);
}

void CPU::OpLD_addrBC_A() {}
void CPU::OpINC_BC() {}
void CPU::OpINC_B() {}
void CPU::OpDEC_B() {}
void CPU::OpLD_B_d8() {}
void CPU::OpRLCA() {}
void CPU::OpLD_a16_SP() {}
void CPU::OpADD_HL_BC() {}
void CPU::OpLD_A_addrBC() {}
void CPU::OpDEC_BC() {}
void CPU::OpINC_C() {}
void CPU::OpDEC_C() {}
void CPU::OpLD_C_d8() {}
void CPU::OpRRCA() {}
void CPU::OpSTOP() {}
void CPU::OpLD_DE_d16() {}
void CPU::OpLD_addrDE_A() {}
void CPU::OpINC_DE() {}
void CPU::OpINC_D() {}
void CPU::OpDEC_D() {}
void CPU::OpLD_D_d8() {}
void CPU::OpRLA() {}
void CPU::OpJR_r8() {}
void CPU::OpADD_HL_DE() {}
void CPU::OpLD_A_addrDE() {}
void CPU::OpDEC_DE() {}
void CPU::OpINC_E() {}
void CPU::OpDEC_E() {}
void CPU::OpLD_E_d8() {}
void CPU::OpRRA() {}
void CPU::OpJR_NZ_r8() {}
void CPU::OpLD_HL_d16() {}
void CPU::OpLD_addrHLinc_A() {}
void CPU::OpINC_HL() {}
void CPU::OpINC_H() {}
void CPU::OpDEC_H() {}
void CPU::OpLD_H_d8() {}
void CPU::OpDAA() {}
void CPU::OpJR_Z_r8() {}
void CPU::OpADD_HL_HL() {}
void CPU::OpLD_A_addrHLinc() {}
void CPU::OpDEC_HL() {}
void CPU::OpINC_L() {}
void CPU::OpDEC_L() {}
void CPU::OpLD_L_d8() {}
void CPU::OpCPL() {}
void CPU::OpJR_NC_r8() {}
void CPU::OpLD_SP_d16() {}
void CPU::OpLD_addrHLdec_A() {}
void CPU::OpINC_SP() {}
void CPU::OpINC_addrHL() {}
void CPU::OpDEC_addrHL() {}
void CPU::OpLD_addrHL_d8() {}
void CPU::OpSCF() {}
void CPU::OpJR_C_r8() {}
void CPU::OpADD_HL_SP() {}
void CPU::OpLD_A_addrHLdec() {}
void CPU::OpDEC_SP() {}
void CPU::OpINC_A() {}
void CPU::OpDEC_A() {}
void CPU::OpLD_A_d8() {}
void CPU::OpCCF() {}
void CPU::OpLD_B_B() {}
void CPU::OpLD_B_C() {}
void CPU::OpLD_B_D() {}
void CPU::OpLD_B_E() {}
void CPU::OpLD_B_H() {}
void CPU::OpLD_B_L() {}
void CPU::OpLD_B_addrHL() {}
void CPU::OpLD_B_A() {}
void CPU::OpLD_C_B() {}
void CPU::OpLD_C_C() {}
void CPU::OpLD_C_D() {}
void CPU::OpLD_C_E() {}
void CPU::OpLD_C_H() {}
void CPU::OpLD_C_L() {}
void CPU::OpLD_C_addrHL() {}
void CPU::OpLD_C_A() {}
void CPU::OpLD_D_B() {}
void CPU::OpLD_D_C() {}
void CPU::OpLD_D_D() {}
void CPU::OpLD_D_E() {}
void CPU::OpLD_D_H() {}
void CPU::OpLD_D_L() {}
void CPU::OpLD_D_addrHL() {}
void CPU::OpLD_D_A() {}
void CPU::OpLD_E_B() {}
void CPU::OpLD_E_C() {}
void CPU::OpLD_E_D() {}
void CPU::OpLD_E_E() {}
void CPU::OpLD_E_H() {}
void CPU::OpLD_E_L() {}
void CPU::OpLD_E_addrHL() {}
void CPU::OpLD_E_A() {}
void CPU::OpLD_H_B() {}
void CPU::OpLD_H_C() {}
void CPU::OpLD_H_D() {}
void CPU::OpLD_H_E() {}
void CPU::OpLD_H_H() {}
void CPU::OpLD_H_L() {}
void CPU::OpLD_H_addrHL() {}
void CPU::OpLD_H_A() {}
void CPU::OpLD_L_B() {}
void CPU::OpLD_L_C() {}
void CPU::OpLD_L_D() {}
void CPU::OpLD_L_E() {}
void CPU::OpLD_L_H() {}
void CPU::OpLD_L_L() {}
void CPU::OpLD_L_addrHL() {}
void CPU::OpLD_L_A() {}
void CPU::OpLD_addrHL_B() {}
void CPU::OpLD_addrHL_C() {}
void CPU::OpLD_addrHL_D() {}
void CPU::OpLD_addrHL_E() {}
void CPU::OpLD_addrHL_H() {}
void CPU::OpLD_addrHL_L() {}
void CPU::OpHALT() {}
void CPU::OpLD_addrHL_A() {}
void CPU::OpLD_A_B() {}
void CPU::OpLD_A_C() {}
void CPU::OpLD_A_D() {}
void CPU::OpLD_A_E() {}
void CPU::OpLD_A_H() {}
void CPU::OpLD_A_L() {}
void CPU::OpLD_A_addrHL() {}
void CPU::OpLD_A_A() {}
void CPU::OpADD_A_B() {}
void CPU::OpADD_A_C() {}
void CPU::OpADD_A_D() {}
void CPU::OpADD_A_E() {}
void CPU::OpADD_A_H() {}
void CPU::OpADD_A_L() {}
void CPU::OpADD_A_addrHL() {}
void CPU::OpADD_A_A() {}
void CPU::OpADC_A_B() {}
void CPU::OpADC_A_C() {}
void CPU::OpADC_A_D() {}
void CPU::OpADC_A_E() {}
void CPU::OpADC_A_H() {}
void CPU::OpADC_A_L() {}
void CPU::OpADC_A_addrHL() {}
void CPU::OpADC_A_A() {}
void CPU::OpSUB_B() {}
void CPU::OpSUB_C() {}
void CPU::OpSUB_D() {}
void CPU::OpSUB_E() {}
void CPU::OpSUB_H() {}
void CPU::OpSUB_L() {}
void CPU::OpSUB_addrHL() {}
void CPU::OpSUB_A() {}
void CPU::OpSBC_A_B() {}
void CPU::OpSBC_A_C() {}
void CPU::OpSBC_A_D() {}
void CPU::OpSBC_A_E() {}
void CPU::OpSBC_A_H() {}
void CPU::OpSBC_A_L() {}
void CPU::OpSBC_A_addrHL() {}
void CPU::OpSBC_A_A() {}
void CPU::OpAND_B() {}
void CPU::OpAND_C() {}
void CPU::OpAND_D() {}
void CPU::OpAND_E() {}
void CPU::OpAND_H() {}
void CPU::OpAND_L() {}
void CPU::OpAND_addrHL() {}
void CPU::OpAND_A() {}
void CPU::OpXOR_B() {}
void CPU::OpXOR_C() {}
void CPU::OpXOR_D() {}
void CPU::OpXOR_E() {}
void CPU::OpXOR_H() {}
void CPU::OpXOR_L() {}
void CPU::OpXOR_addrHL() {}
void CPU::OpXOR_A() {}
void CPU::OpOR_B() {}
void CPU::OpOR_C() {}
void CPU::OpOR_D() {}
void CPU::OpOR_E() {}
void CPU::OpOR_H() {}
void CPU::OpOR_L() {}
void CPU::OpOR_addrHL() {}
void CPU::OpOR_A() {}
void CPU::OpCP_B() {}
void CPU::OpCP_C() {}
void CPU::OpCP_D() {}
void CPU::OpCP_E() {}
void CPU::OpCP_H() {}
void CPU::OpCP_L() {}
void CPU::OpCP_addrHL() {}
void CPU::OpCP_A() {}
void CPU::OpRET_NZ() {}
void CPU::OpPOP_BC() {}
void CPU::OpJP_NZ_a16() {}
void CPU::OpJP_a16() {}
void CPU::OpCALL_NZ_a16() {}
void CPU::OpPUSH_BC() {}
void CPU::OpADD_A_d8() {}
void CPU::OpRST_00H() {}
void CPU::OpRET_Z() {}
void CPU::OpRET() {}
void CPU::OpJP_Z_a16() {}
void CPU::OpCALL_Z_a16() {}
void CPU::OpCALL_a16() {}
void CPU::OpADC_A_d8() {}
void CPU::OpRST_08H() {}
void CPU::OpRET_NC() {}
void CPU::OpPOP_DE() {}
void CPU::OpJP_NC_a16() {}
void CPU::OpCALL_NC_a16() {}
void CPU::OpPUSH_DE() {}
void CPU::OpSUB_d8() {}
void CPU::OpRST_10H() {}
void CPU::OpRET_C() {}
void CPU::OpRETI() {}
void CPU::OpJP_C_a16() {}
void CPU::OpCALL_C_a16() {}
void CPU::OpSBC_A_d8() {}
void CPU::OpRST_18H() {}
void CPU::OpLDH_a8_A() {}
void CPU::OpPOP_HL() {}
void CPU::OpLD_addrC_A() {}
void CPU::OpPUSH_HL() {}
void CPU::OpAND_d8() {}
void CPU::OpRST_20H() {}
void CPU::OpADD_SP_r8() {}
void CPU::OpJP_HL() {}
void CPU::OpLD_a16_A() {}
void CPU::OpXOR_d8() {}
void CPU::OpRST_28H() {}
void CPU::OpLDH_A_a8() {}
void CPU::OpPOP_AF() {}
void CPU::OpLD_A_addrC() {}
void CPU::OpDI() {}
void CPU::OpPUSH_AF() {}
void CPU::OpOR_d8() {}
void CPU::OpRST_30H() {}
void CPU::OpLD_HL_SPplusr8() {}
void CPU::OpLD_SP_HL() {}
void CPU::OpLD_A_a16() {}
void CPU::OpEI() {}
void CPU::OpCP_d8() {}
void CPU::OpRST_38H() {}

} // namespace gb