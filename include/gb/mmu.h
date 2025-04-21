#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace gb {

// Memory-map constants
static constexpr uint16_t kRomBank0Size = 0x4000;
static constexpr uint16_t kRomBankNSize = 0x4000;
static constexpr uint16_t kVramSize      = 0x2000;
static constexpr uint16_t kExtRamSize    = 0x2000;
static constexpr uint16_t kWram0Size     = 0x1000;
static constexpr uint16_t kWram1Size     = 0x1000;
static constexpr uint16_t kOamSize       = 0xA0;
static constexpr uint16_t kIoSize        = 0x80;
static constexpr uint16_t kHramSize      = 0x7F;

class MMU {
  public:
    // Get the singleton instance
    static MMU& Instance();

    // Load the entire ROM into bank0 + bankN
    void LoadROM(const std::vector<uint8_t>& rom_data);

    // Read a byte  at a specific address
    uint8_t Read(uint16_t address) const;

    // Write a byte to a specific address
    void Write(uint16_t address, uint8_t value);

  private:
    MMU();
    ~MMU() = default;

    // Non-copyable, non-movable
    MMU(const MMU&) = delete;
    MMU& operator=(const MMU&) = delete;

    // Raw memory regions
    std::array<uint8_t, kRomBank0Size> rom_bank0_;
    std::array<uint8_t, kRomBankNSize> rom_bank_n_;
    std::array<uint8_t, kVramSize>      vram_;
    std::array<uint8_t, kExtRamSize>    ext_ram_;
    std::array<uint8_t, kWram0Size>     wram0_;
    std::array<uint8_t, kWram1Size>     wram1_;
    std::array<uint8_t, kOamSize>       oam_;
    std::array<uint8_t, kIoSize>        io_regs_;
    std::array<uint8_t, kHramSize>      hram_;
    uint8_t                             interrupt_enable_ = 0;
};

}  // namespace gb
