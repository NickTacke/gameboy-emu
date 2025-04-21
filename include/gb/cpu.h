#pragma once

#include <cstdint>

namespace gb {

enum class Interrupt : uint8_t { VBlank = 0, LCDStat, Timer, Serial, Joypad };

class CPU {
 public:
  CPU() = default;
  ~CPU() = default;

  // Initialize registers and state
  void Reset();

  // Advance the CPU by one instruction
  void Step();

  // Getters for test/inspection
  uint16_t pc() const { return pc_; }
  uint16_t sp() const { return sp_; }
  bool ime() const { return ime_; }

  uint8_t a() const { return a_; }
  uint8_t f() const { return f_; }
  uint8_t b() const { return b_; }
  uint8_t c() const { return c_; }
  uint8_t d() const { return d_; }
  uint8_t e() const { return e_; }
  uint8_t h() const { return h_; }
  uint8_t l() const { return l_; }

 private:
  // Fetch next opcode from memory
  uint8_t FetchOpcode();
  uint8_t Fetch8();
  uint16_t Fetch16();

  // Decode & execute
  void Execute(uint8_t opcode);

  // Per-opcode handlers (256 total)
#define OPCODE(name, code) void Op##name();
#include "gb/opcode_list.h"
#undef OPCODE

  // 8-bit registers
  uint8_t a_, f_, b_, c_, d_, e_, h_, l_;

  // 16-bit special registers
  uint16_t sp_, pc_;

  // Interrupt master enable flag
  bool ime_ = false;
};

} // namespace gb
