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

// Flag helpers implementation
void CPU::SetFlag(uint8_t flag_mask, bool set) {
  if (set) {
    f_ |= flag_mask;
  } else {
    f_ &= ~flag_mask;
  }
  // Lower 4 bits of F are always 0
  f_ &= 0xF0;
}

bool CPU::GetFlag(uint8_t flag_mask) const {
  return (f_ & flag_mask) != 0;
}

// Helper Implementations

// Stack
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

// 8-bit Arithmetic

// Handles ADD A, X and ADC A, X
void CPU::Add8(uint8_t val, bool use_carry) {
  uint8_t current_a = a_;
  uint8_t carry = (use_carry && GetFlag(kCarryFlagMask)) ? 1 : 0;
  uint16_t result = static_cast<uint16_t>(current_a) + val + carry;
  a_ = static_cast<uint8_t>(result & 0xFF);

  SetFlag(kZeroFlagMask, a_ == 0);
  SetFlag(kSubtractFlagMask, false);
  // Half carry: Check carry from bit 3 to bit 4
  SetFlag(kHalfCarryFlagMask,
          ((current_a & 0x0F) + (val & 0x0F) + carry) > 0x0F);
  SetFlag(kCarryFlagMask, result > 0xFF);
}

// Handles SUB A, X and SBC A, X
void CPU::Sub8(uint8_t val, bool use_carry) {
  uint8_t current_a = a_;
  uint8_t carry = (use_carry && GetFlag(kCarryFlagMask)) ? 1 : 0;
  uint16_t result = static_cast<uint16_t>(current_a) - val - carry;
  a_ = static_cast<uint8_t>(result & 0xFF);

  SetFlag(kZeroFlagMask, a_ == 0);
  SetFlag(kSubtractFlagMask, true);
  // Half carry (borrow): Check borrow from bit 4 to bit 3
  SetFlag(kHalfCarryFlagMask, (current_a & 0x0F) < ((val & 0x0F) + carry));
  SetFlag(kCarryFlagMask, result > 0xFF); // Carry flag means borrow here
}

// Handles INC r / INC (HL)
uint8_t CPU::Inc8(uint8_t reg) {
  uint8_t result = reg + 1;
  SetFlag(kZeroFlagMask, result == 0);
  SetFlag(kSubtractFlagMask, false);
  // Half carry: Check carry from bit 3 to bit 4
  SetFlag(kHalfCarryFlagMask, (reg & 0x0F) == 0x0F);
  // Carry flag is not affected by INC
  return result;
}

// Handles DEC r / DEC (HL)
uint8_t CPU::Dec8(uint8_t reg) {
  uint8_t result = reg - 1;
  SetFlag(kZeroFlagMask, result == 0);
  SetFlag(kSubtractFlagMask, true);
  // Half carry (borrow): Check borrow from bit 4
  SetFlag(kHalfCarryFlagMask, (reg & 0x0F) == 0x00);
  // Carry flag is not affected by DEC
  return result;
}

// 8-bit Logic

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

// Handles CP A, X (Compare)
void CPU::Cp8(uint8_t val) {
  uint8_t current_a = a_;
  uint16_t result = static_cast<uint16_t>(current_a) - val;
  // Flags are set as if SUB was performed, but A is not modified
  SetFlag(kZeroFlagMask, (result & 0xFF) == 0);
  SetFlag(kSubtractFlagMask, true);
  SetFlag(kHalfCarryFlagMask, (current_a & 0x0F) < (val & 0x0F));
  SetFlag(kCarryFlagMask, result > 0xFF);
}

// Rotate/Shift (A register)

void CPU::RlcA() { // RLCA
  uint8_t carry = (a_ & 0x80) >> 7; // Bit 7
  a_ = (a_ << 1) | carry;
  SetFlag(kZeroFlagMask, false); // Z is always 0
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, carry);
}

void CPU::RrcA() { // RRCA
  uint8_t carry = a_ & 0x01; // Bit 0
  a_ = (a_ >> 1) | (carry << 7);
  SetFlag(kZeroFlagMask, false); // Z is always 0
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, carry);
}

void CPU::RlA() { // RLA
  uint8_t old_carry = GetFlag(kCarryFlagMask) ? 1 : 0;
  uint8_t new_carry = (a_ & 0x80) >> 7;
  a_ = (a_ << 1) | old_carry;
  SetFlag(kZeroFlagMask, false); // Z is always 0
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, new_carry);
}

