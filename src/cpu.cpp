#include "gb/cpu.h"
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
    // TODO: Implement memory
    uint8_t opcode = 0x0;
    pc_++;
    return opcode;
}

void CPU::Execute(uint8_t opcode) {
    switch (opcode) {
        case 0x00: // NOP
            break;
        case 0x01: // LD BC, d16
            break;
        case 0x02: // LD (BC), A
            break;
        case 0x03: // INC BC
            break;
        case 0x04: // INC B
            break;
        default:
            std::cout << "Unknown opcode: " << std::hex << static_cast<int>(opcode) << std::endl;
            break;
    }
}

void CPU::Step() {
    uint8_t opcode = FetchOpcode();
    Execute(opcode);
}

} // namespace gb 