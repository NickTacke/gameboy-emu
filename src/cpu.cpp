#include "gb/cpu.h"
#include "gb/mmu.h"

#include <iostream>
#include <array>
#include <cstdint> // Include cstdint for fixed-width integers

namespace gb {

// === Core Execution Logic ===

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
  return (static_cast<uint16_t>(high) << 8) | low;
}

void CPU::Execute(uint8_t opcode) {
  static const std::array<void (CPU::*)(), 256> kDispatch = [] {
    std::array<void (CPU::*)(), 256> t{};
#define OPCODE(name, code) t[code] = &CPU::Op##name;
#include "gb/opcode_list.h"
#undef OPCODE
    // Map undefined opcodes to a specific handler (e.g., NOP or an error handler)
    // Alternatively, ensure the opcode list defines them pointing to a handler.
    // For now, assuming opcode_list.h covers all 256, potentially pointing undefined to NOP/PREFIX_CB.
    return t;
  }();

  (this->*kDispatch[opcode])();
}

void CPU::Step() {
  uint8_t opcode = FetchOpcode();
  Execute(opcode);
}

// === Flag Helpers ===

void CPU::SetFlag(uint8_t flag_mask, bool set) {
  if (set) {
    f_ |= flag_mask;
  } else {
    f_ &= ~flag_mask;
  }
  f_ &= 0xF0; // Lower 4 bits are always 0
}

bool CPU::GetFlag(uint8_t flag_mask) const {
  return (f_ & flag_mask) != 0;
}

// === Stack Helpers ===

void CPU::PushWord(uint16_t word) {
  sp_--;
  MMU::Instance().Write(sp_, static_cast<uint8_t>(word >> 8)); // High byte
  sp_--;
  MMU::Instance().Write(sp_, static_cast<uint8_t>(word & 0xFF)); // Low byte
}

uint16_t CPU::PopWord() {
  uint16_t low = MMU::Instance().Read(sp_);
  sp_++;
  uint16_t high = MMU::Instance().Read(sp_);
  sp_++;
  return (high << 8) | low;
}

// === Arithmetic Helpers ===

void CPU::Add8(uint8_t val, bool use_carry) {
  uint8_t current_a = a_;
  uint8_t carry = (use_carry && GetFlag(kCarryFlagMask)) ? 1 : 0;
  uint16_t result = static_cast<uint16_t>(current_a) + val + carry;
  a_ = static_cast<uint8_t>(result & 0xFF);

  SetFlag(kZeroFlagMask, a_ == 0);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, ((current_a & 0x0F) + (val & 0x0F) + carry) > 0x0F);
  SetFlag(kCarryFlagMask, result > 0xFF);
}

void CPU::Sub8(uint8_t val, bool use_carry) {
  uint8_t current_a = a_;
  uint8_t carry = (use_carry && GetFlag(kCarryFlagMask)) ? 1 : 0;
  // Simulate subtraction using addition with two's complement might be needed
  // for precise flag calculation depending on how flags are defined.
  // Using direct subtraction here, assuming flags match common definitions.
  uint16_t result_sub = static_cast<uint16_t>(current_a) - val - carry;
  uint8_t result_byte = static_cast<uint8_t>(result_sub & 0xFF);

  SetFlag(kZeroFlagMask, result_byte == 0);
  SetFlag(kSubtractFlagMask, true);
  // Half Carry (Borrow) for SUB/SBC/CP
  SetFlag(kHalfCarryFlagMask, (current_a & 0x0F) < ((val & 0x0F) + carry));
  // Carry (Borrow) for SUB/SBC/CP
  SetFlag(kCarryFlagMask, result_sub > 0xFF); // Check for borrow (result < 0 implies overflow in unsigned)

  a_ = result_byte;
}

uint8_t CPU::Inc8(uint8_t reg) {
  uint8_t result = reg + 1;
  SetFlag(kZeroFlagMask, result == 0);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, (reg & 0x0F) == 0x0F); // Half carry if lower nibble was 0xF
  // Carry flag is not affected
  return result;
}

uint8_t CPU::Dec8(uint8_t reg) {
  uint8_t result = reg - 1;
  SetFlag(kZeroFlagMask, result == 0);
  SetFlag(kSubtractFlagMask, true);
  SetFlag(kHalfCarryFlagMask, (reg & 0x0F) == 0x00); // Half borrow if lower nibble was 0x0
  // Carry flag is not affected
  return result;
}

// === Logic Helpers ===

void CPU::And8(uint8_t val) {
  a_ &= val;
  SetFlag(kZeroFlagMask, a_ == 0);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, true); // Always set for AND
  SetFlag(kCarryFlagMask, false);
}

void CPU::Or8(uint8_t val) {
  a_ |= val;
  SetFlag(kZeroFlagMask, a_ == 0);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, false);
}

