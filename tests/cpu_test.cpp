#include "gb/cpu.h"

#include <gtest/gtest.h>

#include "gb/mmu.h"

namespace gb {
namespace {

class CpuTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Reset both MMU and CPU to a known state
    gb::MMU::Instance().Reset();
    cpu_.Reset();
  }

  CPU cpu_;
};

// Verify Reset() puts the CPU into the correct initial state.
TEST_F(CpuTest, ResetInitializesRegistersAndFlags) {
  EXPECT_EQ(cpu_.pc(), 0x0100);
  EXPECT_EQ(cpu_.sp(), 0xFFFE);
  EXPECT_FALSE(cpu_.ime());

  EXPECT_EQ(cpu_.a(), 0);
  EXPECT_EQ(cpu_.f(), 0);
  EXPECT_EQ(cpu_.b(), 0);
  EXPECT_EQ(cpu_.c(), 0);
  EXPECT_EQ(cpu_.d(), 0);
  EXPECT_EQ(cpu_.e(), 0);
  EXPECT_EQ(cpu_.h(), 0);
  EXPECT_EQ(cpu_.l(), 0);
}

// Writing a NOP (0x00) at 0x0100 and stepping should simply increment PC.
TEST_F(CpuTest, StepExecutesNopAndIncrementsPc) {
  auto& mmu = MMU::Instance();
  mmu.Write(0x0100, 0x00);

  uint16_t before = cpu_.pc();
  cpu_.Step();
  EXPECT_EQ(cpu_.pc(), before + 1);
}

// Even unknown opcodes should advance PC by 1 (and not crash).
TEST_F(CpuTest, StepUnknownOpcodeStillAdvancesPc) {
  auto& mmu = MMU::Instance();
  const uint8_t kInvalid = 0xFF;
  mmu.Write(0x0100, kInvalid);

  uint16_t before = cpu_.pc();
  EXPECT_NO_FATAL_FAILURE(cpu_.Step());
  EXPECT_EQ(cpu_.pc(), before + 1);
}

// You can add more opcode‐specific tests as you implement them.
// e.g. LD A, d8 (0x3E): load immediate into A, pc should advance by 2
TEST_F(CpuTest, LdAImmediateLoadsAndAdvancesPc) {
  auto& mmu = MMU::Instance();
  // Opcode 0x3E, immediate value 0x42
  mmu.Write(0x0100, 0x3E);
  mmu.Write(0x0101, 0x42);

  uint16_t before = cpu_.pc();
  cpu_.Step();

  EXPECT_EQ(cpu_.a(), 0x42);
  EXPECT_EQ(cpu_.pc(), before + 2);
}

} // namespace
} // namespace gb