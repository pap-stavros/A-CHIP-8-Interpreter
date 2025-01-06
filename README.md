# CHIP-8
CHIP-8 emulator written in C++ with Raylib 🎮

## About
The CHIP-8 was an early platform for simple games and applications in the 1970s.<br>This emulator provides a modern implementation with cycle-accurate timing and original hardware quirks.

## Screenshot
[You could add a screenshot of a game running]

## Development
This emulator was developed and tested using this excellent [CHIP-8 Test Suite](https://github.com/Timendus/chip8-test-suite) by [Timendus](https://github.com/Timendus)

### Future Goals
- [ ] GUI Debugger
- [ ] ROM Browser/Manager
- [ ] Custom Key Mapping
- [ ] Adjustable CPU speed
- [ ] Configurable Quirks

## Built With
- [Timendus](https://github.com/Timendus) a comprehensive test suite.
- [Raylib](https://www.raylib.com/) a graphics and audio library.
## Requirements

- C++
- Raylib
- CMake

## Controls
```text
CHIP-8 Key    Keyboard
1 2 3 C       1 2 3 4
4 5 6 D   →   Q W E R
7 8 9 E       A S D F
A 0 B F       Z X C V

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