void CPU::Xor8(uint8_t val) {
  a_ ^= val;
  SetFlag(kZeroFlagMask, a_ == 0);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, false);
}

void CPU::Cp8(uint8_t val) {
  uint8_t current_a = a_;
  uint16_t result_sub = static_cast<uint16_t>(current_a) - val;
  uint8_t result_byte = static_cast<uint8_t>(result_sub & 0xFF);

  SetFlag(kZeroFlagMask, result_byte == 0);
  SetFlag(kSubtractFlagMask, true);
  SetFlag(kHalfCarryFlagMask, (current_a & 0x0F) < (val & 0x0F));
  SetFlag(kCarryFlagMask, result_sub > 0xFF);
  // Note: A register is NOT modified by CP
}

// === Rotate/Shift Helpers (A Register) ===

void CPU::RlcA() { // RLCA 0x07
  uint8_t carry = (a_ & 0x80) >> 7;
  a_ = (a_ << 1) | carry;
  SetFlag(kZeroFlagMask, false);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, carry);
}

void CPU::RrcA() { // RRCA 0x0F
  uint8_t carry = a_ & 0x01;
  a_ = (a_ >> 1) | (carry << 7);
  SetFlag(kZeroFlagMask, false);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, carry);
}

void CPU::RlA() { // RLA 0x17
  uint8_t old_carry = GetFlag(kCarryFlagMask) ? 1 : 0;
  uint8_t new_carry = (a_ & 0x80) >> 7;
  a_ = (a_ << 1) | old_carry;
  SetFlag(kZeroFlagMask, false);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, new_carry);
}

void CPU::RrA() { // RRA 0x1F
  uint8_t old_carry = GetFlag(kCarryFlagMask) ? 1 : 0;
  uint8_t new_carry = a_ & 0x01;
  a_ = (a_ >> 1) | (old_carry << 7);
  SetFlag(kZeroFlagMask, false);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, new_carry);
}


// === Opcode Handlers ===

// --- Load Instructions (LD) ---

// 8-bit Loads
void CPU::OpLD_B_d8() { b_ = Fetch8(); } // 0x06
void CPU::OpLD_C_d8() { c_ = Fetch8(); } // 0x0E
void CPU::OpLD_D_d8() { d_ = Fetch8(); } // 0x16
void CPU::OpLD_E_d8() { e_ = Fetch8(); } // 0x1E
void CPU::OpLD_H_d8() { h_ = Fetch8(); } // 0x26
void CPU::OpLD_L_d8() { l_ = Fetch8(); } // 0x2E
void CPU::OpLD_A_d8() { a_ = Fetch8(); } // 0x3E

void CPU::OpLD_B_B() { /* NOP */ }         // 0x40
void CPU::OpLD_B_C() { b_ = c_; }         // 0x41
void CPU::OpLD_B_D() { b_ = d_; }         // 0x42
void CPU::OpLD_B_E() { b_ = e_; }         // 0x43
void CPU::OpLD_B_H() { b_ = h_; }         // 0x44
void CPU::OpLD_B_L() { b_ = l_; }         // 0x45
void CPU::OpLD_B_addrHL() { b_ = MMU::Instance().Read(hl()); } // 0x46
void CPU::OpLD_B_A() { b_ = a_; }         // 0x47
void CPU::OpLD_C_B() { c_ = b_; }         // 0x48
void CPU::OpLD_C_C() { /* NOP */ }         // 0x49
void CPU::OpLD_C_D() { c_ = d_; }         // 0x4A
void CPU::OpLD_C_E() { c_ = e_; }         // 0x4B
void CPU::OpLD_C_H() { c_ = h_; }         // 0x4C
void CPU::OpLD_C_L() { c_ = l_; }         // 0x4D
void CPU::OpLD_C_addrHL() { c_ = MMU::Instance().Read(hl()); } // 0x4E
void CPU::OpLD_C_A() { c_ = a_; }         // 0x4F
void CPU::OpLD_D_B() { d_ = b_; }         // 0x50
void CPU::OpLD_D_C() { d_ = c_; }         // 0x51
void CPU::OpLD_D_D() { /* NOP */ }         // 0x52
void CPU::OpLD_D_E() { d_ = e_; }         // 0x53
void CPU::OpLD_D_H() { d_ = h_; }         // 0x54
void CPU::OpLD_D_L() { d_ = l_; }         // 0x55
void CPU::OpLD_D_addrHL() { d_ = MMU::Instance().Read(hl()); } // 0x56
void CPU::OpLD_D_A() { d_ = a_; }         // 0x57
void CPU::OpLD_E_B() { e_ = b_; }         // 0x58
void CPU::OpLD_E_C() { e_ = c_; }         // 0x59
void CPU::OpLD_E_D() { e_ = d_; }         // 0x5A
void CPU::OpLD_E_E() { /* NOP */ }         // 0x5B
void CPU::OpLD_E_H() { e_ = h_; }         // 0x5C
void CPU::OpLD_E_L() { e_ = l_; }         // 0x5D
void CPU::OpLD_E_addrHL() { e_ = MMU::Instance().Read(hl()); } // 0x5E
void CPU::OpLD_E_A() { e_ = a_; }         // 0x5F
void CPU::OpLD_H_B() { h_ = b_; }         // 0x60
void CPU::OpLD_H_C() { h_ = c_; }         // 0x61
void CPU::OpLD_H_D() { h_ = d_; }         // 0x62
void CPU::OpLD_H_E() { h_ = e_; }         // 0x63
void CPU::OpLD_H_H() { /* NOP */ }         // 0x64
void CPU::OpLD_H_L() { h_ = l_; }         // 0x65
void CPU::OpLD_H_addrHL() { h_ = MMU::Instance().Read(hl()); } // 0x66
void CPU::OpLD_H_A() { h_ = a_; }         // 0x67
void CPU::OpLD_L_B() { l_ = b_; }         // 0x68
void CPU::OpLD_L_C() { l_ = c_; }         // 0x69
void CPU::OpLD_L_D() { l_ = d_; }         // 0x6A
void CPU::OpLD_L_E() { l_ = e_; }         // 0x6B
void CPU::OpLD_L_H() { l_ = h_; }         // 0x6C
void CPU::OpLD_L_L() { /* NOP */ }         // 0x6D
void CPU::OpLD_L_addrHL() { l_ = MMU::Instance().Read(hl()); } // 0x6E
void CPU::OpLD_L_A() { l_ = a_; }         // 0x6F

