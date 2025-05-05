#include "chip8.h"
#include <iostream>
#include <cstring>
#include <chrono>

// global stuff
std::array<uint8_t, MEM_SIZE> memory{};
std::array<uint8_t, NUM_REGISTERS> v_regs{};
uint16_t i_reg = 0; // index register
uint16_t pc = 0x200;
uint16_t sp = 0; // stack pointer
std::array<uint16_t, 16> stack{};
std::array<std::array<bool, SWIDTH>, SHEIGHT> screen{};

uint8_t delay_timer = 0;
uint8_t sound_timer = 0;

std::array<bool, 16> keypad{};

const std::map<int, uint8_t> keymap = {
    {KEY_ONE, 0x1}, {KEY_TWO, 0x2}, {KEY_THREE, 0x3}, {KEY_FOUR, 0xC},
    {KEY_Q, 0x4}, {KEY_W, 0x5}, {KEY_E, 0x6}, {KEY_R, 0xD},
    {KEY_A, 0x7}, {KEY_S, 0x8}, {KEY_D, 0x9}, {KEY_F, 0xE},
    {KEY_Z, 0xA}, {KEY_X, 0x0}, {KEY_C, 0xB}, {KEY_V, 0xF}
};

const uint8_t fontset[FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void initialize_system() {
    std::memcpy(&memory[FONTSET_START_ADDRESS], fontset, FONTSET_SIZE);
    pc = PROGRAM_START;
    i_reg = 0;
    sp = 0;
    delay_timer = 0;
    sound_timer = 0;
    
    std::memset(&screen, 0, sizeof(screen));
    std::memset(&stack, 0, sizeof(stack));
    std::memset(&v_regs, 0, sizeof(v_regs));
    std::memset(&memory, 0, sizeof(memory));
    std::memcpy(&memory[FONTSET_START_ADDRESS], fontset, FONTSET_SIZE);
    srand(time(0));
}

uint16_t grab_opcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    return opcode;
}

// timing stuff
std::string millisecs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

void update_timers() {
    if (delay_timer > 0) {
        --delay_timer;
    }
    
    if (sound_timer > 0) {
        --sound_timer;
    }
}

void process_input() {
    for (const auto& [key, chip8_key] : keymap) {
        keypad[chip8_key] = IsKeyDown(key);
    }
}