void CPU::RrA() { // RRA
  uint8_t old_carry = GetFlag(kCarryFlagMask) ? 1 : 0;
  uint8_t new_carry = a_ & 0x01;
  a_ = (a_ >> 1) | (old_carry << 7);
  SetFlag(kZeroFlagMask, false); // Z is always 0
  SetFlag(kSubtractFlagMask, false);
  SetFlag(kHalfCarryFlagMask, false);
  SetFlag(kCarryFlagMask, new_carry);
}

// Per-opcode handlers
void CPU::OpNOP() {}

void CPU::OpLD_BC_d16() {
  uint16_t val = Fetch16();
  b_ = static_cast<uint8_t>(val >> 8);
  c_ = static_cast<uint8_t>(val & 0xFF);
}

void CPU::OpLD_addrBC_A() { MMU::Instance().Write((static_cast<uint16_t>(b_) << 8) | c_, a_); }

void CPU::OpINC_BC() { 
    uint16_t bc = (static_cast<uint16_t>(b_) << 8) | c_;
    bc++;
    b_ = static_cast<uint8_t>(bc >> 8);
    c_ = static_cast<uint8_t>(bc & 0xFF);
    // No flags affected
}

void CPU::OpINC_B() { b_ = Inc8(b_); }

void CPU::OpDEC_B() { b_ = Dec8(b_); }

void CPU::OpLD_B_d8() { b_ = Fetch8(); }

void CPU::OpRLCA() { RlcA(); }

void CPU::OpLD_a16_SP() { 
    uint16_t addr = Fetch16();
    MMU::Instance().Write(addr, static_cast<uint8_t>(sp_ & 0xFF)); // Low byte
    MMU::Instance().Write(addr + 1, static_cast<uint8_t>(sp_ >> 8)); // High byte
}

void CPU::OpADD_HL_BC() {
    uint16_t val1 = hl();
    uint16_t val2 = (static_cast<uint16_t>(b_) << 8) | c_;
    uint32_t result = static_cast<uint32_t>(val1) + val2;
    SetFlag(kSubtractFlagMask, false);
    SetFlag(kHalfCarryFlagMask, ((val1 & 0x0FFF) + (val2 & 0x0FFF)) > 0x0FFF);
    SetFlag(kCarryFlagMask, result > 0xFFFF);
    set_hl(static_cast<uint16_t>(result & 0xFFFF));
}

void CPU::OpLD_A_addrBC() { a_ = MMU::Instance().Read((static_cast<uint16_t>(b_) << 8) | c_); }

void CPU::OpDEC_BC() { 
    uint16_t bc = (static_cast<uint16_t>(b_) << 8) | c_;
    bc--;
    b_ = static_cast<uint8_t>(bc >> 8);
    c_ = static_cast<uint8_t>(bc & 0xFF);
    // No flags affected
}

void CPU::OpINC_C() { c_ = Inc8(c_); }

void CPU::OpDEC_C() { c_ = Dec8(c_); }

void CPU::OpLD_C_d8() { c_ = Fetch8(); }

void CPU::OpRRCA() { RrcA(); }

void CPU::OpSTOP() { 
    // TODO: Implement STOP behavior (halt until button press) - For now, treat as NOP? Needs PPU/Input state.
    // It also consumes an extra byte (0x00) that should be skipped.
    Fetch8(); // Skip the 0x00 byte
}

void CPU::OpLD_DE_d16() { 
    uint16_t val = Fetch16();
    d_ = static_cast<uint8_t>(val >> 8);
    e_ = static_cast<uint8_t>(val & 0xFF);
}

void CPU::OpLD_addrDE_A() { MMU::Instance().Write((static_cast<uint16_t>(d_) << 8) | e_, a_); }

void CPU::OpINC_DE() { 
    uint16_t de = (static_cast<uint16_t>(d_) << 8) | e_;
    de++;
    d_ = static_cast<uint8_t>(de >> 8);
    e_ = static_cast<uint8_t>(de & 0xFF);
    // No flags affected
}

void CPU::OpINC_D() { d_ = Inc8(d_); }

void CPU::OpDEC_D() { d_ = Dec8(d_); }

void CPU::OpLD_D_d8() { d_ = Fetch8(); }

void CPU::OpRLA() { RlA(); }

