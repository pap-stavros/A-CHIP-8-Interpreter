# CHIP-8

CHIP-8 emulator written in C++ with Raylib 🎮

## Requirements

- C++
- Raylib
- CMake

## Build & Run

```bash
# Install Raylib first if you haven't:
# Ubuntu/Debian:
sudo apt install libraylib-dev

# Clone and build
git clone https://github.com/[your-username]/chip8
cd chip8
cmake .
make

# Run with a ROM
./main ROMs/[romname].ch8