void CPU::OpLD_addrHL_B() { MMU::Instance().Write(hl(), b_); } // 0x70
void CPU::OpLD_addrHL_C() { MMU::Instance().Write(hl(), c_); } // 0x71
void CPU::OpLD_addrHL_D() { MMU::Instance().Write(hl(), d_); } // 0x72
void CPU::OpLD_addrHL_E() { MMU::Instance().Write(hl(), e_); } // 0x73
void CPU::OpLD_addrHL_H() { MMU::Instance().Write(hl(), h_); } // 0x74
void CPU::OpLD_addrHL_L() { MMU::Instance().Write(hl(), l_); } // 0x75
// 0x76 is HALT
void CPU::OpLD_addrHL_A() { MMU::Instance().Write(hl(), a_); } // 0x77
void CPU::OpLD_addrHL_d8() { MMU::Instance().Write(hl(), Fetch8()); } // 0x36

void CPU::OpLD_A_B() { a_ = b_; }         // 0x78
void CPU::OpLD_A_C() { a_ = c_; }         // 0x79
void CPU::OpLD_A_D() { a_ = d_; }         // 0x7A
void CPU::OpLD_A_E() { a_ = e_; }         // 0x7B
void CPU::OpLD_A_H() { a_ = h_; }         // 0x7C
void CPU::OpLD_A_L() { a_ = l_; }         // 0x7D
void CPU::OpLD_A_addrHL() { a_ = MMU::Instance().Read(hl()); } // 0x7E
void CPU::OpLD_A_A() { /* NOP */ }         // 0x7F

void CPU::OpLD_A_addrBC() { a_ = MMU::Instance().Read((static_cast<uint16_t>(b_) << 8) | c_); } // 0x0A
void CPU::OpLD_A_addrDE() { a_ = MMU::Instance().Read((static_cast<uint16_t>(d_) << 8) | e_); } // 0x1A
void CPU::OpLD_A_addrHLinc() { a_ = MMU::Instance().Read(hl()); set_hl(hl() + 1); } // 0x2A, LD A, (HL+)
void CPU::OpLD_A_addrHLdec() { a_ = MMU::Instance().Read(hl()); set_hl(hl() - 1); } // 0x3A, LD A, (HL-)

void CPU::OpLD_addrBC_A() { MMU::Instance().Write((static_cast<uint16_t>(b_) << 8) | c_, a_); } // 0x02
void CPU::OpLD_addrDE_A() { MMU::Instance().Write((static_cast<uint16_t>(d_) << 8) | e_, a_); } // 0x12
void CPU::OpLD_addrHLinc_A() { MMU::Instance().Write(hl(), a_); set_hl(hl() + 1); } // 0x22, LD (HL+), A
void CPU::OpLD_addrHLdec_A() { MMU::Instance().Write(hl(), a_); set_hl(hl() - 1); } // 0x32, LD (HL-), A

void CPU::OpLD_A_a16() { uint16_t addr = Fetch16(); a_ = MMU::Instance().Read(addr); } // 0xFA
void CPU::OpLD_a16_A() { uint16_t addr = Fetch16(); MMU::Instance().Write(addr, a_); } // 0xEA

