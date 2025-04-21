#pragma once

#include <cstdint>

namespace gb {

enum class Interrupt : uint8_t {
    VBlank = 0,
    LCDStat,
    Timer,
    Serial,
    Joypad
};

class CPU {
  public:
    CPU() = default;
    ~CPU() = default;

    // Initialize registers and state
    void Reset();

    // Advance the CPU by one instruction
    void Step();
  private:
    // Fetch next opcode from memory
    uint8_t FetchOpcode();

    // Decode & execute
    void Execute(uint8_t opcode);

    // 8-bit registers
    uint8_t a_, f_, b_, c_, d_, e_, h_, l_;

    // 16-bit special registers
    uint16_t sp_, pc_;

    // Interrupt master enable flag
    bool ime_ = false;
};

} // namespace gb
