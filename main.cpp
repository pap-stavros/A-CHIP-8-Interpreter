#include <iostream>
#include <array>
#include <cstdint>
#include <chrono>
#include <thread>
#include <fstream>
#include "chip8.h"
#include <cstring>
#include <raylib.h>
#include <cmath>

std::array<uint8_t, MEM_SIZE> memory{};
std::array<uint8_t, NUM_REGISTERS> v_regs{};
uint16_t i_reg = 0; // index register
uint16_t pc = 0x200;
uint16_t sp = 0; // stack pointer
std::array<uint16_t, 16> stack{};
std::array<std::array<bool, SWIDTH>, SHEIGHT> screen{};

uint8_t delay_timer = 0;
uint8_t sound_timer = 0;

Sound beep;
bool audio_initialized = false;

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

// Raylib stuff
bool window_initialized = false;
Vector2 screen_size = { SWIDTH, SHEIGHT };

//declaration of independence, no just declarations actually
bool init_raylib();
bool load_chip8_file(const std::string& filepath);
void render_screen();
uint16_t grab_opcode();


uint16_t grab_opcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    return opcode;
}

void render_screen() {
    const int scaleup = 10;
    BeginDrawing();
    ClearBackground(BLACK);

    for (size_t y = 0; y < SHEIGHT; ++y) {
        for (size_t x = 0; x < SWIDTH; ++x) {
            if (screen[y][x]) {
                DrawRectangle(x * scaleup, y * scaleup, scaleup, scaleup, GREEN);
            }
        }
    }

    EndDrawing();
}

// timing stuff
std::string millisecs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