// High RAM (LDH)
void CPU::OpLDH_A_a8() { uint16_t addr = 0xFF00 + Fetch8(); a_ = MMU::Instance().Read(addr); } // 0xF0
void CPU::OpLDH_a8_A() { uint16_t addr = 0xFF00 + Fetch8(); MMU::Instance().Write(addr, a_); } // 0xE0
void CPU::OpLD_A_addrC() { uint16_t addr = 0xFF00 + c_; a_ = MMU::Instance().Read(addr); } // 0xF2
void CPU::OpLD_addrC_A() { uint16_t addr = 0xFF00 + c_; MMU::Instance().Write(addr, a_); } // 0xE2

// 16-bit Loads
void CPU::OpLD_BC_d16() { uint16_t val = Fetch16(); b_ = static_cast<uint8_t>(val >> 8); c_ = static_cast<uint8_t>(val & 0xFF); } // 0x01
void CPU::OpLD_DE_d16() { uint16_t val = Fetch16(); d_ = static_cast<uint8_t>(val >> 8); e_ = static_cast<uint8_t>(val & 0xFF); } // 0x11
void CPU::OpLD_HL_d16() { set_hl(Fetch16()); } // 0x21
void CPU::OpLD_SP_d16() { sp_ = Fetch16(); } // 0x31

void CPU::OpLD_SP_HL() { sp_ = hl(); } // 0xF9

void CPU::OpLD_HL_SPplusr8() { // 0xF8
  int8_t offset = static_cast<int8_t>(Fetch8());
  uint16_t current_sp = sp_;
  uint16_t result = static_cast<uint16_t>(current_sp + offset);

  SetFlag(kZeroFlagMask, false);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, ((current_sp ^ offset ^ result) & 0x10) != 0);
  SetFlag(kCarryFlagMask, ((current_sp ^ offset ^ result) & 0x100) != 0);
  // More precise flag calculation based on common docs for LD HL, SP+r8
  //SetFlag(kHalfCarryFlagMask, ((current_sp & 0x0F) + (offset & 0x0F)) > 0x0F);
  //SetFlag(kCarryFlagMask, ((current_sp & 0xFF) + (offset & 0xFF)) > 0xFF);
  set_hl(result);
}

void CPU::OpLD_a16_SP() { // 0x08
  uint16_t addr = Fetch16();
  MMU::Instance().Write(addr, static_cast<uint8_t>(sp_ & 0xFF)); // Low byte
  MMU::Instance().Write(addr + 1, static_cast<uint8_t>(sp_ >> 8)); // High byte
}


// --- ALU Instructions ---

// 8-bit Arithmetic (INC, DEC)
void CPU::OpINC_B() { b_ = Inc8(b_); } // 0x04
void CPU::OpDEC_B() { b_ = Dec8(b_); } // 0x05
void CPU::OpINC_C() { c_ = Inc8(c_); } // 0x0C
void CPU::OpDEC_C() { c_ = Dec8(c_); } // 0x0D
void CPU::OpINC_D() { d_ = Inc8(d_); } // 0x14
void CPU::OpDEC_D() { d_ = Dec8(d_); } // 0x15
void CPU::OpINC_E() { e_ = Inc8(e_); } // 0x1C
void CPU::OpDEC_E() { e_ = Dec8(e_); } // 0x1D
void CPU::OpINC_H() { h_ = Inc8(h_); } // 0x24
void CPU::OpDEC_H() { h_ = Dec8(h_); } // 0x25
void CPU::OpINC_L() { l_ = Inc8(l_); } // 0x2C
void CPU::OpDEC_L() { l_ = Dec8(l_); } // 0x2D
void CPU::OpINC_A() { a_ = Inc8(a_); } // 0x3C
void CPU::OpDEC_A() { a_ = Dec8(a_); } // 0x3D

void CPU::OpINC_addrHL() { uint16_t addr = hl(); MMU::Instance().Write(addr, Inc8(MMU::Instance().Read(addr))); } // 0x34
void CPU::OpDEC_addrHL() { uint16_t addr = hl(); MMU::Instance().Write(addr, Dec8(MMU::Instance().Read(addr))); } // 0x35

// 8-bit Arithmetic (ADD, ADC, SUB, SBC)
void CPU::OpADD_A_B() { Add8(b_, false); } // 0x80
void CPU::OpADD_A_C() { Add8(c_, false); } // 0x81
void CPU::OpADD_A_D() { Add8(d_, false); } // 0x82
void CPU::OpADD_A_E() { Add8(e_, false); } // 0x83
void CPU::OpADD_A_H() { Add8(h_, false); } // 0x84
void CPU::OpADD_A_L() { Add8(l_, false); } // 0x85
void CPU::OpADD_A_addrHL() { Add8(MMU::Instance().Read(hl()), false); } // 0x86
void CPU::OpADD_A_A() { Add8(a_, false); } // 0x87
void CPU::OpADD_A_d8() { Add8(Fetch8(), false); } // 0xC6

