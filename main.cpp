#include <iostream>
#include <array>
#include <cstdint>
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <fstream>
#include "chip8.h"
#include <cstring>

// memory stuff and other
std::array<uint8_t, MEM_SIZE> memory{};
std::array<uint8_t, NUM_REGISTERS> v_regs{};
uint16_t i_reg = 0; // index register
uint16_t pc = 0x200; // not sure if i still need to do that i think i changed some code later but ill keep it just in case...
uint16_t sp = 0; // stack pointer
std::array<uint16_t, 16> stack{};
std::array<std::array<bool, SWIDTH>, SHEIGHT> screen{};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

uint16_t grab_opcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    return opcode;
}


void render_screen() {
    const int scaleup = 10;
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (size_t y = 0; y < SHEIGHT; ++y) {
        for (size_t x = 0; x < SWIDTH; ++x) {
            if (screen[y][x]) {
                SDL_SetRenderDrawColor(renderer, 51, 255, 51, 255);

                SDL_Rect rect = {
                    static_cast<int>(x * scaleup),
                    static_cast<int>(y * scaleup), 
                    scaleup,                       
                    scaleup                        
                };

                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

//performance related timing stuff
std::string millisecs() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

// the opcode executor, i think he likes killing
void run_opcode(uint16_t opcode) {
    instruction_counter++;

    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                std::memset(&screen, 0, sizeof(screen));
                std::cout << "screen cleared." << std::endl;
                render_screen();
            }
            break;

        case 0x00EE: // RET
            sp--;
            pc = stack[sp];
            std::cout << "RET from subroutine to 0x" << std::hex << pc << std::dec << std::endl;
            break;

        case 0x2000: // ring ring subroutine at NNN
            {
                uint16_t subaddress = opcode & 0x0FFF;
                stack[sp] = pc;
                sp++;
                pc = subaddress;
                std::cout << "Call subroutine at 0x" << std::hex << subaddress << std::dec << std::endl;
            }
            break;

        
        case 0x4000:
            {
                uint8_t vx_index = (opcode & 0x0F00) >> 8;
                uint8_t nn = static_cast<uint8_t>(opcode & 0x00FF);
                if (v_regs[vx_index] != nn) {
                    pc += 2;  // Skip the next instruction
                    std::cout << "Skipped instr cause V" << std::hex << vx_index << " != " << std::hex << (int)nn << std::dec << std::endl;
                }
            }
            break;

        case 0x5000:
            {
                uint8_t vx_index = (opcode & 0x0F00) >> 8;
                uint8_t vy_index = (opcode & 0x00F0) >> 4;
                if (v_regs[vx_index] == v_regs[vy_index]) {
                    pc += 2;
                    std::cout << "Skipped instr cause V" << std::hex << vx_index << " == V" << vy_index << std::dec << std::endl;
                }
            }
            break;

        case 0x3000:
            {
                uint8_t vx_index = (opcode & 0x0F00) >> 8;
                uint8_t nn = static_cast<uint8_t>(opcode & 0x00FF);
                if (v_regs[vx_index] == nn) {
                    pc += 2;  // Skip the next instruction
                    std::cout << "Skipped isntru because V" << std::hex << vx_index << " == " << std::hex << (int)nn << std::dec << std::endl;
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
                std::cout << "et reg V" << std::hex << (int)xvIndex << " to " << (int)val8b << std::dec << std::endl;
            }
            break;
        
        case 0x7000:
            {
                uint8_t vxIndex = (opcode & 0x0F00) >> 8;
                uint8_t val8b = static_cast<uint8_t>(opcode & 0x00FF);
                v_regs[vxIndex] += val8b;
                std::cout << "Add " << std::hex << (int)val8b << " to V" << vxIndex << std::dec << std::endl;
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

        case 0xA000:
            {
                i_reg = opcode & 0x0FFF;
                std::cout << "set index reg to 0x" << std::hex << i_reg << std::dec << std::endl;
            }
            break;

        case 0xD000:
            {
                uint8_t x = v_regs[(opcode & 0x0F00) >> 8];
                uint8_t y = v_regs[(opcode & 0x00F0) >> 4];
                uint8_t height = opcode & 0x000F;

                v_regs[0xF] = 0; // clear vf before doing gods work

                for (uint8_t row = 0; row < height; ++row) {
                    uint8_t sprite_row = memory[i_reg + row];
                    for (uint8_t col = 0; col < 8; ++col) {
                        if (sprite_row & (0x80 >> col)) {
                            // collision check!!
                            if (screen[y + row][x + col]) {
                                v_regs[0xF] = 1;
                            }
                            screen[y + row][x + col] ^= true;
                        }
                    }
                }
                render_screen();
                std::cout << "Draw sprite at (" << std::hex << (int)x << ", " << (int)y << ")" << std::dec << std::endl;
            }
            break;

        default:
            std::cout << "Unknown opcode uhhhh implement it... 0x" << std::hex << opcode << std::dec << std::endl;
            break;
    }

    // purely to see how many instructions we can get
//    if (instruction_counter % 1000 == 0) {
//        std::cout << "Instructions executed: " << instruction_counter << std::endl;
//    }
}

bool init_SDL() {

    std::cerr << "Initializing SDL2... " << std::endl;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init error i cant start: " << SDL_GetError() << std::endl;
        return false;
    }

    // the cool window that i might give ui cause i dont want to just make keybinds, im thinking a whole fake keyboard and all
    window = SDL_CreateWindow(">_CHIP-8 Interpreter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SWIDTH, SHEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL_CreateWindow Something went bad idk: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer something went even more bad " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    return true;
}

bool load_chip8_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg(); // tell me ur size file
    file.seekg(0, std::ios::beg); // seek 0 or like go to the start

    // is the file too big? probably not, still wanted to add this idk
    if (static_cast<size_t>(size) > (MEM_SIZE - 0x200)) {
        std::cerr << "File too large!!!!!" << std::endl;
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(&memory[0x200]), size)) {
        std::cerr << "cant read file: " << filepath << std::endl;
        return false;
    }

    return true;
}

int main() {
    const std::string filepath = "2-ibm-logo.ch8"; // had to manually load rom cause im lazy
    std::cerr << "Loading ROM...: " << std::endl;

    // rev up the engine (sdl2)
    if (!init_SDL()) {
        return 1;
         std::cerr << "SDL2 Initialized: " << std::endl;
    }

    // this is like an imaginary memory cartridge
    if (!load_chip8_file(filepath)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    constexpr auto frame_delay = std::chrono::microseconds(16667);
    auto last_frame_time = std::chrono::high_resolution_clock::now();

    while (!quit) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        for (int i = 0; i < 10; i++) {
            uint16_t opcode = grab_opcode();
            run_opcode(opcode);
        }

        if (elapsed >= frame_delay) {
            render_screen();
            last_frame_time = current_time;
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
    }

    // clean up on aisle 8
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