//  opcode executor, hes a cold blooded motherf***er
void run_opcode(uint16_t opcode) {
    switch (opcode & 0xF000) {
        case 0x0000:
            switch(opcode) {
                case 0x00E0:  // Clear screen
                    std::memset(&screen, 0, sizeof(screen));
                    std::cout << "screen cleared." << std::endl;
                    render_screen();
                    break;
                    
                case 0x00EE:
                    if (sp > 0) {
                        --sp;
                        pc = stack[sp];
                    } else {
                        std::cerr << "Stack underflow during RET, huh?" << std::endl;
                    }
                    break;
            }
            break;

        case 0xB000:  // Jump with offset
        {
            uint16_t address = opcode & 0x0FFF;
            pc = address + v_regs[0];
            break;
        }

        case 0x2000: // ring ring subroutine at NNN
            {
                uint16_t subaddress = opcode & 0x0FFF;
                stack[sp] = pc;
                sp++;
                pc = subaddress;
            }
            break;

        case 0x4000: {
            uint8_t vx_index = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            if (v_regs[vx_index] != nn) {
                pc += 2;
            }
            break;
        }

        case 0x5000:
            {
                uint8_t vx_index = (opcode & 0x0F00) >> 8;
                uint8_t vy_index = (opcode & 0x00F0) >> 4;
                if (v_regs[vx_index] == v_regs[vy_index]) {
                    pc += 2;
                }
            }
            break;

        case 0x3000:
            {
                uint8_t vx_index = (opcode & 0x0F00) >> 8;
                uint8_t nn = static_cast<uint8_t>(opcode & 0x00FF);
                if (v_regs[vx_index] == nn) {
                    pc += 2;
                }
            }
            break;

        case 0x1000:
        {
            uint16_t address = opcode & 0x0FFF;

            if (address != pc) {
                pc = address;
            } else {
                std::cout << "Infinite loop detected, staying at 0x" << std::hex << pc << std::dec << std::endl;
            }
        }
        break;

        case 0x6000:
            {
                uint8_t xvIndex = (opcode & 0x0F00) >> 8;
                uint8_t val8b = static_cast<uint8_t>(opcode & 0x00FF);
                v_regs[xvIndex] = val8b;
            }
            break;
        
        case 0x7000:
            {
                uint8_t vxIndex = (opcode & 0x0F00) >> 8;
                uint8_t val8b = static_cast<uint8_t>(opcode & 0x00FF);
                v_regs[vxIndex] += val8b;
            }
            break;

        case 0x8000: // arithmetic opcodes
        {
            uint8_t vxIndex = (opcode & 0x0F00) >> 8;
            uint8_t vyIndex = (opcode & 0x00F0) >> 4;
            uint8_t vx = v_regs[vxIndex];
            uint8_t vy = v_regs[vyIndex];
            
            switch (opcode & 0x000F) {
                case 0x0:
                    v_regs[vxIndex] = vy;
                    break;
                case 0x1:
                    v_regs[vxIndex] = vx | vy;
                    break;
                case 0x2:
                    v_regs[vxIndex] = vx & vy;
                    break;
                case 0x3:
                    v_regs[vxIndex] = vx ^ vy;
                    break;
                case 0x4: {
                    uint16_t result = vx + vy;
                    uint8_t flag = (result > 0xFF) ? 1 : 0;
                    v_regs[vxIndex] = static_cast<uint8_t>(result);
                    v_regs[0xF] = flag;
                    break;
                }
                case 0x5: {
                    uint8_t flag = (vx >= vy) ? 1 : 0;
                    v_regs[vxIndex] = vx - vy;
                    v_regs[0xF] = flag;
                    break;
                }
                case 0x6: {
                    uint8_t val = vy;
                    uint8_t flag = val & 0x1;
                    v_regs[vxIndex] = val >> 1;
                    v_regs[0xF] = flag;
                    break;
                }
                case 0x7: {
                    uint8_t flag = (vy >= vx) ? 1 : 0;
                    v_regs[vxIndex] = vy - vx;
                    v_regs[0xF] = flag;
                    break;
                }
                case 0xE: {
                    uint8_t val = vy;
                    uint8_t flag = (val & 0x80) >> 7;
                    v_regs[vxIndex] = val << 1;
                    v_regs[0xF] = flag;
                    break;
                }
                default:
                    std::cout << "Unknown 8xy opcode: " << std::hex << opcode << std::dec << std::endl;
                    break;
            }
        }
        break;

        case 0x9000: {
            uint8_t vx_index = (opcode & 0x0F00) >> 8;
            uint8_t vy_index = (opcode & 0x00F0) >> 4;
            if (v_regs[vx_index] != v_regs[vy_index]) {
                pc += 2;
            }
            break;
        }

        case 0xA000:
            {
                i_reg = opcode & 0x0FFF;
            }
            break;

        case 0xF000: {
            switch (opcode & 0x00FF) {
                case 0x0007:
                    {
                        uint8_t vx_index = (opcode & 0x0F00) >> 8;
                        v_regs[vx_index] = delay_timer;
                    }
                    break;
                    
                case 0x000A: {
                    uint8_t vx_index = (opcode & 0x0F00) >> 8;
                    bool key_pressed = false;
                    
                    for (uint8_t i = 0; i < 16; i++) {
                        if (keypad[i]) {
                            v_regs[vx_index] = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    
                    if (!key_pressed) {
                        pc -= 2;
                    }
                    break;
                }
                    
                case 0x0015:
                    {
                        uint8_t vx_index = (opcode & 0x0F00) >> 8;
                        delay_timer = v_regs[vx_index];
                    }
                    break;
                    
                case 0x0018:
                    {
                        uint8_t vx_index = (opcode & 0x0F00) >> 8;
                        sound_timer = v_regs[vx_index];
                    }
                    break;
                    
                case 0x001E:
                    {
                        uint8_t vx_index = (opcode & 0x0F00) >> 8;
                        i_reg += v_regs[vx_index];
                        v_regs[0xF] = (i_reg > 0xFFF) ? 1 : 0;
                    }
                    break;
                    
                case 0x0029:
                    {
                        uint8_t vx_index = (opcode & 0x0F00) >> 8;
                        i_reg = FONTSET_START_ADDRESS + (v_regs[vx_index] * 5);
                    }
                    break;
                    
                case 0x0033:
                    {
                        uint8_t vx_index = (opcode & 0x0F00) >> 8;
                        memory[i_reg] = v_regs[vx_index] / 100;
                        memory[i_reg + 1] = (v_regs[vx_index] / 10) % 10;
                        memory[i_reg + 2] = v_regs[vx_index] % 10;
                    }
                    break;
                    
                case 0x0055:
                {
                    uint8_t vx_index = (opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= vx_index; ++i) {
                        memory[i_reg + i] = v_regs[i];
                    }
                    i_reg += vx_index + 1;
                }
                break;

                case 0x0065:
                {
                    uint8_t vx_index = (opcode & 0x0F00) >> 8;
                    for (int i = 0; i <= vx_index; ++i) {
                        v_regs[i] = memory[i_reg + i];
                    }
                    i_reg += vx_index + 1;
                }
                break;
                    
                default:
                    std::cout << "Unknown 0xF000 opcode: " << std::hex << opcode << std::dec << std::endl;
                    break;
            }
            break;
        }

        case 0xE000: {
            uint8_t vx_index = (opcode & 0x0F00) >> 8;
            switch (opcode & 0x00FF) {
                case 0x009E:
                    if (keypad[v_regs[vx_index]]) {
                        pc += 2;
                    }
                    break;
                    
                case 0x00A1:
                    if (!keypad[v_regs[vx_index]]) {
                        pc += 2;
                    }
                    break;
                    
                default:
                    std::cout << "Unknown 0xE000 opcode who dis: " << std::hex << opcode << std::dec << std::endl;
                    break;
            }
            break;
        }

        case 0xC000: {
            uint8_t vx = (opcode & 0x0F00) >> 8;
            uint8_t nn = opcode & 0x00FF;
            v_regs[vx] = (rand() & 0xFF) & nn;
            break;
        }

        case 0xD000: {
            uint8_t vx = v_regs[(opcode & 0x0F00) >> 8];
            uint8_t vy = v_regs[(opcode & 0x00F0) >> 4];
            uint8_t height = opcode & 0x000F;
            
            std::cout << std::hex << "Draw sprite i:" << i_reg 
                        << " vx:" << (int)vx << " vy:" << (int)vy 
                        << " h:" << (int)height << std::dec << std::endl;
            
            v_regs[0xF] = 0;

            for (uint8_t row = 0; row < height; row++) {
                uint8_t sprite_byte = memory[i_reg + row];
                std::cout << "Row " << (int)row << " data: " << std::hex << (int)sprite_byte << std::dec << std::endl;
                
                for (uint8_t bit = 0; bit < 8; bit++) {
                    if (sprite_byte & (0x80 >> bit)) {
                        size_t x = (vx + bit) % SWIDTH;
                        size_t y = (vy + row) % SHEIGHT;
                        
                        if (screen[y][x]) {
                            v_regs[0xF] = 1;
                        }
                        screen[y][x] ^= 1;
                    }
                }
            }
            render_screen();
            break;
            }
        
        default:
            std::cout << "Unknown opcode who dis: " << std::hex << opcode << " (" << (opcode & 0xF000) << ")" << std::dec << std::endl;
            break;
    }
}


bool init_raylib() {
    std::cerr << "Initializing Raylib... " << std::endl;
    
    const int scaleup = 10;
    InitWindow(SWIDTH * scaleup, SHEIGHT * scaleup, ">_ CHIP-8 Interpreter in Raylib.");
    SetTargetFPS(60);

    // audio stuff
    InitAudioDevice();
    if (IsAudioDeviceReady()) {
        audio_initialized = true;
        const unsigned int sampleRate = 44100;
        const unsigned int seconds = 1;
        Wave wave = {
            .frameCount = sampleRate * seconds,
            .sampleRate = sampleRate,
            .sampleSize = 16,
            .channels = 1,
            .data = nullptr
        };
        
        wave.data = malloc(wave.frameCount * sizeof(short));

        // 440Hz sine wave
        short* samples = static_cast<short*>(wave.data);
        for (unsigned int i = 0; i < wave.frameCount; i++) {
            float time = static_cast<float>(i) / sampleRate;
            samples[i] = static_cast<short>(32000.0f * std::sin(2.0f * PI * 440.0f * time));
        }

        beep = LoadSoundFromWave(wave);
        UnloadWave(wave);
    }

    if (IsWindowReady()) {
        window_initialized = true;
        return true;
    } else {
        std::cerr << "Raylib failed to start window, ooops...sorry!" << std::endl;
        return false;
    }
}

bool load_chip8_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file > " << filepath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (static_cast<size_t>(size) > (MEM_SIZE - 0x200)) {
        std::cerr << "File too large..." << std::endl;
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(&memory[0x200]), size)) {
        std::cerr << "Can't read that file > " << filepath << std::endl;
        return false;
    }

    return true;
}

