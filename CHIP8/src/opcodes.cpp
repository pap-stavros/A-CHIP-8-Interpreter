#include "chip8.h"
#include <iostream>
#include <cstring>

//  opcode executor
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