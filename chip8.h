#ifndef CHIP8_H
#define CHIP8_H

#include <array>
#include <cstdint>
#include <string>
#include <SDL2/SDL.h>

// Constants defining memory size and screen dimensions
constexpr size_t MEM_SIZE = 4096;
constexpr size_t SWIDTH = 64; // screen width
constexpr size_t SHEIGHT = 32; // screen height
constexpr size_t NUM_REGISTERS = 16;

// Extern variables for memory, registers, stack, and screen
extern std::array<uint8_t, MEM_SIZE> memory;
extern std::array<uint8_t, NUM_REGISTERS> v_regs; // registers from v0 to vf
extern uint16_t i_reg; // i register (address register)
extern uint16_t pc; // program counter (starts at 0x200)
extern uint16_t sp; // stack pointer
extern std::array<uint16_t, 16> stack; // 16-level stack

// 64x32 screen buffer (using bool for on/off state)
extern std::array<std::array<bool, SWIDTH>, SHEIGHT> screen;

// SDL window and renderer for graphics
extern SDL_Window* window;
extern SDL_Renderer* renderer;

// Counter for tracking executed instructions
extern unsigned long long instruction_counter;

// Function declarations
uint16_t grab_opcode(); // Grabs the next opcode from memory
void render_screen(); // Renders the screen to the window
std::string current_time_ms(); // Gets current system time in milliseconds
void run_opcode(uint16_t opcode); // Executes the given opcode
bool init_SDL(); // Initializes SDL2 for rendering
bool load_chip8_file(const std::string& filepath); // Loads a CHIP-8 program from a file

#endif // CHIP8_H
