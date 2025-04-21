#include "gb/cpu.h"
#include "gb/mmu.h"

#include <iostream>
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

} // namespace gb