void CPU::OpJR_r8() { 
    int8_t offset = static_cast<int8_t>(Fetch8());
    pc_ = static_cast<uint16_t>(pc_ + offset);
}

void CPU::OpADD_HL_DE() {
    uint16_t val1 = hl();
    uint16_t val2 = (static_cast<uint16_t>(d_) << 8) | e_;
    uint32_t result = static_cast<uint32_t>(val1) + val2;
    SetFlag(kSubtractFlagMask, false);
    SetFlag(kHalfCarryFlagMask, ((val1 & 0x0FFF) + (val2 & 0x0FFF)) > 0x0FFF);
    SetFlag(kCarryFlagMask, result > 0xFFFF);
    set_hl(static_cast<uint16_t>(result & 0xFFFF));
}

void CPU::OpLD_A_addrDE() { a_ = MMU::Instance().Read((static_cast<uint16_t>(d_) << 8) | e_); }

void CPU::OpDEC_DE() { 
    uint16_t de = (static_cast<uint16_t>(d_) << 8) | e_;
    de--;
    d_ = static_cast<uint8_t>(de >> 8);
    e_ = static_cast<uint8_t>(de & 0xFF);
    // No flags affected
}

void CPU::OpINC_E() { e_ = Inc8(e_); }

void CPU::OpDEC_E() { e_ = Dec8(e_); }

void CPU::OpLD_E_d8() { e_ = Fetch8(); }

void CPU::OpRRA() { RrA(); }

void CPU::OpJR_NZ_r8() { 
    int8_t offset = static_cast<int8_t>(Fetch8());
    if (!GetFlag(kZeroFlagMask)) {
        pc_ = static_cast<uint16_t>(pc_ + offset);
        // TODO: Add cycle cost for taken jump
    }
    // TODO: Add cycle cost for not-taken jump
}

void CPU::OpLD_HL_d16() { 
    uint16_t val = Fetch16();
    set_hl(val);
}

void CPU::OpLD_addrHLinc_A() { 
    MMU::Instance().Write(hl(), a_); 
    set_hl(hl() + 1); 
}

void CPU::OpINC_HL() { 
    uint16_t hl_val = hl();
    hl_val++;
    set_hl(hl_val);
    // No flags affected
}

void CPU::OpINC_H() { h_ = Inc8(h_); }

void CPU::OpDEC_H() { h_ = Dec8(h_); }

void CPU::OpLD_H_d8() { h_ = Fetch8(); }

void CPU::OpDAA() {
    uint16_t correction = 0;
    bool carry = GetFlag(kCarryFlagMask);
    bool half_carry = GetFlag(kHalfCarryFlagMask);
    bool subtract = GetFlag(kSubtractFlagMask);
    uint8_t current_a = a_;

    if (!subtract) { // After addition
        if (carry || current_a > 0x99) {
            correction |= 0x60;
            SetFlag(kCarryFlagMask, true);
        }
        if (half_carry || (current_a & 0x0F) > 0x09) {
            correction |= 0x06;
        }
    } else { // After subtraction
        if (carry) {
            correction |= 0x60;
            // Carry flag remains set
        }
        if (half_carry) {
            correction |= 0x06;
        }
    }

    a_ = current_a + (subtract ? -correction : correction);
    SetFlag(kZeroFlagMask, a_ == 0);
    SetFlag(kHalfCarryFlagMask, false); // Always reset
}

void CPU::OpJR_Z_r8() { 
    int8_t offset = static_cast<int8_t>(Fetch8());
    if (GetFlag(kZeroFlagMask)) {
        pc_ = static_cast<uint16_t>(pc_ + offset);
        // TODO: Add cycle cost for taken jump
    }
    // TODO: Add cycle cost for not-taken jump
}

void CPU::OpADD_HL_HL() {
    uint16_t val1 = hl();
    uint32_t result = static_cast<uint32_t>(val1) + val1; // Add HL to itself
    SetFlag(kSubtractFlagMask, false);
    SetFlag(kHalfCarryFlagMask, ((val1 & 0x0FFF) + (val1 & 0x0FFF)) > 0x0FFF);
    SetFlag(kCarryFlagMask, result > 0xFFFF);
    set_hl(static_cast<uint16_t>(result & 0xFFFF));
}

void CPU::OpLD_A_addrHLinc() { 
    a_ = MMU::Instance().Read(hl()); 
    set_hl(hl() + 1); 
}

