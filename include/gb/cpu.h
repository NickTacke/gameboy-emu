#pragma once

#include <cstdint>

namespace gb {

// Flag definitions
constexpr uint8_t kZeroFlagBit = 7;
constexpr uint8_t kSubtractFlagBit = 6;
constexpr uint8_t kHalfCarryFlagBit = 5;
constexpr uint8_t kCarryFlagBit = 4;

constexpr uint8_t kZeroFlagMask = 1 << kZeroFlagBit;
constexpr uint8_t kSubtractFlagMask = 1 << kSubtractFlagBit;
constexpr uint8_t kHalfCarryFlagMask = 1 << kHalfCarryFlagBit;
constexpr uint8_t kCarryFlagMask = 1 << kCarryFlagBit;

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

  // Helper to get HL register pair
  uint16_t hl() const { return (static_cast<uint16_t>(h_) << 8) | l_; }
  // Helper to set HL register pair
  void set_hl(uint16_t val) {
    h_ = static_cast<uint8_t>(val >> 8);
    l_ = static_cast<uint8_t>(val & 0xFF);
  }

  // Flag helpers
  void SetFlag(uint8_t flag_mask, bool set);
  bool GetFlag(uint8_t flag_mask) const;

  // Stack helpers
  void PushWord(uint16_t word);
  uint16_t PopWord();

  // Arithmetic helpers
  void Add8(uint8_t val, bool use_carry);
  void Sub8(uint8_t val, bool use_carry);
  uint8_t Inc8(uint8_t reg);
  uint8_t Dec8(uint8_t reg);

  // Logic helpers
  void And8(uint8_t val);
  void Or8(uint8_t val);
  void Xor8(uint8_t val);
  void Cp8(uint8_t val);

  // Rotate/Shift helpers
  void RlcA(); // RLCA
  void RrcA(); // RRCA
  void RlA();  // RLA
  void RrA();  // RRA

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

  // Handler for undefined opcodes
  void OpIllegal();
  // Check and service any pending interrupts
  void ServiceInterrupts();
  // Cycle counter (machine cycles)
  uint64_t cycles_ = 0;
  // Delayed interrupt enable flag (for EI instruction)
  bool ei_delay_ = false;
  // CPU halted or stopped state
  bool halted_ = false;

  // 8-bit registers
  uint8_t a_, f_, b_, c_, d_, e_, h_, l_;

  // 16-bit special registers
  uint16_t sp_, pc_;

  // Interrupt master enable flag
  bool ime_ = false;
};

} // namespace gb
