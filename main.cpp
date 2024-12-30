#include <iostream>
#include <array>
#include <cstdint>
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <fstream>

// the holy gaming specs
constexpr size_t MEM_SIZE = 4096;
constexpr size_t SWIDTH = 64; // screen width
constexpr size_t SHEIGHT = 32; // screen height
constexpr size_t NUM_REGISTERS = 16;

// memory and stuff
std::array<uint8_t, MEM_SIZE> memory{};
std::array<uint8_t, NUM_REGISTERS> v_regs{}; // registers from v0 to vf
uint16_t i_reg = 0; // i register thingy
uint16_t pc = 0x200; // pc starts here
uint16_t sp = 0; // stack pointer baby
std::array<uint16_t, 16> stack{}; // 16-level stack thing

// 64x32 screen buffer, bool on or off
std::array<std::array<bool, SWIDTH>, SHEIGHT> screen{};

// SDL variables u never know when u might need them (always lol)
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

// Fixed delay duration for each cycle (in milliseconds)
constexpr uint32_t target_cycle_duration = 2;  // 2ms per cycle for fixed delay

// Counter for tracking executed instructions
unsigned long long instruction_counter = 0;

// func to grab next opcode from mem
uint16_t grab_opcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    return opcode;
}

void render_screen() {
    // set the pixels to black
    SDL_RenderClear(renderer);

    // screen loop complicated stuff bro
    for (size_t y = 0; y < SHEIGHT; ++y) {
        for (size_t x = 0; x < SWIDTH; ++x) {
            if (screen[y][x]) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // white
                SDL_RenderDrawPoint(renderer, static_cast<int>(x), static_cast<int>(y));
            }
        }
    }

    // show me the rendered stuff ! ! !
    SDL_RenderPresent(renderer);
}

// debug stuff to see milliseconds
std::string current_time_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

// obey the opcode i think this is the interpreter hey peter
void run_opcode(uint16_t opcode) {
    instruction_counter++;  // Increment the counter every time an opcode is executed
    switch (opcode & 0xF000) {
        case 0x0000: //CLEAR SCREEN !
            if (opcode == 0x00E0) {
                for (size_t y = 0; y < SHEIGHT; ++y) {
                    for (size_t x = 0; x < SWIDTH; ++x) {
                        screen[y][x] = false; // clear the screen
                    }
                }
                std::cout << "screen cleared." << std::endl;
                render_screen();
            }
            break;

        case 0x1000: // JUMP TO NNN
            pc = opcode & 0x0FFF; // jump to address nnn
            std::cout << "[" << current_time_ms() << "] Jump to 0x" << std::hex << pc << std::dec << std::endl;
            break;

        case 0x6000: // LOAD REGISTER VALUE
            {
                uint8_t xvIndex = (opcode & 0x0F00) >> 8;
                uint8_t val8b = static_cast<uint8_t>(opcode & 0x00FF);
                v_regs[xvIndex] = val8b;
                std::cout << "[" << current_time_ms() << "] Set reg V" << std::hex << (int)xvIndex << " to " << (int)val8b << std::dec << std::endl;
            }
            break;

        default:
            std::cout << "Unknown opcode uhhhh implement it... 0x" << std::hex << opcode << std::dec << std::endl;
            break;
    }

    // Print the instruction count periodically (every 1000 instructions)
    if (instruction_counter % 1000 == 0) {
        std::cout << "Instructions executed: " << instruction_counter << std::endl;
    }
}

// rev up those sdl2 engines, time to witness the 1970s
bool init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    window = SDL_CreateWindow(">_ CHIP-8 Interpreter by SP", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SWIDTH * 10, SHEIGHT * 10, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return false;
    }

    return true;
}

// Load CHIP-8 file into memory
bool load_chip8_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (static_cast<size_t>(size) > (MEM_SIZE - 0x200)) {
        std::cerr << "File too large for CHIP-8 memory." << std::endl;
        return false;
    }

    if (!file.read(reinterpret_cast<char*>(&memory[0x200]), size)) {
        std::cerr << "Failed to read file: " << filepath << std::endl;
        return false;
    }

    return true;
}

// loooooooooooooop (pretend its infinite O's yet you can still read the p)
int main() {
    const std::string filepath = "1-chip8-logo.ch8"; // Hardcoded file path

    if (!init_SDL()) {
        return 1;
    }

    if (!load_chip8_file(filepath)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    // pray and hope for 60hz it took me like 4 hours to figure out
    constexpr auto frame_delay = std::chrono::microseconds(16667);
    auto last_frame_time = std::chrono::high_resolution_clock::now();

    while (!quit) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        // multitasking stuff
        for (int i = 0; i < 10; i++) { // if my math is correct ass line target is 600 instructions per sec, dont know if accurate, it looks fast though
            uint16_t opcode = grab_opcode();
            run_opcode(opcode);
        }

        // basically render at the right time
        if (elapsed >= frame_delay) {
            render_screen();
            last_frame_time = current_time;
        }

        // sdl event guy he does important stuff probably i saw it on the internet
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
