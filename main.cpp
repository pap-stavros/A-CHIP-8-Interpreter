#include <iostream>
#include <array>
#include <cstdint>
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <fstream>
#include "chip8.h"

// Memory, registers, and system state initialization
std::array<uint8_t, MEM_SIZE> memory{};
std::array<uint8_t, NUM_REGISTERS> v_regs{};
uint16_t i_reg = 0; // i register
uint16_t pc = 0x200; // pc starts here
uint16_t sp = 0; // stack pointer
std::array<uint16_t, 16> stack{};
std::array<std::array<bool, SWIDTH>, SHEIGHT> screen{}; // screen buffer

SDL_Window* window = nullptr; // SDL window pointer
SDL_Renderer* renderer = nullptr; // SDL renderer pointer

unsigned long long instruction_counter = 0; // Instruction execution counter

// Grabs the next opcode from memory
uint16_t grab_opcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2; // Move program counter ahead
    return opcode;
}

// Renders the screen buffer to the SDL window
void render_screen() {
    SDL_RenderClear(renderer); // Clear the renderer

    // Loop through each pixel on the screen and draw it
    for (size_t y = 0; y < SHEIGHT; ++y) {
        for (size_t x = 0; x < SWIDTH; ++x) {
            if (screen[y][x]) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White color
                SDL_RenderDrawPoint(renderer, static_cast<int>(x), static_cast<int>(y));
            }
        }
    }

    SDL_RenderPresent(renderer); // Present the rendered content to the window
}

// Gets the current system time in milliseconds
std::string current_time_ms() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    return std::to_string(millis);
}

// Executes the given opcode
void run_opcode(uint16_t opcode) {
    instruction_counter++; // Increment the instruction counter

    switch (opcode & 0xF000) {
        case 0x0000: // CLEAR SCREEN
            if (opcode == 0x00E0) { // Clear screen opcode
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
            pc = opcode & 0x0FFF; // Set program counter to new address
            std::cout << "[" << current_time_ms() << "] Jump to 0x" << std::hex << pc << std::dec << std::endl;
            break;

        case 0x6000: // LOAD REGISTER VALUE
            {
                uint8_t xvIndex = (opcode & 0x0F00) >> 8; // Extract register index
                uint8_t val8b = static_cast<uint8_t>(opcode & 0x00FF); // Extract value
                v_regs[xvIndex] = val8b; // Set register value
                std::cout << "[" << current_time_ms() << "] Set reg V" << std::hex << (int)xvIndex << " to " << (int)val8b << std::dec << std::endl;
            }
            break;

        default:
            std::cout << "Unknown opcode uhhhh implement it... 0x" << std::hex << opcode << std::dec << std::endl;
            break;
    }

    // Print instruction count every 1000 instructions
    if (instruction_counter % 1000 == 0) {
        std::cout << "Instructions executed: " << instruction_counter << std::endl;
    }
}

// Initializes SDL2 for video rendering
bool init_SDL() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // Create window and renderer
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

// Loads a CHIP-8 file into memory
bool load_chip8_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate); // Open file in binary mode
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg(); // Get file size
    file.seekg(0, std::ios::beg); // Move to the beginning of the file

    // Check if the file is too large to fit into memory
    if (static_cast<size_t>(size) > (MEM_SIZE - 0x200)) {
        std::cerr << "File too large for CHIP-8 memory." << std::endl;
        return false;
    }

    // Read the file contents into memory starting at 0x200
    if (!file.read(reinterpret_cast<char*>(&memory[0x200]), size)) {
        std::cerr << "Failed to read file: " << filepath << std::endl;
        return false;
    }

    return true;
}

// Main interpreter loop
int main() {
    const std::string filepath = "1-chip8-logo.ch8"; // Path to the CHIP-8 program file

    // Initialize SDL2
    if (!init_SDL()) {
        return 1;
    }

    // Load the CHIP-8 file into memory
    if (!load_chip8_file(filepath)) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    // Frame delay for 60Hz execution (16ms per frame)
    constexpr auto frame_delay = std::chrono::microseconds(16667);
    auto last_frame_time = std::chrono::high_resolution_clock::now();

    // Main loop
    while (!quit) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        // Execute 10 opcodes per frame (targeting ~600 instructions per second)
        for (int i = 0; i < 10; i++) {
            uint16_t opcode = grab_opcode();
            run_opcode(opcode);
        }

        // Render the screen at the correct timing
        if (elapsed >= frame_delay) {
            render_screen();
            last_frame_time = current_time;
        }

        // Handle SDL events (e.g., quit event)
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
        }
    }

    // Clean up SDL
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