void CPU::OpADC_A_B() { Add8(b_, true); } // 0x88
void CPU::OpADC_A_C() { Add8(c_, true); } // 0x89
void CPU::OpADC_A_D() { Add8(d_, true); } // 0x8A
void CPU::OpADC_A_E() { Add8(e_, true); } // 0x8B
void CPU::OpADC_A_H() { Add8(h_, true); } // 0x8C
void CPU::OpADC_A_L() { Add8(l_, true); } // 0x8D
void CPU::OpADC_A_addrHL() { Add8(MMU::Instance().Read(hl()), true); } // 0x8E
void CPU::OpADC_A_A() { Add8(a_, true); } // 0x8F
void CPU::OpADC_A_d8() { Add8(Fetch8(), true); } // 0xCE

void CPU::OpSUB_B() { Sub8(b_, false); } // 0x90
void CPU::OpSUB_C() { Sub8(c_, false); } // 0x91
void CPU::OpSUB_D() { Sub8(d_, false); } // 0x92
void CPU::OpSUB_E() { Sub8(e_, false); } // 0x93
void CPU::OpSUB_H() { Sub8(h_, false); } // 0x94
void CPU::OpSUB_L() { Sub8(l_, false); } // 0x95
void CPU::OpSUB_addrHL() { Sub8(MMU::Instance().Read(hl()), false); } // 0x96
void CPU::OpSUB_A() { Sub8(a_, false); } // 0x97
void CPU::OpSUB_d8() { Sub8(Fetch8(), false); } // 0xD6

void CPU::OpSBC_A_B() { Sub8(b_, true); } // 0x98
void CPU::OpSBC_A_C() { Sub8(c_, true); } // 0x99
void CPU::OpSBC_A_D() { Sub8(d_, true); } // 0x9A
void CPU::OpSBC_A_E() { Sub8(e_, true); } // 0x9B
void CPU::OpSBC_A_H() { Sub8(h_, true); } // 0x9C
void CPU::OpSBC_A_L() { Sub8(l_, true); } // 0x9D
void CPU::OpSBC_A_addrHL() { Sub8(MMU::Instance().Read(hl()), true); } // 0x9E
void CPU::OpSBC_A_A() { Sub8(a_, true); } // 0x9F
void CPU::OpSBC_A_d8() { Sub8(Fetch8(), true); } // 0xDE

// 8-bit Logic (AND, OR, XOR, CP)
void CPU::OpAND_B() { And8(b_); } // 0xA0
void CPU::OpAND_C() { And8(c_); } // 0xA1
void CPU::OpAND_D() { And8(d_); } // 0xA2
void CPU::OpAND_E() { And8(e_); } // 0xA3
void CPU::OpAND_H() { And8(h_); } // 0xA4
void CPU::OpAND_L() { And8(l_); } // 0xA5
void CPU::OpAND_addrHL() { And8(MMU::Instance().Read(hl())); } // 0xA6
void CPU::OpAND_A() { And8(a_); } // 0xA7
void CPU::OpAND_d8() { And8(Fetch8()); } // 0xE6

void CPU::OpXOR_B() { Xor8(b_); } // 0xA8
void CPU::OpXOR_C() { Xor8(c_); } // 0xA9
void CPU::OpXOR_D() { Xor8(d_); } // 0xAA
void CPU::OpXOR_E() { Xor8(e_); } // 0xAB
void CPU::OpXOR_H() { Xor8(h_); } // 0xAC
void CPU::OpXOR_L() { Xor8(l_); } // 0xAD
void CPU::OpXOR_addrHL() { Xor8(MMU::Instance().Read(hl())); } // 0xAE
void CPU::OpXOR_A() { Xor8(a_); } // 0xAF
void CPU::OpXOR_d8() { Xor8(Fetch8()); } // 0xEE

void CPU::OpOR_B() { Or8(b_); } // 0xB0
void CPU::OpOR_C() { Or8(c_); } // 0xB1
void CPU::OpOR_D() { Or8(d_); } // 0xB2
void CPU::OpOR_E() { Or8(e_); } // 0xB3
void CPU::OpOR_H() { Or8(h_); } // 0xB4
void CPU::OpOR_L() { Or8(l_); } // 0xB5
void CPU::OpOR_addrHL() { Or8(MMU::Instance().Read(hl())); } // 0xB6
void CPU::OpOR_A() { Or8(a_); } // 0xB7
void CPU::OpOR_d8() { Or8(Fetch8()); } // 0xF6

