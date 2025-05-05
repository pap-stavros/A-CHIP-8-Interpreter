#include <iostream>
#include <array>
#include <cstdint>
#include <chrono>
#include <thread>
#include <fstream>
#include <cstring>
#include <raylib.h>
#include <cmath>
#include <filesystem>
#include <vector>
#include "chip8.h"

// list rom files!!!
std::vector<std::string> GetRomFiles(const std::string& romDir) {
    std::vector<std::string> romFiles;
    for (const auto& entry : std::filesystem::directory_iterator(romDir)) {
        if (entry.path().extension() == ".ch8") {
            romFiles.push_back(entry.path().filename().string());
        }
    }
    return romFiles;
}

// pick from listed rom files!!!
std::string SelectRom(const std::vector<std::string>& romFiles) {
    std::cout << "Available ROM files:\n";
    for (size_t i = 0; i < romFiles.size(); ++i) {
        std::cout << i + 1 << ". " << romFiles[i] << "\n";
    }
    std::cout << "Enter the number of the ROM you want to load: ";
    size_t choice = 0;
    std::cin >> choice;
    
    if (choice < 1 || choice > romFiles.size()) {
        std::cerr << "Invalid selection.\n";
        return "";
    }
    
    return romFiles[choice - 1];
}

int main() {
    srand(time(0));
    std::cerr << "Hello, World!: " << std::endl;

    const std::string romDirectory = "../ROMs";
    std::vector<std::string> romFiles = GetRomFiles(romDirectory);

    if (romFiles.empty()) {
        std::cerr << "No ROM files found in the directory.\n";
        return 1;
    }

    // Let the user select a ROM
    std::string selectedRom = SelectRom(romFiles);
    if (selectedRom.empty()) {
        std::cerr << "Failed to load ROM.\n";
        return 1;
    }

    const std::string filepath = romDirectory + "/" + selectedRom;
    std::cerr << "ROM found: " << filepath << std::endl;
    std::cerr << "Loading ROM...: " << std::endl;
    std::cerr << "Logging in debuglog.txt" << std::endl;
    freopen("debuglog.txt", "w", stdout);
    SetTraceLogLevel(LOG_INFO);

    initialize_system();

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

    while (!quit && !WindowShouldClose()) {
        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed = current_time - last_frame_time;

        process_input();

        // opcode exec
        for (int i = 0; i < 10; i++) {
            uint16_t opcode = grab_opcode();
            run_opcode(opcode);
        }

        update_timers();
        handle_audio();

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
