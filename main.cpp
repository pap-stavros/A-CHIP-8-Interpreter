#include <iostream>
#include <array>
#include <cstdint>
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <fstream>
#include "chip8.h"

// memory stuff and other
std::array<uint8_t, MEM_SIZE> memory{};
std::array<uint8_t, NUM_REGISTERS> v_regs{};
uint16_t i_reg = 0; // i register
uint16_t pc = 0x200; // pc starts here but i might not need that anymore, ill keep it cause idk
uint16_t sp = 0; // stack pointer
std::array<uint16_t, 16> stack{};
std::array<std::array<bool, SWIDTH>, SHEIGHT> screen{};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

unsigned long long instruction_counter = 0; // performance stats thing

// self explanatory
uint16_t grab_opcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    return opcode;
}


void render_screen() {
    SDL_RenderClear(renderer);

    // this guy is busy as hell drawing pixels everywhere
    for (size_t y = 0; y < SHEIGHT; ++y) {
        for (size_t x = 0; x < SWIDTH; ++x) {
            if (screen[y][x]) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                SDL_RenderDrawPoint(renderer, static_cast<int>(x), static_cast<int>(y));
            }
        }
    }

    SDL_RenderPresent(renderer); // this guy is like the talk show host
}

// timing stuff for performance purposes
std::string current_time_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

// the opcode executor, i think he likes killing
void run_opcode(uint16_t opcode) {
    instruction_counter++;

    switch (opcode & 0xF000) {
        case 0x0000: // this is the clear screen opcode and the first one i implemented
            if (opcode == 0x00E0) {
                for (size_t y = 0; y < SHEIGHT; ++y) {
                    for (size_t x = 0; x < SWIDTH; ++x) {
                        screen[y][x] = false; // Clear each pixel
                    }
                }
                std::cout << "screen cleared." << std::endl;
                render_screen();
            }
            break;

        case 0x1000: // JUMP TO NNN
            pc = opcode & 0x0FFF;
            std::cout << "[" << current_time_ms() << "] Jump to 0x" << std::hex << pc << std::dec << std::endl;
            break;

        case 0x6000: // LOAD REGISTER VALUE
            {
                uint8_t xvIndex = (opcode & 0x0F00) >> 8; // get reg indx value
                uint8_t val8b = static_cast<uint8_t>(opcode & 0x00FF); // keep that value to urself
                v_regs[xvIndex] = val8b; // and set it to something i forgot
                std::cout << "[" << current_time_ms() << "] Set reg V" << std::hex << (int)xvIndex << " to " << (int)val8b << std::dec << std::endl;
            }
            break;

        default:
            std::cout << "Unknown opcode uhhhh implement it... 0x" << std::hex << opcode << std::dec << std::endl;
            break;
    }

    // purely to see how many instructions we can get
    if (instruction_counter % 1000 == 0) {
        std::cout << "Instructions executed: " << instruction_counter << std::endl;
    }
}

// this is like the engine starter for video rendering
bool init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init error i cant start: " << SDL_GetError() << std::endl;
        return false;
    }

    // the cool window that i might give ui cause i dont want to just make keybinds, im thinking a whole fake keyboard and all
    window = SDL_CreateWindow(">_ CHIP-8 Interpreter by SP", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SWIDTH * 10, SHEIGHT * 10, SDL_WINDOW_SHOWN);
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

// load up the rom ! ! !
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

// loooooooooooooop (pretend its infinite o's yet you can still see the p)
int main() {
    const std::string filepath = "1-chip8-logo.ch8"; // had to manually load rom cause im lazy

    // rev up the engine (sdl2)
    if (!init_SDL()) {
        return 1;
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

    // target 16ms, 500Hz cpu clock speed, so far: 17Mhz 0ms oops lol
    constexpr auto frame_delay = std::chrono::microseconds(16667);
    auto last_frame_time = std::chrono::high_resolution_clock::now();

    
    while (!quit) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        
        for (int i = 0; i < 10; i++) {
            uint16_t opcode = grab_opcode();
            run_opcode(opcode);
        }

        // this bad boy makes sure the render happens at the right time
        if (elapsed >= frame_delay) {
            render_screen();
            last_frame_time = current_time;
        }

        // this is like some event handler i saw online i forgot
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
