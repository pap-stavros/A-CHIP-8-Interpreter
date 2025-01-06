## About
CHIP-8 emulator written in C++ with Raylib. <br>
This emulator/interpreter provides a modern implementation with cycle-accurate timing and original hardware quirks.
<br><br>
There's still work to be done with some of the interpreter logic, regarding the opcodes. <br>It's not passing all the flag tests.

## Controls
```
CHIP-8        Keyboard
1 2 3 C       1 2 3 4
4 5 6 D   →   Q W E R
7 8 9 E       A S D F
A 0 B F       Z X C V
```
### Future Goals
- [ ] GUI Debugger
- [ ] ROM Browser/Manager
- [ ] Custom Key Mapping
- [ ] Adjustable CPU speed
- [ ] Configurable Quirks

## Development
- This emulator was developed and tested using this excellent [CHIP-8 Test Suite](https://github.com/Timendus/chip8-test-suite) by [Timendus](https://github.com/Timendus)
- [Raylib](https://www.raylib.com/) is the video/audio library of choice.
## Requirements

- C++
- Raylib
- CMake
