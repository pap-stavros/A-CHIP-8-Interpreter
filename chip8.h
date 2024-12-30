#ifndef CHIP8_H
#define CHIP8_H

#include <array>
#include <cstdint>
#include <string>
#include <SDL2/SDL.h>


constexpr size_t MEM_SIZE = 4096;
constexpr size_t SWIDTH = 64 * 10;
constexpr size_t SHEIGHT = 32 * 10; 
constexpr size_t NUM_REGISTERS = 16;

constexpr uint16_t PROGRAM_START = 0x200;
constexpr uint16_t FONTSET_START_ADDRESS = 0x50;
constexpr uint8_t FONTSET_SIZE = 80;

extern std::array<uint8_t, MEM_SIZE> memory;
extern std::array<uint8_t, NUM_REGISTERS> v_regs; 
extern uint16_t i_reg; 
extern uint16_t pc; 
extern uint16_t sp;
extern std::array<uint16_t, 16> stack;


extern std::array<std::array<bool, SWIDTH>, SHEIGHT> screen;


extern SDL_Window* window;
extern SDL_Renderer* renderer;

extern unsigned long long instruction_counter;


uint16_t grab_opcode();
void render_screen();
std::string current_time_ms();
void run_opcode(uint16_t opcode);
bool init_SDL();
bool load_chip8_file(const std::string& filepath); 

#endif
