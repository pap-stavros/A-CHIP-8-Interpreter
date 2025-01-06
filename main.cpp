#include <iostream>
#include <array>
#include <cstdint>
#include <chrono>
#include <thread>
#include <fstream>
#include "chip8.h"
#include <cstring>
#include <raylib.h>

// Global variables
std::array<uint8_t, MEM_SIZE> memory{};
std::array<uint8_t, NUM_REGISTERS> v_regs{};
uint16_t i_reg = 0; // index register
uint16_t pc = 0x200;
uint16_t sp = 0; // stack pointer
std::array<uint16_t, 16> stack{};
std::array<std::array<bool, SWIDTH>, SHEIGHT> screen{};

uint8_t delay_timer = 0;
uint8_t sound_timer = 0;

// Raylib stuff
bool window_initialized = false;
Vector2 screen_size = { SWIDTH, SHEIGHT };

// Function declarations
bool init_raylib();
bool load_chip8_file(const std::string& filepath);
void render_screen();
uint16_t grab_opcode();

// Implementation of grab_opcode
uint16_t grab_opcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    return opcode;
}

// Implementation of render_screen
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

// Utility function for timing
std::string millisecs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

// Main opcode executor
void run_opcode(uint16_t opcode) {
    if ((opcode & 0xF000) == 0xF000) {
        std::cout << "Matched 0x" << std::hex << opcode << std::dec << std::endl;
    }

    switch (opcode & 0xF000) {
        case 0x0000:
            switch(opcode) {
                case 0x00E0:  // Clear screen
                    std::memset(&screen, 0, sizeof(screen));
                    std::cout << "screen cleared." << std::endl;
                    render_screen();
                    break;
                    
                case 0x00EE:  // Return from subroutine
                    if (sp > 0) {
                        --sp;  // Decrease stack pointer first
                        pc = stack[sp];  // Then get the return address
                    } else {
                        std::cerr << "Stack underflow during RET, huh?" << std::endl;
                    }
                    break;
            }
            break;

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

        case 0x8000: // big arithmetic opcode
            {
                uint8_t vxIndex = (opcode & 0x0F00) >> 8;
                uint8_t vyIndex = (opcode & 0x00F0) >> 4;
                switch (opcode & 0x000F) {
                    case 0x0:
                        v_regs[vxIndex] = v_regs[vyIndex];
                        break;
                    case 0x1:
                        v_regs[vxIndex] |= v_regs[vyIndex];
                        break;
                    case 0x2: 
                        v_regs[vxIndex] &= v_regs[vyIndex];
                        break;
                    case 0x3: 
                        v_regs[vxIndex] ^= v_regs[vyIndex];
                        break;
                    case 0x4: 
                        {
                            uint16_t result = v_regs[vxIndex] + v_regs[vyIndex];
                            v_regs[vxIndex] = static_cast<uint8_t>(result);
                            v_regs[0xF] = (result > 0xFF) ? 1 : 0;
                        }
                        break;
                    case 0x5:
                        v_regs[0xF] = (v_regs[vxIndex] > v_regs[vyIndex]) ? 1 : 0;
                        v_regs[vxIndex] -= v_regs[vyIndex];
                        break;
                    case 0x6:
                        v_regs[0xF] = v_regs[vxIndex] & 0x1;
                        v_regs[vxIndex] >>= 1;
                        break;
                    case 0x7:
                        v_regs[0xF] = (v_regs[vyIndex] > v_regs[vxIndex]) ? 1 : 0;
                        v_regs[vxIndex] = v_regs[vyIndex] - v_regs[vxIndex];
                        break;
                    case 0xE:
                        v_regs[0xF] = (v_regs[vxIndex] & 0x80) >> 7; 
                        v_regs[vxIndex] <<= 1;
                        break;
                    default:
                        std::cout << "Unknown 8xy opcode, who dis: " << std::hex << opcode << std::dec << std::endl;
                        break;
                }
            }
            break;
        
        case 0x8005: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;
            v_regs[0xF] = v_regs[x] >= v_regs[y] ? 1 : 0;
            v_regs[x] = v_regs[x] - v_regs[y];
            break;
        }

        case 0x8006: {
            uint8_t vx_index = (opcode & 0x0F00) >> 8;
            uint8_t flag = v_regs[0xF];
            v_regs[0xF] = v_regs[vx_index] & 0x1;
            v_regs[vx_index] >>= 1;
            break;
        }

        case 0x8007: {
            uint8_t vx_index = (opcode & 0x0F00) >> 8;
            uint8_t vy_index = (opcode & 0x00F0) >> 4;
            v_regs[vx_index] = v_regs[vy_index] - v_regs[vx_index];
            v_regs[0xF] = (v_regs[vy_index] > v_regs[vx_index]) ? 1 : 0;
            break;
        }

        case 0x800E: {
            uint8_t vx_index = (opcode & 0x0F00) >> 8;
            uint8_t flag = v_regs[0xF];
            v_regs[0xF] = (v_regs[vx_index] >> 7) & 0x1;
            v_regs[vx_index] <<= 1;
            break;
        }

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
                    
                case 0x000A:
                    // Handle key press
                    break;
                    
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
                        i_reg = v_regs[vx_index] * 5;
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
                    }
                    break;
                    
                case 0x0065:
                    {
                        uint8_t vx_index = (opcode & 0x0F00) >> 8;
                        for (int i = 0; i <= vx_index; ++i) {
                            v_regs[i] = memory[i_reg + i];
                        }
                    }
                    break;
                    
                default:
                    std::cout << "Unknown 0xF000 opcode: " << std::hex << opcode << std::dec << std::endl;
                    break;
            }
            break;
        }

        case 0xD000: {
            uint8_t x = v_regs[(opcode & 0x0F00) >> 8] % SWIDTH;  // Wrap around screen width
            uint8_t y = v_regs[(opcode & 0x00F0) >> 4] % SHEIGHT; // Wrap around screen height
            uint8_t height = opcode & 0x000F;
            
            v_regs[0xF] = 0; // Reset collision flag
            
            for (uint8_t row = 0; row < height && (y + row) < SHEIGHT; ++row) {
                uint8_t sprite_row = memory[i_reg + row];
                for (uint8_t col = 0; col < 8 && (x + col) < SWIDTH; ++col) {
                    if (sprite_row & (0x80 >> col)) {
                        // Get screen coordinates with wrapping
                        size_t screen_x = (x + col) % SWIDTH;
                        size_t screen_y = (y + row) % SHEIGHT;
                        
                        // If there's already a pixel there, we have a collision
                        if (screen[screen_y][screen_x]) {
                            v_regs[0xF] = 1;
                        }
                        
                        // XOR the pixel
                        screen[screen_y][screen_x] ^= 1;
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
// Implementation of init_raylib
bool init_raylib() {
    std::cerr << "Initializing Raylib... " << std::endl;
    
    const int scaleup = 10;
    InitWindow(SWIDTH * scaleup, SHEIGHT * scaleup, ">_ CHIP-8 Interpreter in Raylib.");
    SetTargetFPS(60);

    // The previous check was wrong - WindowShouldClose() checks if the window
    // should close, not if it's successfully initialized
    if (IsWindowReady()) {
        window_initialized = true;
        return true;
    } else {
        std::cerr << "Raylib failed to start window, ooops...sorry!" << std::endl;
        return false;
    }
}

// Implementation of load_chip8_file
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

// Main function
int main() {
    const std::string filepath = "ROMs/3corax.ch8";
    freopen("debuglog.txt", "w", stdout);
    std::cerr << "ROM found: " << filepath << std::endl;
    std::cerr << "Loading ROM...: " << std::endl;
    std::cerr << "Writing debug info in debuglog.txt..." << std::endl;
    SetTraceLogLevel(LOG_INFO);

    if (!init_raylib()) {
        return 1;
    }

    if (!load_chip8_file(filepath)) {
        CloseWindow();
        return 1;
    }

    bool quit = false;
    constexpr auto frame_delay = std::chrono::microseconds(16667);
    auto last_frame_time = std::chrono::high_resolution_clock::now();

    while (!quit && !WindowShouldClose()) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        for (int i = 0; i < 10; i++) {
            uint16_t opcode = grab_opcode();
            run_opcode(opcode);
        }

        if (delay_timer > 0) {
            --delay_timer;
        }

        if (sound_timer > 0) {
            --sound_timer;
        }

        if (elapsed >= frame_delay) {
            render_screen();
            last_frame_time = current_time;
        }
    }

    CloseWindow();
    std::cerr << "Goodbye, World..." << std::endl;
    return 0;
}