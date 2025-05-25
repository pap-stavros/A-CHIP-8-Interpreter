#include "chip8.h"
#include <iostream>
#include <cmath>

// Raylib stuff
bool window_initialized = false;
Vector2 screen_size = { SWIDTH, SHEIGHT };
Sound beep;
bool audio_initialized = false;

void render_screen() {
    const int scaleup = 15;
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

bool init_raylib() {
    std::cerr << "Initializing Raylib... " << std::endl;
    
    const int scaleup = 15;
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
        free(wave.data);
    }

    if (IsWindowReady()) {
        window_initialized = true;
        return true;
    } else {
        std::cerr << "Raylib failed to start window, ooops...sorry!" << std::endl;
        return false;
    }
}

void handle_audio() {
    static bool was_playing = false;
    
    if (audio_initialized) {
        if (sound_timer > 0) {
            if (!was_playing) {
                PlaySound(beep);
                was_playing = true;
            }
        } else if (was_playing) {
            StopSound(beep);
            was_playing = false;
        }
    }
}
