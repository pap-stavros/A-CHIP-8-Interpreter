#ifndef CHIP8_H
#define CHIP8_H

#include <array>
#include <cstdint>
#include <string>
#include <map>
#include <raylib.h>

constexpr size_t MEM_SIZE = 4096;
constexpr size_t SWIDTH = 64;
constexpr size_t SHEIGHT = 32; 
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

extern bool window_initialized;
extern Vector2 screen_size;
extern unsigned long long instruction_counter;

// Key mapping for CHIP-8
extern const std::map<int, uint8_t> key_map;

// Font data and related functions
extern const uint8_t fontset[FONTSET_SIZE];
void load_font_set();

uint16_t grab_opcode();
void render_screen();
std::string current_time_ms();
void run_opcode(uint16_t opcode);
bool init_raylib();
bool load_chip8_file(const std::string& filepath);

#endif