void CPU::OpDEC_HL() { 
    uint16_t hl_val = hl();
    hl_val--;
    set_hl(hl_val);
    // No flags affected
}

void CPU::OpINC_L() { l_ = Inc8(l_); }

void CPU::OpDEC_L() { l_ = Dec8(l_); }

void CPU::OpLD_L_d8() { l_ = Fetch8(); }

void CPU::OpCPL() { 
    a_ = ~a_;
    SetFlag(kSubtractFlagMask, true);
    SetFlag(kHalfCarryFlagMask, true);
    // Z and C flags are not affected
}

void CPU::OpJR_NC_r8() { 
    int8_t offset = static_cast<int8_t>(Fetch8());
    if (!GetFlag(kCarryFlagMask)) {
        pc_ = static_cast<uint16_t>(pc_ + offset);
        // TODO: Add cycle cost for taken jump
    }
    // TODO: Add cycle cost for not-taken jump
}

void CPU::OpLD_SP_d16() { sp_ = Fetch16(); }

void CPU::OpLD_addrHLdec_A() { 
    MMU::Instance().Write(hl(), a_); 
    set_hl(hl() - 1); 
}

void CPU::OpINC_SP() { 
    sp_++;
    // No flags affected
}

void CPU::OpINC_addrHL() { 
    uint16_t addr = hl();
    uint8_t val = MMU::Instance().Read(addr);
    val = Inc8(val);
    MMU::Instance().Write(addr, val);
}

void CPU::OpDEC_addrHL() { 
    uint16_t addr = hl();
    uint8_t val = MMU::Instance().Read(addr);
    val = Dec8(val);
    MMU::Instance().Write(addr, val);
}

void CPU::OpLD_addrHL_d8() { MMU::Instance().Write(hl(), Fetch8()); }

void CPU::OpSCF() { 
    SetFlag(kSubtractFlagMask, false);
    SetFlag(kHalfCarryFlagMask, false);
    SetFlag(kCarryFlagMask, true);
    // Z flag is not affected
}

void CPU::OpJR_C_r8() { 
    int8_t offset = static_cast<int8_t>(Fetch8());
    if (GetFlag(kCarryFlagMask)) {
        pc_ = static_cast<uint16_t>(pc_ + offset);
        // TODO: Add cycle cost for taken jump
    }
    // TODO: Add cycle cost for not-taken jump
}

void CPU::OpADD_HL_SP() {
    uint16_t val1 = hl();
    uint16_t val2 = sp_;
    uint32_t result = static_cast<uint32_t>(val1) + val2;
    SetFlag(kSubtractFlagMask, false);
    SetFlag(kHalfCarryFlagMask, ((val1 & 0x0FFF) + (val2 & 0x0FFF)) > 0x0FFF);
    SetFlag(kCarryFlagMask, result > 0xFFFF);
    set_hl(static_cast<uint16_t>(result & 0xFFFF));
}

void CPU::OpLD_A_addrHLdec() { 
    a_ = MMU::Instance().Read(hl()); 
    set_hl(hl() - 1); 
}

void CPU::OpDEC_SP() { 
    sp_--;
    // No flags affected
}

void CPU::OpINC_A() { a_ = Inc8(a_); }

void CPU::OpDEC_A() { a_ = Dec8(a_); }

void CPU::OpLD_A_d8() { a_ = Fetch8(); }

void CPU::OpCCF() { 
    SetFlag(kSubtractFlagMask, false);
    SetFlag(kHalfCarryFlagMask, false);
    SetFlag(kCarryFlagMask, !GetFlag(kCarryFlagMask)); // Flip Carry flag
    // Z flag is not affected
}