void CPU::OpCP_B() { Cp8(b_); } // 0xB8
void CPU::OpCP_C() { Cp8(c_); } // 0xB9
void CPU::OpCP_D() { Cp8(d_); } // 0xBA
void CPU::OpCP_E() { Cp8(e_); } // 0xBB
void CPU::OpCP_H() { Cp8(h_); } // 0xBC
void CPU::OpCP_L() { Cp8(l_); } // 0xBD
void CPU::OpCP_addrHL() { Cp8(MMU::Instance().Read(hl())); } // 0xBE
void CPU::OpCP_A() { Cp8(a_); } // 0xBF
void CPU::OpCP_d8() { Cp8(Fetch8()); } // 0xFE

// 16-bit Arithmetic
void CPU::OpINC_BC() { uint16_t bc = (static_cast<uint16_t>(b_) << 8) | c_; bc++; b_ = static_cast<uint8_t>(bc >> 8); c_ = static_cast<uint8_t>(bc & 0xFF); } // 0x03
void CPU::OpDEC_BC() { uint16_t bc = (static_cast<uint16_t>(b_) << 8) | c_; bc--; b_ = static_cast<uint8_t>(bc >> 8); c_ = static_cast<uint8_t>(bc & 0xFF); } // 0x0B
void CPU::OpINC_DE() { uint16_t de = (static_cast<uint16_t>(d_) << 8) | e_; de++; d_ = static_cast<uint8_t>(de >> 8); e_ = static_cast<uint8_t>(de & 0xFF); } // 0x13
void CPU::OpDEC_DE() { uint16_t de = (static_cast<uint16_t>(d_) << 8) | e_; de--; d_ = static_cast<uint8_t>(de >> 8); e_ = static_cast<uint8_t>(de & 0xFF); } // 0x1B
void CPU::OpINC_HL() { set_hl(hl() + 1); } // 0x23
void CPU::OpDEC_HL() { set_hl(hl() - 1); } // 0x2B
void CPU::OpINC_SP() { sp_++; } // 0x33
void CPU::OpDEC_SP() { sp_--; } // 0x3B

void CPU::OpADD_HL_BC() { // 0x09
  uint16_t val1 = hl();
  uint16_t val2 = (static_cast<uint16_t>(b_) << 8) | c_;
  uint32_t result = static_cast<uint32_t>(val1) + val2;
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, ((val1 & 0x0FFF) + (val2 & 0x0FFF)) > 0x0FFF);
  SetFlag(kCarryFlagMask, result > 0xFFFF);
  set_hl(static_cast<uint16_t>(result & 0xFFFF));
}
void CPU::OpADD_HL_DE() { // 0x19
  uint16_t val1 = hl();
  uint16_t val2 = (static_cast<uint16_t>(d_) << 8) | e_;
  uint32_t result = static_cast<uint32_t>(val1) + val2;
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, ((val1 & 0x0FFF) + (val2 & 0x0FFF)) > 0x0FFF);
  SetFlag(kCarryFlagMask, result > 0xFFFF);
  set_hl(static_cast<uint16_t>(result & 0xFFFF));
}
void CPU::OpADD_HL_HL() { // 0x29
  uint16_t val1 = hl();
  uint32_t result = static_cast<uint32_t>(val1) + val1;
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, ((val1 & 0x0FFF) + (val1 & 0x0FFF)) > 0x0FFF);
  SetFlag(kCarryFlagMask, result > 0xFFFF);
  set_hl(static_cast<uint16_t>(result & 0xFFFF));
}
void CPU::OpADD_HL_SP() { // 0x39
  uint16_t val1 = hl();
  uint16_t val2 = sp_;
  uint32_t result = static_cast<uint32_t>(val1) + val2;
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, ((val1 & 0x0FFF) + (val2 & 0x0FFF)) > 0x0FFF);
  SetFlag(kCarryFlagMask, result > 0xFFFF);
  set_hl(static_cast<uint16_t>(result & 0xFFFF));
}

void CPU::OpADD_SP_r8() { // 0xE8
  int8_t offset = static_cast<int8_t>(Fetch8());
  uint16_t current_sp = sp_;
  uint16_t result = static_cast<uint16_t>(current_sp + offset);

  SetFlag(kZeroFlagMask, false);
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, ((current_sp ^ offset ^ result) & 0x10) != 0);
  SetFlag(kCarryFlagMask, ((current_sp ^ offset ^ result) & 0x100) != 0);
  //SetFlag(kHalfCarryFlagMask, ((current_sp & 0x0F) + (offset & 0x0F)) > 0x0F);
  //SetFlag(kCarryFlagMask, ((current_sp & 0xFF) + (offset & 0xFF)) > 0xFF);
  sp_ = result;
}

