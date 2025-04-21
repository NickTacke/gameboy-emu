#include "gb/mmu.h"

#include <gtest/gtest.h>
#include <vector>

namespace gb {

TEST(MMU, LoadAndReadROM) {
    std::vector<uint8_t> rom(0x8000);
    for (size_t i = 0; i < rom.size(); ++i) {
        rom[i] = static_cast<uint8_t>(i & 0xFF);
    }
    auto& mmu = gb::MMU::Instance();
    mmu.LoadROM(rom);

    // Bank0: addr 0x1234 -> rom[0x1234]
    EXPECT_EQ(mmu.Read(0x1234), 0x34);

    // BankN: addr 0x4000 + 0x0100 -> rom[0x4000 + 0x100]
    EXPECT_EQ(mmu.Read(0x4100), rom[0x4100]);
}

TEST(MMU, VRAMandWRAM) {
    auto& mmu = gb::MMU::Instance();
    mmu.Write(0x8000, 0xAA);
    EXPECT_EQ(mmu.Read(0x8000), 0xAA);

    mmu.Write(0xC000, 0xBB);
    EXPECT_EQ(mmu.Read(0xC000), 0xBB);

    // Echo region: write to 0xE000 maps to 0xC000
    mmu.Write(0xE000, 0xCC);
    EXPECT_EQ(mmu.Read(0xC000), 0xCC);
}

}  // namespace gb