// LD r, r' (0x40 - 0x7F, excluding 0x76 HALT)
void CPU::OpLD_B_B() { /* NOP */ }
void CPU::OpLD_B_C() { b_ = c_; }
void CPU::OpLD_B_D() { b_ = d_; }
void CPU::OpLD_B_E() { b_ = e_; }
void CPU::OpLD_B_H() { b_ = h_; }
void CPU::OpLD_B_L() { b_ = l_; }
void CPU::OpLD_B_addrHL() { b_ = MMU::Instance().Read(hl()); }
void CPU::OpLD_B_A() { b_ = a_; }
void CPU::OpLD_C_B() { c_ = b_; }
void CPU::OpLD_C_C() { /* NOP */ }
void CPU::OpLD_C_D() { c_ = d_; }
void CPU::OpLD_C_E() { c_ = e_; }
void CPU::OpLD_C_H() { c_ = h_; }
void CPU::OpLD_C_L() { c_ = l_; }
void CPU::OpLD_C_addrHL() { c_ = MMU::Instance().Read(hl()); }
void CPU::OpLD_C_A() { c_ = a_; }
void CPU::OpLD_D_B() { d_ = b_; }
void CPU::OpLD_D_C() { d_ = c_; }
void CPU::OpLD_D_D() { /* NOP */ }
void CPU::OpLD_D_E() { d_ = e_; }
void CPU::OpLD_D_H() { d_ = h_; }
void CPU::OpLD_D_L() { d_ = l_; }
void CPU::OpLD_D_addrHL() { d_ = MMU::Instance().Read(hl()); }
void CPU::OpLD_D_A() { d_ = a_; }
void CPU::OpLD_E_B() { e_ = b_; }
void CPU::OpLD_E_C() { e_ = c_; }
void CPU::OpLD_E_D() { e_ = d_; }
void CPU::OpLD_E_E() { /* NOP */ }
void CPU::OpLD_E_H() { e_ = h_; }
void CPU::OpLD_E_L() { e_ = l_; }
void CPU::OpLD_E_addrHL() { e_ = MMU::Instance().Read(hl()); }
void CPU::OpLD_E_A() { e_ = a_; }
void CPU::OpLD_H_B() { h_ = b_; }
void CPU::OpLD_H_C() { h_ = c_; }
void CPU::OpLD_H_D() { h_ = d_; }
void CPU::OpLD_H_E() { h_ = e_; }
void CPU::OpLD_H_H() { /* NOP */ }
void CPU::OpLD_H_L() { h_ = l_; }
void CPU::OpLD_H_addrHL() { h_ = MMU::Instance().Read(hl()); }
void CPU::OpLD_H_A() { h_ = a_; }
void CPU::OpLD_L_B() { l_ = b_; }
void CPU::OpLD_L_C() { l_ = c_; }
void CPU::OpLD_L_D() { l_ = d_; }
void CPU::OpLD_L_E() { l_ = e_; }
void CPU::OpLD_L_H() { l_ = h_; }
void CPU::OpLD_L_L() { /* NOP */ }
void CPU::OpLD_L_addrHL() { l_ = MMU::Instance().Read(hl()); }
void CPU::OpLD_L_A() { l_ = a_; }
void CPU::OpLD_addrHL_B() { MMU::Instance().Write(hl(), b_); }
void CPU::OpLD_addrHL_C() { MMU::Instance().Write(hl(), c_); }
void CPU::OpLD_addrHL_D() { MMU::Instance().Write(hl(), d_); }
void CPU::OpLD_addrHL_E() { MMU::Instance().Write(hl(), e_); }
void CPU::OpLD_addrHL_H() { MMU::Instance().Write(hl(), h_); }
void CPU::OpLD_addrHL_L() { MMU::Instance().Write(hl(), l_); }
void CPU::OpHALT() { 
    // TODO: Implement HALT behavior (pause until interrupt)
    // Needs interrupt handling system.
    // For now, might just loop or do nothing, potentially incorrectly.
}
void CPU::OpLD_addrHL_A() { MMU::Instance().Write(hl(), a_); }
void CPU::OpLD_A_B() { a_ = b_; }
void CPU::OpLD_A_C() { a_ = c_; }
void CPU::OpLD_A_D() { a_ = d_; }
void CPU::OpLD_A_E() { a_ = e_; }
void CPU::OpLD_A_H() { a_ = h_; }
void CPU::OpLD_A_L() { a_ = l_; }
void CPU::OpLD_A_addrHL() { a_ = MMU::Instance().Read(hl()); }
void CPU::OpLD_A_A() { /* NOP */ }

// ADD A, X (0x80 - 0x87)
void CPU::OpADD_A_B() { Add8(b_, false); }
void CPU::OpADD_A_C() { Add8(c_, false); }
void CPU::OpADD_A_D() { Add8(d_, false); }
void CPU::OpADD_A_E() { Add8(e_, false); }
void CPU::OpADD_A_H() { Add8(h_, false); }
void CPU::OpADD_A_L() { Add8(l_, false); }
void CPU::OpADD_A_addrHL() { Add8(MMU::Instance().Read(hl()), false); }
void CPU::OpADD_A_A() { Add8(a_, false); }

