#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace gb {

// Memory-map constants
static constexpr uint16_t kBankSize = 0x4000;
static constexpr uint16_t kVramSize = 0x2000;
static constexpr uint16_t kExtRamSize = 0x2000;
static constexpr uint16_t kWram0Size = 0x1000;
static constexpr uint16_t kWram1Size = 0x1000;
static constexpr uint16_t kOamSize = 0xA0;
static constexpr uint16_t kIoSize = 0x80;
static constexpr uint16_t kHramSize = 0x7F;

class MMU {
 public:
  // Get the singleton instance
  static MMU& Instance();

  // Load the entire ROM into bank0 + bankN
  void LoadROM(const std::vector<uint8_t>& rom_data);

  // Read/write data from/to a specific address
  uint8_t Read(uint16_t address) const;
  void Write(uint16_t address, uint8_t value);

  // Reset all memory regions back to default values
  void Reset();

 private:
  MMU();
  ~MMU() = default;

  // Non-copyable, non-movable
  MMU(const MMU&) = delete;
  MMU& operator=(const MMU&) = delete;

  // Helpers for current bank indices
  uint8_t CurrentRomBank() const;
  uint8_t CurrentRamBank() const;

  // Raw memory regions
  std::vector<uint8_t> rom_; // Entire ROM image
  std::array<uint8_t, kVramSize> vram_;
  std::array<uint8_t, kExtRamSize> ext_ram_;
  std::array<uint8_t, kWram0Size> wram0_;
  std::array<uint8_t, kWram1Size> wram1_;
  std::array<uint8_t, kOamSize> oam_;
  std::array<uint8_t, kIoSize> io_regs_;
  std::array<uint8_t, kHramSize> hram_;
  uint8_t interrupt_enable_ = 0;

  // MBC1 registers
  bool    ram_enable_        = false;  // 0x0000–0x1FFF
  uint8_t rom_bank_low5_     = 1;      // 0x2000–0x3FFF (bits 0–4)
  uint8_t rom_bank_high2_    = 0;      // 0x4000–0x5FFF (bits 5–6)
  uint8_t banking_mode_      = 0;      // 0x6000–0x7FFF (0=ROM, 1=RAM)
};

} // namespace gb