// Miscellaneous ALU
void CPU::OpDAA() { // 0x27
  uint16_t correction = 0;
  bool carry = GetFlag(kCarryFlagMask);
  bool half_carry = GetFlag(kHalfCarryFlagMask);
  bool subtract = GetFlag(kSubtractFlagMask);
  uint8_t current_a = a_;

  if (!subtract) { // Addition
      if (carry || current_a > 0x99) { correction |= 0x60; SetFlag(kCarryFlagMask, true); }
      if (half_carry || (current_a & 0x0F) > 0x09) { correction |= 0x06; }
  } else { // Subtraction
      if (carry) { correction |= 0x60; /* Carry flag remains set */ }
      if (half_carry) { correction |= 0x06; }
  }

  a_ = current_a + (subtract ? -correction : correction);
  SetFlag(kZeroFlagMask, a_ == 0);
  SetFlag(kHalfCarryFlagMask, false); // Always reset
}

void CPU::OpCPL() { // 0x2F - Complement A
  a_ = ~a_;
  SetFlag(kSubtractFlagMask, true);
  SetFlag(kHalfCarryFlagMask, true);
  // Z, C flags not affected
}

void CPU::OpSCF() { // 0x37 - Set Carry Flag
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, true);
  // Z flag not affected
}

void CPU::OpCCF() { // 0x3F - Complement Carry Flag
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, !GetFlag(kCarryFlagMask));
  // Z flag not affected
}

// Rotates & Shifts (A Register only for non-CB)
void CPU::OpRLCA() { RlcA(); } // 0x07
void CPU::OpRRCA() { RrcA(); } // 0x0F
void CPU::OpRLA() { RlA(); }   // 0x17
void CPU::OpRRA() { RrA(); }   // 0x1F


// --- Control Flow Instructions ---

// Jumps (JP, JR)
void CPU::OpJP_a16() { pc_ = Fetch16(); } // 0xC3

void CPU::OpJP_HL() { pc_ = hl(); } // 0xE9

