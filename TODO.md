# TODO List

Below is a prioritized list of tasks to complete and improve the GameBoy Emulator project.

## 📖 Documentation

- [ ] Expand `README.md` with:
  - Project overview and emulator capabilities.
  - Prerequisites (SDL2, CMake, compiler versions).
  - Build and run instructions (CMake commands, loading ROM via CLI).
  - Usage examples and supported command-line options.
- [ ] Add architecture overview diagrams (CPU, MMU, PPU, timers).
- [ ] Include contribution guidelines and code style conventions.

## 🔧 Build & Tooling

- [ ] Configure a CI pipeline (e.g., GitHub Actions) to:
  - Build both `Debug` and `Release` configurations.
  - Run unit tests (`gameboy-emu-tests`).
  - Enforce code formatting with `clang-format` and linting with `clang-tidy`.
  - [ ] Create `.github/workflows/ci.yml` with build matrix for Debug/Release.
  - [ ] Add steps to install SDL2 dependencies (e.g., actions-setup-sdl2).
  - [ ] Add caching for CMake artifacts and dependencies.
- [ ] Enable Address and Undefined Behavior sanitizers in debug builds.
- [ ] Add code coverage reporting and badge.

## 🧮 Memory Management Unit (MMU)

- [ ] Implement full MBC support:
  - Complete MBC1 banking logic (high bits, banking modes).
  - Add support for MBC2, MBC3 (RTC), MBC5, etc.
  - [ ] Parse cartridge header to detect MBC type (0x0147).
  - [ ] Implement MBC1 ROM/RAM banking and mode switching.
  - [ ] Implement MBC2 banking and external RAM.
  - [ ] Implement MBC3 RTC latching and banking.
  - [ ] Implement MBC5 extended ROM/RAM banking.
  - [ ] Handle battery-backed SRAM read/write (.sav files).
  - [ ] Add unit tests for each MBC variant.
- [ ] Load and map the Game Boy BIOS before cartridge ROM.
- [ ] Support battery-backed SRAM (load/save .sav files).
- [ ] Add unit tests for bank switching, DMA, and other MMU behaviors.

## 🖥 CPU

- [x] Implement `FetchOpcode()` to read bytes from `MMU::Read()` and increment `PC` correctly.
- [x] Use `MMU::Read(pc_)` to fetch the opcode byte.
- [x] Increment `pc_` by 1 and correctly handle multi-byte immediates.
- [ ] Decode and execute all base opcodes (0x00–0xFF) in `Execute()`.
- [ ] Group and implement loads (`LD`), ALU ops (`ADD`, `SUB`, etc.), control flow, and stack instructions.
- [ ] Implement memory loads/stores (`LD (HL),r` and `LD r,(HL)`).
- [ ] Support CB-prefixed instructions (bit operations, rotates, shifts).
- [ ] After reading prefix `0xCB`, fetch next opcode and execute rot/shift (`RL`, `RR`, `RLC`, `RRC`) and bit ops (`BIT`, `RES`, `SET`).
- [ ] Handle CPU flags (Z, N, H, C) accurately for all operations.
- [ ] Implement interrupt handling:
  - Master enable (`IME`), `IF` and `IE` registers.
  - VBlank, LCDStat, Timer, Serial, Joypad interrupts.
  - [ ] Add `IF` (0xFF0F) and `IE` (0xFFFF) handling in MMU.
  - [ ] On `Step()`, check and service highest-priority pending interrupt.
  - [ ] Push `pc_` to stack and jump to the interrupt vector, clear `IF`.
- [ ] Write comprehensive unit tests for instruction timing, flag behavior, and interrupts.

## 🎨 PPU (Graphics)

- [ ] Implement the Game Boy PPU state machine (modes 0–3):
  - OAM scan, pixel transfer, HBlank, VBlank.
- [ ] Track cycle counts for each mode and transition accurately.
- [ ] Update `LY` (0xFF44) each scanline and compare to `LYC` (0xFF45) for STAT interrupt.
- [ ] Render background tiles and window using VRAM data.
- [ ] Render sprites with correct priority and palettes.
- [ ] Render background, window, and sprites into a pixel buffer.
- [ ] Integrate with SDL renderer in `main.cpp` to display frames at ~60 FPS.
- [ ] Add optional tests for PPU state transitions.

## ⏱ Timers & DMA

- [ ] Implement timer registers (`DIV`, `TIMA`, `TMA`, `TAC`) & cycle-based increments.
- [ ] Increment `DIV` every 256 CPU cycles.
- [ ] Control `TIMA` increments based on `TAC` settings (enable, frequency).
- [ ] On `TIMA` overflow, reload from `TMA` and trigger Timer interrupt.
- [ ] Raise timer interrupts and connect them to the CPU.
- [ ] Implement OAM DMA transfer on writes to 0xFF46.
- [ ] Add tests covering timer overflow and DMA correctness.

## 🎮 Input Handling

- [ ] Implement joypad register (0xFF00) read/write logic in MMU.
- [ ] Map SDL keyboard/controller events to GameBoy button bits.
- [ ] Add tests for joypad state updates.

## 🔊 Audio

- [ ] Implement all four Game Boy audio channels (square waves, wave, noise).
- [ ] Channel 1: sweep unit, envelope, frequency timer.
- [ ] Channel 2: envelope, frequency timer.
- [ ] Channel 3: wave pattern RAM and DAC output.
- [ ] Channel 4: LFSR noise generator and envelope.
- [ ] Mix channel outputs and send via SDL audio callback at correct sample rate.
- [ ] Mix and output audio via SDL audio callback.
- [ ] Add tests for envelope, sweep, and frequency behaviors.

## 🚀 Main Application & UX

- [ ] Parse command-line arguments to load ROM files and configure options.
- [ ] Implement pause, resume, step-by-step execution modes for debugging.
- [ ] Graceful shutdown and resource cleanup.

## ✅ Testing & Quality

- [ ] Increase unit test coverage across all modules.
- [ ] Add integration tests using known-good test ROMs (blargg tests).
- [ ] Enforce coding guidelines in CI (formatting, linting).

## 💡 Future Enhancements

- [ ] Save and load emulator state (save states).
- [ ] Add Color Game Boy (CGB) support: palettes, double-speed mode.
- [ ] Integrate a simple GUI for settings and state management.
- [ ] Explore performance optimizations (cycle-accurate timing, JIT).
