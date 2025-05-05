#ifndef CHIP8_H
#define CHIP8_H
#include <array>
#include <cstdint>
#include <string>
#include <map>
#include <raylib.h>

// Constants
constexpr size_t MEM_SIZE = 4096;
constexpr size_t SWIDTH = 64;
constexpr size_t SHEIGHT = 32; 
constexpr size_t NUM_REGISTERS = 16;
constexpr uint16_t PROGRAM_START = 0x200;
constexpr uint16_t FONTSET_START_ADDRESS = 0x50;
constexpr uint8_t FONTSET_SIZE = 80;

// External variables
extern std::array<uint8_t, MEM_SIZE> memory;
extern std::array<uint8_t, NUM_REGISTERS> v_regs; 
extern uint16_t i_reg; 
extern uint16_t pc; 
extern uint16_t sp;
extern std::array<uint16_t, 16> stack;
extern std::array<std::array<bool, SWIDTH>, SHEIGHT> screen;

extern uint8_t delay_timer;
extern uint8_t sound_timer;

extern bool window_initialized;
extern Vector2 screen_size;
extern std::array<bool, 16> keypad;
extern const std::map<int, uint8_t> keymap;
extern Sound beep;
extern bool audio_initialized;
extern const uint8_t fontset[FONTSET_SIZE];

// Function declarations
void initialize_system();
uint16_t grab_opcode();
void render_screen();
std::string millisecs();
void run_opcode(uint16_t opcode);
bool init_raylib();
bool load_chip8_file(const std::string& filepath);
void update_timers();
void process_input();
void handle_audio();

#endif // CHIP8_H