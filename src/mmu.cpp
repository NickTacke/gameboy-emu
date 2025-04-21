#include "gb/mmu.h"

#include <algorithm> // for std::copy
#include <cstring>   // for std::memcpy
#include <iostream>

namespace gb {

MMU& MMU::Instance() {
  static MMU instance;
  return instance;
}

MMU::MMU() {
  // Clear all memory regions
  vram_.fill(0);
  ext_ram_.fill(0);
  wram0_.fill(0);
  wram1_.fill(0);
  oam_.fill(0);
  io_regs_.fill(0);
  hram_.fill(0);
}

// Load ROM data into bank0 + bankN (TODO: MBC handling)
void MMU::LoadROM(const std::vector<uint8_t>& rom_data) {
  rom_ = rom_data;
  // Reset banking registers
  ram_enable_ = false;
  rom_bank_low5_ = 1;
  rom_bank_high2_ = 0;
  banking_mode_ = 0;
}

uint8_t MMU::CurrentRomBank() const {
  // Merge low5 + high2 bits
  uint8_t bank = (rom_bank_high2_ << 5) | (rom_bank_low5_ & 0x1F);
  if ((bank & 0x1F) == 0) bank |= 1;
  // Wrap around the actual number of banks
  uint8_t max_banks = static_cast<uint8_t>(rom_.size() / kBankSize);
  return bank % max_banks;
}

uint8_t MMU::CurrentRamBank() const {
  return (banking_mode_ == 0) ? 0 : (rom_bank_high2_ & 0x03);
}

uint8_t MMU::Read(uint16_t address) const {
  if (address < 0x4000) {
    // Bank 0 is always first 16K
    return (address < rom_.size()) ? rom_[address] : 0xFF;
  }
  if (address < 0x8000) {
    // Switchable ROM bank
    uint8_t bank = CurrentRomBank();
    size_t  idx  = bank * kBankSize + (address - kBankSize);
    return (idx < rom_.size()) ? rom_[idx] : 0xFF;
  }
  if (address < 0xA000) {
    return vram_[address - 0x8000];
  }
  if (address < 0xC000) {
    // External RAM (banked)
    if (!ram_enable_) return 0xFF;
    uint8_t rb = CurrentRamBank();
    size_t  off = rb * kExtRamSize + (address - 0xA000);
    return (off < ext_ram_.size()) ? ext_ram_[off] : 0xFF;
  }
  if (address < 0xD000) {
    return wram0_[address - 0xC000];
  }
  if (address < 0xE000) {
    return wram1_[address - 0xD000];
  }
  if (address < 0xFE00) {
    // Echo
    return Read(address - 0x2000);
  }
  if (address < 0xFEA0) {
    return oam_[address - 0xFE00];
  }
  if (address < 0xFF00) {
    return 0xFF;  // unusable
  }
  if (address < 0xFF80) {
    return io_regs_[address - 0xFF00];
  }
  if (address < 0xFFFF) {
    return hram_[address - 0xFF80];
  }
  return interrupt_enable_;
}

void MMU::Write(uint16_t address, uint8_t value) {
  if (address < 0x2000) {
    // RAM enable
    ram_enable_ = ((value & 0x0F) == 0x0A);
    return;
  }
  if (address < 0x4000) {
    // ROM bank low 5 bits
    rom_bank_low5_ = value & 0x1F;
    return;
  }
  if (address < 0x6000) {
    // ROM bank high 2 bits (or RAM bank in mode 1)
    rom_bank_high2_ = value & 0x03;
    return;
  }
  if (address < 0x8000) {
    // Banking mode select
    banking_mode_ = value & 0x01;
    return;
  }
  if (address < 0xA000) {
    // VRAM
    vram_[address - 0x8000] = value;
    return;
  }
  if (address < 0xC000) {
    // External RAM (banked)
    if (ram_enable_) {
      uint8_t rb = CurrentRamBank();
      size_t  off = rb * kExtRamSize + (address - 0xA000);
      if (off < ext_ram_.size()) {
        ext_ram_[off] = value;
      }
    }
    return;
  }
  if (address < 0xD000) {
    wram0_[address - 0xC000] = value;
    return;
  }
  if (address < 0xE000) {
    wram1_[address - 0xD000] = value;
    return;
  }
  if (address < 0xFE00) {
    // Echo
    Write(address - 0x2000, value);
    return;
  }
  if (address < 0xFEA0) {
    oam_[address - 0xFE00] = value;
    return;
  }
  if (address < 0xFF00) {
    // Unusable
    return;
  }
  if (address < 0xFF80) {
    // OAM DMA trigger?
    if (address == 0xFF46) {
      uint16_t src = static_cast<uint16_t>(value) << 8;
      for (int i = 0; i < kOamSize; ++i) {
        oam_[i] = Read(src + i);
      }
    }
    io_regs_[address - 0xFF00] = value;
    return;
  }
  if (address < 0xFFFF) {
    hram_[address - 0xFF80] = value;
    return;
  }
  interrupt_enable_ = value;
}

void MMU::Reset() {
  vram_.fill(0);
  ext_ram_.fill(0);
  wram0_.fill(0);
  wram1_.fill(0);
  oam_.fill(0);
  io_regs_.fill(0);
  hram_.fill(0);
  interrupt_enable_ = 0;
}

} // namespace gb
