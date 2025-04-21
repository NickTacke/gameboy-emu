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
    rom_bank0_.fill(0xFF);
    rom_bank_n_.fill(0xFF);
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
    size_t copy_size = std::min(rom_data.size(), rom_bank0_.size());
    std::memcpy(rom_bank0_.data(), rom_data.data(), copy_size);

    // Copy the rest of the ROM into bankN
    if (rom_data.size() > rom_bank0_.size()) {
        size_t n_size = std::min(rom_data.size() - rom_bank0_.size(), rom_bank_n_.size());
        std::memcpy(rom_bank_n_.data(), rom_data.data() + rom_bank0_.size(), n_size);
    }
}

// Read memory data from a specific address
uint8_t MMU::Read(uint16_t address) const {
    // Map memory regions
    if (address < 0x4000) {
        return rom_bank0_[address];
    } else if (address < 0x8000) {
        return rom_bank_n_[address - 0x4000];
    } else if (address < 0xA000) {
        return vram_[address - 0x8000];
    } else if (address < 0xC000) {
        return ext_ram_[address - 0xA000];
    } else if (address < 0xD000) {
        return wram0_[address - 0xC000];
    } else if (address < 0xE000) {
        return wram1_[address - 0xD000];
    } else if (address < 0xFE00) {
        // Echo of C000–DDFF
        return Read(address - 0x2000);
    } else if (address < 0xFEA0) {
        return oam_[address - 0xFE00];
    } else if (address < 0xFF00) {
        // Unusable
        return 0xFF;
    } else if (address < 0xFF80) {
        return io_regs_[address - 0xFF00];
    } else if (address < 0xFFFF) {
        return hram_[address - 0xFF80];
    } else {  // 0xFFFF
        return interrupt_enable_;
    }
}

// Write data to a specific address
void MMU::Write(uint16_t address, uint8_t value) {
    if (address < 0x4000 || address < 0x8000) {
        // ROM area: typically handled by MBC; ignore for now
    } else if (address < 0xA000) {
        vram_[address - 0x8000] = value;
    } else if (address < 0xC000) {
        ext_ram_[address - 0xA000] = value;
    } else if (address < 0xD000) {
        wram0_[address - 0xC000] = value;
    } else if (address < 0xE000) {
        wram1_[address - 0xD000] = value;
    } else if (address < 0xFE00) {
        // Echo
        Write(address - 0x2000, value);
    } else if (address < 0xFEA0) {
        oam_[address - 0xFE00] = value;
    } else if (address < 0xFF00) {
        // Unusable: ignore
    } else if (address < 0xFF80) {
        // Handle OAM DMA trigger at 0xFF46
        if (address == 0xFF46) {
            uint16_t source = static_cast<uint16_t>(value) << 8;
            for (uint16_t i = 0; i < kOamSize; ++i) {
                oam_[i] = Read(source + i);
            }
        }
        io_regs_[address - 0xFF00] = value;
    } else if (address < 0xFFFF) {
        hram_[address - 0xFF80] = value;
    } else {  // 0xFFFF
        interrupt_enable_ = value;
    }
}

}  // namespace gb