void CPU::OpJP_NZ_a16() { // 0xC2
  uint16_t addr = Fetch16();
  if (!GetFlag(kZeroFlagMask)) {
    pc_ = addr;
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpJP_Z_a16() { // 0xCA
  uint16_t addr = Fetch16();
  if (GetFlag(kZeroFlagMask)) {
    pc_ = addr;
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpJP_NC_a16() { // 0xD2
  uint16_t addr = Fetch16();
  if (!GetFlag(kCarryFlagMask)) {
    pc_ = addr;
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpJP_C_a16() { // 0xDA
  uint16_t addr = Fetch16();
  if (GetFlag(kCarryFlagMask)) {
    pc_ = addr;
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpJR_r8() { // 0x18
  int8_t offset = static_cast<int8_t>(Fetch8());
  pc_ = static_cast<uint16_t>(pc_ + offset);
}

void CPU::OpJR_NZ_r8() { // 0x20
  int8_t offset = static_cast<int8_t>(Fetch8());
  if (!GetFlag(kZeroFlagMask)) {
    pc_ = static_cast<uint16_t>(pc_ + offset);
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpJR_Z_r8() { // 0x28
  int8_t offset = static_cast<int8_t>(Fetch8());
  if (GetFlag(kZeroFlagMask)) {
    pc_ = static_cast<uint16_t>(pc_ + offset);
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpJR_NC_r8() { // 0x30
  int8_t offset = static_cast<int8_t>(Fetch8());
  if (!GetFlag(kCarryFlagMask)) {
    pc_ = static_cast<uint16_t>(pc_ + offset);
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpJR_C_r8() { // 0x38
  int8_t offset = static_cast<int8_t>(Fetch8());
  if (GetFlag(kCarryFlagMask)) {
    pc_ = static_cast<uint16_t>(pc_ + offset);
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

// Calls
void CPU::OpCALL_a16() { // 0xCD
  uint16_t addr = Fetch16();
  PushWord(pc_);
  pc_ = addr;
}

void CPU::OpCALL_NZ_a16() { // 0xC4
  uint16_t addr = Fetch16();
  if (!GetFlag(kZeroFlagMask)) {
    PushWord(pc_);
    pc_ = addr;
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpCALL_Z_a16() { // 0xCC
  uint16_t addr = Fetch16();
  if (GetFlag(kZeroFlagMask)) {
    PushWord(pc_);
    pc_ = addr;
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpCALL_NC_a16() { // 0xD4
  uint16_t addr = Fetch16();
  if (!GetFlag(kCarryFlagMask)) {
    PushWord(pc_);
    pc_ = addr;
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

void CPU::OpCALL_C_a16() { // 0xDC
  uint16_t addr = Fetch16();
  if (GetFlag(kCarryFlagMask)) {
    PushWord(pc_);
    pc_ = addr;
    // TODO: Cycle cost
  }
  // TODO: Cycle cost
}

// Restarts (RST)
void CPU::OpRST_00H() { PushWord(pc_); pc_ = 0x0000; } // 0xC7
void CPU::OpRST_08H() { PushWord(pc_); pc_ = 0x0008; } // 0xCF
void CPU::OpRST_10H() { PushWord(pc_); pc_ = 0x0010; } // 0xD7
void CPU::OpRST_18H() { PushWord(pc_); pc_ = 0x0018; } // 0xDF
void CPU::OpRST_20H() { PushWord(pc_); pc_ = 0x0020; } // 0xE7
void CPU::OpRST_28H() { PushWord(pc_); pc_ = 0x0028; } // 0xEF
void CPU::OpRST_30H() { PushWord(pc_); pc_ = 0x0030; } // 0xF7
void CPU::OpRST_38H() { PushWord(pc_); pc_ = 0x0038; } // 0xFF

// Returns (RET, RETI)
void CPU::OpRET() { pc_ = PopWord(); } // 0xC9

void CPU::OpRET_NZ() { // 0xC0
  // TODO: Add cycle cost
  if (!GetFlag(kZeroFlagMask)) {
    pc_ = PopWord();
    // TODO: Add cycle cost
  }
}

void CPU::OpRET_Z() { // 0xC8
  // TODO: Add cycle cost
  if (GetFlag(kZeroFlagMask)) {
    pc_ = PopWord();
    // TODO: Add cycle cost
  }
}

void CPU::OpRET_NC() { // 0xD0
  // TODO: Add cycle cost
  if (!GetFlag(kCarryFlagMask)) {
    pc_ = PopWord();
    // TODO: Add cycle cost
  }
}

void CPU::OpRET_C() { // 0xD8
  // TODO: Add cycle cost
  if (GetFlag(kCarryFlagMask)) {
    pc_ = PopWord();
    // TODO: Add cycle cost
  }
}

void CPU::OpRETI() { // 0xD9
  pc_ = PopWord();
  ime_ = true; // Enable interrupts immediately after returning
}


// --- Stack Instructions (PUSH, POP) ---

void CPU::OpPUSH_BC() { PushWord((static_cast<uint16_t>(b_) << 8) | c_); } // 0xC5
void CPU::OpPUSH_DE() { PushWord((static_cast<uint16_t>(d_) << 8) | e_); } // 0xD5
void CPU::OpPUSH_HL() { PushWord(hl()); } // 0xE5
void CPU::OpPUSH_AF() { PushWord((static_cast<uint16_t>(a_) << 8) | f_); } // 0xF5

void CPU::OpPOP_BC() { uint16_t val = PopWord(); b_ = static_cast<uint8_t>(val >> 8); c_ = static_cast<uint8_t>(val & 0xFF); } // 0xC1
void CPU::OpPOP_DE() { uint16_t val = PopWord(); d_ = static_cast<uint8_t>(val >> 8); e_ = static_cast<uint8_t>(val & 0xFF); } // 0xD1
void CPU::OpPOP_HL() { set_hl(PopWord()); } // 0xE1
void CPU::OpPOP_AF() { uint16_t val = PopWord(); a_ = static_cast<uint8_t>(val >> 8); f_ = static_cast<uint8_t>(val & 0xF0); } // 0xF1


// --- Miscellaneous Instructions ---

void CPU::OpNOP() {} // 0x00

void CPU::OpSTOP() { // 0x10
  // TODO: Implement proper STOP behavior (halts CPU&LCD until button press)
  // Consumes following 0x00 byte
  Fetch8();
  // For now, may behave like NOP or HALT depending on exact system state needed
}

void CPU::OpHALT() { // 0x76
  // TODO: Implement HALT behavior (pauses CPU until an interrupt occurs)
  // Requires interrupt handling system.
  // If IME=0 and IF&IE=0, HALT behavior is buggy on DMG.
}

void CPU::OpDI() { ime_ = false; } // 0xF3 - Disable Interrupts

void CPU::OpEI() { // 0xFB - Enable Interrupts
  // TODO: Implement delayed interrupt enable (after instruction following EI)
  ime_ = true; // Enable immediately for now
}

void CPU::OpPREFIX_CB() { // 0xCB
  // TODO: This should trigger CB prefix handling.
  // If CB instructions are not implemented, this is effectively an illegal/NOP.
  std::cerr << "Error: Encountered 0xCB prefix but CB instructions are not implemented." << std::endl;
}

// --- Undefined Opcodes ---
// Handled by the dispatch table mapping them (e.g., to OpNOP or OpPREFIX_CB as placeholder)
// 0xD3, 0xDB, 0xDD, 0xE3, 0xE4, 0xEB, 0xEC, 0xED, 0xF4, 0xFC, 0xFD

} // namespace gb