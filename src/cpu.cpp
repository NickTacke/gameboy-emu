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
  switch (opcode) {
    case 0x00: // NOP
      break;
    case 0x01: // LD BC, d16
      uint16_t d16 = Fetch16();
      b_ = d16 >> 8;
      c_ = d16 & 0xFF;
      break;
    case 0x02: // LD (BC), A
      break;
    case 0x03: // INC BC
      break;
    case 0x04: // INC B
      break;
    default:
      std::cout << "Unknown opcode: " << std::hex << static_cast<int>(opcode)
                << std::endl;
      break;
  }
}

void CPU::Step() {
  uint8_t opcode = FetchOpcode();
  Execute(opcode);
}

} // namespace gb