// ADC A, X (0x88 - 0x8F)
void CPU::OpADC_A_B() { Add8(b_, true); }
void CPU::OpADC_A_C() { Add8(c_, true); }
void CPU::OpADC_A_D() { Add8(d_, true); }
void CPU::OpADC_A_E() { Add8(e_, true); }
void CPU::OpADC_A_H() { Add8(h_, true); }
void CPU::OpADC_A_L() { Add8(l_, true); }
void CPU::OpADC_A_addrHL() { Add8(MMU::Instance().Read(hl()), true); }
void CPU::OpADC_A_A() { Add8(a_, true); }

// SUB A, X (0x90 - 0x97)
void CPU::OpSUB_B() { Sub8(b_, false); }
void CPU::OpSUB_C() { Sub8(c_, false); }
void CPU::OpSUB_D() { Sub8(d_, false); }
void CPU::OpSUB_E() { Sub8(e_, false); }
void CPU::OpSUB_H() { Sub8(h_, false); }
void CPU::OpSUB_L() { Sub8(l_, false); }
void CPU::OpSUB_addrHL() { Sub8(MMU::Instance().Read(hl()), false); }
void CPU::OpSUB_A() { Sub8(a_, false); } // Result is 0, sets Z flag

// SBC A, X (0x98 - 0x9F)
void CPU::OpSBC_A_B() { Sub8(b_, true); }
void CPU::OpSBC_A_C() { Sub8(c_, true); }
void CPU::OpSBC_A_D() { Sub8(d_, true); }
void CPU::OpSBC_A_E() { Sub8(e_, true); }
void CPU::OpSBC_A_H() { Sub8(h_, true); }
void CPU::OpSBC_A_L() { Sub8(l_, true); }
void CPU::OpSBC_A_addrHL() { Sub8(MMU::Instance().Read(hl()), true); }
void CPU::OpSBC_A_A() { Sub8(a_, true); }

// AND A, X (0xA0 - 0xA7)
void CPU::OpAND_B() { And8(b_); }
void CPU::OpAND_C() { And8(c_); }
void CPU::OpAND_D() { And8(d_); }
void CPU::OpAND_E() { And8(e_); }
void CPU::OpAND_H() { And8(h_); }
void CPU::OpAND_L() { And8(l_); }
void CPU::OpAND_addrHL() { And8(MMU::Instance().Read(hl())); }
void CPU::OpAND_A() { And8(a_); }

// XOR A, X (0xA8 - 0xAF)
void CPU::OpXOR_B() { Xor8(b_); }
void CPU::OpXOR_C() { Xor8(c_); }
void CPU::OpXOR_D() { Xor8(d_); }
void CPU::OpXOR_E() { Xor8(e_); }
void CPU::OpXOR_H() { Xor8(h_); }
void CPU::OpXOR_L() { Xor8(l_); }
void CPU::OpXOR_addrHL() { Xor8(MMU::Instance().Read(hl())); }
void CPU::OpXOR_A() { Xor8(a_); } // Result is 0, sets Z flag

// OR A, X (0xB0 - 0xB7)
void CPU::OpOR_B() { Or8(b_); }
void CPU::OpOR_C() { Or8(c_); }
void CPU::OpOR_D() { Or8(d_); }
void CPU::OpOR_E() { Or8(e_); }
void CPU::OpOR_H() { Or8(h_); }
void CPU::OpOR_L() { Or8(l_); }
void CPU::OpOR_addrHL() { Or8(MMU::Instance().Read(hl())); }
void CPU::OpOR_A() { Or8(a_); }

// CP A, X (0xB8 - 0xBF)
void CPU::OpCP_B() { Cp8(b_); }
void CPU::OpCP_C() { Cp8(c_); }
void CPU::OpCP_D() { Cp8(d_); }
void CPU::OpCP_E() { Cp8(e_); }
void CPU::OpCP_H() { Cp8(h_); }
void CPU::OpCP_L() { Cp8(l_); }
void CPU::OpCP_addrHL() { Cp8(MMU::Instance().Read(hl())); }
void CPU::OpCP_A() { Cp8(a_); } // Result is 0, sets Z flag

