# CHIP-8

CHIP-8 emulator written in C++ with Raylib 🎮

## Requirements

- C++
- Raylib
- CMake

# Information
This emulator was developed and tested using this excellent CHIP-8 Test Suite:
(https://github.com/Timendus/chip8-test-suite?tab=readme-ov-file#beep-test)


Controls
Original   Keyboard
1 2 3 C    1 2 3 4
4 5 6 D -> Q W E R
7 8 9 E    A S D F
A 0 B F    Z X C V

### Future Goals
- [ ] GUI debugger
- [ ] Memory viewer
- [ ] Register display
- [ ] Configurable quirks menu
- [ ] ROM browser
- [ ] Custom key mapping

## Build & Run

```bash
# Install Raylib first if you haven't:
# Ubuntu/Debian:
sudo apt install libraylib-dev

# Arch
sudo pacman -S raylib

# Clone and build
git clone https://github.com/[your-username]/chip8
cd chip8
cmake .
make