// the soup
int main() {
    std::memcpy(&memory[FONTSET_START_ADDRESS], fontset, FONTSET_SIZE);
    srand(time(0));
    std::cerr << "Hello, World!: " << std::endl;
    const std::string filepath = "ROMs/aceattorney.ch8";
    freopen("debuglog.txt", "w", stdout);
    std::cerr << "ROM found: " << filepath << std::endl;
    std::cerr << "Loading ROM...: " << std::endl;
    std::cerr << "Logging in debuglog.txt" << std::endl;
    SetTraceLogLevel(LOG_INFO);

    if (!init_raylib()) {
        return 1;
    }

    if (!load_chip8_file(filepath)) {
        CloseWindow();
        if (audio_initialized) {
            UnloadSound(beep);
            CloseAudioDevice();
        }
        return 1;
    }

    bool quit = false;
    constexpr auto frame_delay = std::chrono::microseconds(16667);
    auto last_frame_time = std::chrono::high_resolution_clock::now();
    bool was_playing = false;

    while (!quit && !WindowShouldClose()) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        for (const auto& [key, chip8_key] : keymap) {
            keypad[chip8_key] = IsKeyDown(key);
        }

        for (int i = 0; i < 10; i++) {
            uint16_t opcode = grab_opcode();
            run_opcode(opcode);
        }

        if (delay_timer > 0) {
            --delay_timer;
        }

        if (audio_initialized) {
            if (sound_timer > 0) {
                if (!was_playing) {
                    PlaySound(beep);
                    was_playing = true;
                }
                --sound_timer;
            } else if (was_playing) {
                StopSound(beep);
                was_playing = false;
            }
        }

        if (elapsed >= frame_delay) {
            render_screen();
            last_frame_time = current_time;
        }
    }

    if (audio_initialized) {
        UnloadSound(beep);
        CloseAudioDevice();
    }
    CloseWindow();
    std::cerr << "Goodbye, World..." << std::endl;
    return 0;
}