// Returns (0xC0, C8, D0, D8, C9, D9)
void CPU::OpRET_NZ() { 
    if (!GetFlag(kZeroFlagMask)) { 
        pc_ = PopWord(); 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}
void CPU::OpPOP_BC() { 
    uint16_t val = PopWord(); 
    b_ = static_cast<uint8_t>(val >> 8);
    c_ = static_cast<uint8_t>(val & 0xFF);
}

void CPU::OpJP_NZ_a16() { 
    uint16_t addr = Fetch16();
    if (!GetFlag(kZeroFlagMask)) { 
        pc_ = addr; 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

void CPU::OpJP_a16() { pc_ = Fetch16(); }

void CPU::OpCALL_NZ_a16() { 
    uint16_t addr = Fetch16();
    if (!GetFlag(kZeroFlagMask)) { 
        PushWord(pc_); 
        pc_ = addr; 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

void CPU::OpPUSH_BC() { PushWord((static_cast<uint16_t>(b_) << 8) | c_); }

void CPU::OpADD_A_d8() { Add8(Fetch8(), false); }

void CPU::OpRST_00H() { PushWord(pc_); pc_ = 0x0000; }

void CPU::OpRET_Z() { 
    if (GetFlag(kZeroFlagMask)) { 
        pc_ = PopWord(); 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

void CPU::OpRET() { pc_ = PopWord(); }

void CPU::OpJP_Z_a16() { 
    uint16_t addr = Fetch16();
    if (GetFlag(kZeroFlagMask)) { 
        pc_ = addr; 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

void CPU::OpPREFIX_CB() {
    // This opcode is just a prefix, the actual operation is handled
    // by fetching the next byte and using a (currently removed) CB dispatch table.
    // For now, it acts like an illegal opcode or NOP if CB handling isn't added.
    // We should ideally throw an error or log here if CB is not implemented.
    std::cerr << "Error: Encountered 0xCB prefix but CB instructions are not implemented." << std::endl;
    // Treat as NOP for now to avoid crashing, but this is incorrect.
}

void CPU::OpCALL_Z_a16() { 
    uint16_t addr = Fetch16();
    if (GetFlag(kZeroFlagMask)) { 
        PushWord(pc_); 
        pc_ = addr; 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

void CPU::OpCALL_a16() { 
    uint16_t addr = Fetch16();
    PushWord(pc_);
    pc_ = addr;
}

void CPU::OpADC_A_d8() { Add8(Fetch8(), true); }

void CPU::OpRST_08H() { PushWord(pc_); pc_ = 0x0008; }

void CPU::OpRET_NC() { 
    if (!GetFlag(kCarryFlagMask)) { 
        pc_ = PopWord(); 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

void CPU::OpPOP_DE() { 
    uint16_t val = PopWord(); 
    d_ = static_cast<uint8_t>(val >> 8);
    e_ = static_cast<uint8_t>(val & 0xFF);
}

void CPU::OpJP_NC_a16() { 
    uint16_t addr = Fetch16();
    if (!GetFlag(kCarryFlagMask)) { 
        pc_ = addr; 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

// Opcode 0xD3 Undefined - No handler needed if dispatch maps to NOP/error

void CPU::OpCALL_NC_a16() { 
    uint16_t addr = Fetch16();
    if (!GetFlag(kCarryFlagMask)) { 
        PushWord(pc_); 
        pc_ = addr; 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

void CPU::OpPUSH_DE() { PushWord((static_cast<uint16_t>(d_) << 8) | e_); }

void CPU::OpSUB_d8() { Sub8(Fetch8(), false); }

void CPU::OpRST_10H() { PushWord(pc_); pc_ = 0x0010; }

void CPU::OpRET_C() { 
    if (GetFlag(kCarryFlagMask)) { 
        pc_ = PopWord(); 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

void CPU::OpRETI() { 
    pc_ = PopWord(); 
    ime_ = true; // Enable interrupts after RETI
}

void CPU::OpJP_C_a16() { 
    uint16_t addr = Fetch16();
    if (GetFlag(kCarryFlagMask)) { 
        pc_ = addr; 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

// Opcode 0xDB Undefined

void CPU::OpCALL_C_a16() { 
    uint16_t addr = Fetch16();
    if (GetFlag(kCarryFlagMask)) { 
        PushWord(pc_); 
        pc_ = addr; 
        // TODO: Add cycle cost
    } 
    // TODO: Add cycle cost
}

// Opcode 0xDD Undefined

void CPU::OpSBC_A_d8() { Sub8(Fetch8(), true); }

void CPU::OpRST_18H() { PushWord(pc_); pc_ = 0x0018; }

void CPU::OpLDH_a8_A() { 
    uint16_t addr = 0xFF00 + Fetch8();
    MMU::Instance().Write(addr, a_);
}

void CPU::OpPOP_HL() { 
    uint16_t val = PopWord(); 
    set_hl(val);
}

void CPU::OpLD_addrC_A() { 
    uint16_t addr = 0xFF00 + c_;
    MMU::Instance().Write(addr, a_);
}

// Opcode 0xE3 Undefined
// Opcode 0xE4 Undefined

void CPU::OpPUSH_HL() { PushWord(hl()); }

void CPU::OpAND_d8() { And8(Fetch8()); }

void CPU::OpRST_20H() { PushWord(pc_); pc_ = 0x0020; }

void CPU::OpADD_SP_r8() { 
    int8_t offset = static_cast<int8_t>(Fetch8());
    uint16_t current_sp = sp_;
    uint16_t result = static_cast<uint16_t>(current_sp + offset);

    SetFlag(kZeroFlagMask, false); // Always 0
    SetFlag(kSubtractFlagMask, false); // Always 0
    // Check carry/half-carry on lower byte addition
    SetFlag(kHalfCarryFlagMask, ((current_sp & 0x0F) + (offset & 0x0F)) > 0x0F);
    SetFlag(kCarryFlagMask, ((current_sp & 0xFF) + (offset & 0xFF)) > 0xFF);
    sp_ = result;
}

void CPU::OpJP_HL() { pc_ = hl(); }

void CPU::OpLD_a16_A() { 
    uint16_t addr = Fetch16();
    MMU::Instance().Write(addr, a_);
}

// Opcode 0xEB Undefined
// Opcode 0xEC Undefined
// Opcode 0xED Undefined

void CPU::OpXOR_d8() { Xor8(Fetch8()); }

void CPU::OpRST_28H() { PushWord(pc_); pc_ = 0x0028; }

void CPU::OpLDH_A_a8() { 
    uint16_t addr = 0xFF00 + Fetch8();
    a_ = MMU::Instance().Read(addr);
}

void CPU::OpPOP_AF() { 
    uint16_t val = PopWord(); 
    a_ = static_cast<uint8_t>(val >> 8);
    f_ = static_cast<uint8_t>(val & 0xF0); // Lower 4 bits of F are always 0
}

void CPU::OpLD_A_addrC() { 
    uint16_t addr = 0xFF00 + c_;
    a_ = MMU::Instance().Read(addr);
}

void CPU::OpDI() { ime_ = false; }

// Opcode 0xF4 Undefined

void CPU::OpPUSH_AF() { PushWord((static_cast<uint16_t>(a_) << 8) | f_); }

void CPU::OpOR_d8() { Or8(Fetch8()); }

void CPU::OpRST_30H() { PushWord(pc_); pc_ = 0x0030; }

void CPU::OpLD_HL_SPplusr8() { 
    int8_t offset = static_cast<int8_t>(Fetch8());
    uint16_t current_sp = sp_;
    uint16_t result = static_cast<uint16_t>(current_sp + offset);

    SetFlag(kZeroFlagMask, false); // Always 0
    SetFlag(kSubtractFlagMask, false); // Always 0
    // Check carry/half-carry on lower byte addition
    SetFlag(kHalfCarryFlagMask, ((current_sp & 0x0F) + (offset & 0x0F)) > 0x0F);
    SetFlag(kCarryFlagMask, ((current_sp & 0xFF) + (offset & 0xFF)) > 0xFF);
    set_hl(result);
}

void CPU::OpLD_SP_HL() { sp_ = hl(); }

void CPU::OpLD_A_a16() { 
    uint16_t addr = Fetch16();
    a_ = MMU::Instance().Read(addr);
}

void CPU::OpEI() { 
    // Interrupts are enabled *after* the instruction following EI
    // TODO: Need a mechanism to delay IME activation by one instruction cycle.
    // For now, enable immediately (potential minor inaccuracy).
    ime_ = true; 
}

// Opcode 0xFC Undefined
// Opcode 0xFD Undefined

void CPU::OpCP_d8() { Cp8(Fetch8()); }

void CPU::OpRST_38H() { PushWord(pc_); pc_ = 0x0038; }

} // namespace gb