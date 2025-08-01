#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <time.h>
#include "include/raylib.h"
#include "stack.h"

uint8_t memory[4096];

/// each character is 5 pixels tall and 4 pixels wide. 
/// the first 4 bits of every hex represent the 4 horizontal pixels.  
uint8_t fontset[80] = { 
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

uint8_t delay_timer = 0;
uint8_t sound_timer = 0;
uint16_t pc = 0x200;
uint16_t I = 0;
struct stack stack = {0};
uint8_t v_registers[16]; // 0 to 15, 0 to F.
bool frame_buffer[32][64];
bool key_down_status[16];

// copies rom bytes into memory starting at 0x200.
void load_rom(char *filename) {
    FILE *rom = fopen(filename, "rb");
    if (rom == NULL) perror("cannot open rom");
    fseek(rom, 0, SEEK_END);
    long size = ftell(rom);
    rewind(rom);
    
    uint8_t *rom_data_buffer = malloc(size);
    fread(rom_data_buffer, sizeof(uint8_t), size, rom);

    for (int i = 0; i < size; i++) {
        uint8_t current_byte = *(rom_data_buffer + i);
        // printf("%x\n", current_byte);
        memory[0x200 + i] = current_byte;
    }

    fclose(rom);
    free(rom_data_buffer);
}

void init() {
    srand(time(0));
    memset(memory, 0, 4096);
    // copy font into memory
    for (int i = 0; i < 80; i++) memory[0x50 + i] = fontset[i]; 
    // load_rom("ibm_logo.ch8");
}

void update_key_downs() {
    if (IsKeyDown(KEY_ONE)) key_down_status[0x1] = true;
    else key_down_status[0x1] = false;
    if (IsKeyDown(KEY_TWO)) key_down_status[0x2] = true;
    else key_down_status[0x2] = false;
    if (IsKeyDown(KEY_THREE)) key_down_status[0x3] = true;
    else key_down_status[0x3] = false;
    if (IsKeyDown(KEY_FOUR)) key_down_status[0xc] = true;
    else key_down_status[0xc] = false;

    if (IsKeyDown(KEY_Q)) key_down_status[0x4] = true;
    else key_down_status[0x4] = false;
    if (IsKeyDown(KEY_W)) key_down_status[0x5] = true;
    else key_down_status[0x5] = false;
    if (IsKeyDown(KEY_E)) key_down_status[0x6] = true;
    else key_down_status[0x6] = false;
    if (IsKeyDown(KEY_R)) key_down_status[0xd] = true;
    else key_down_status[0xd] = false;

    if (IsKeyDown(KEY_A)) key_down_status[0x7] = true;
    else key_down_status[0x7] = false;
    if (IsKeyDown(KEY_S)) key_down_status[0x8] = true;
    else key_down_status[0x8] = false;
    if (IsKeyDown(KEY_D)) key_down_status[0x9] = true;
    else key_down_status[0x9] = false;
    if (IsKeyDown(KEY_F)) key_down_status[0xe] = true;
    else key_down_status[0xe] = false;

    if (IsKeyDown(KEY_Z)) key_down_status[0xa] = true;
    else key_down_status[0xa] = false;
    if (IsKeyDown(KEY_X)) key_down_status[0x0] = true;
    else key_down_status[0x0] = false;
    if (IsKeyDown(KEY_C)) key_down_status[0xb] = true;
    else key_down_status[0xb] = false;
    if (IsKeyDown(KEY_V)) key_down_status[0xf] = true;
    else key_down_status[0xf] = false;
}

void process_opcode() {
    uint16_t opcode = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;
    /*
     * no bits are lost using uint8, because the expression is evaluated first.
     * X: The second nibble. Used to look up one of the 16 registers (VX) from V0 through VF.
     * Y: The third nibble. Also used to look up one of the 16 registers (VY) from V0 through VF.
     * N: The fourth nibble. A 4-bit number.
     * NN: The second byte (third and fourth nibbles). An 8-bit immediate number.
     * NNN: The second, third and fourth nibbles. A 12-bit immediate memory address.
    */
    uint8_t x = (opcode & 0x0F00) >> 8;
	uint8_t y = (opcode & 0x00F0) >> 4;
	uint8_t n = (opcode & 0x000F);
	uint8_t nn = (opcode & 0x00FF);
    uint16_t nnn = (opcode & 0x0FFF);

    // opcode & 0xF000 is the first nibble (first 4 bits)
    switch (opcode & 0xF000) { 
        case 0x0000: 
            switch (opcode & 0x00FF) {
                // 00E0
                case 0x00E0:
                    // clear screen
                    memset(frame_buffer, 0, sizeof(frame_buffer));
                    break;

                // 00EE
                case 0x00EE:
                    pc = pop(&stack);
                    break;
            }
            break;

        // 1nnn
        case 0x1000:
            pc = nnn;
            break;

        // 2nnn
        case 0x2000:
            push(&stack, pc);
            pc = nnn;
            break;

        // 3xnn
        case 0x3000:
            if (v_registers[x] == nn) pc += 2;
            break;

        // 4xnn
        case 0x4000:
            if (v_registers[x] != nn) pc += 2;
            break;

        // 5xy0
        case 0x5000:
            if (v_registers[x] == v_registers[y]) pc += 2;
            break;

        // 9xy0
        case 0x9000:
            if (v_registers[x] != v_registers[y]) pc += 2;
            break;

        // 6xnn
        case 0x6000:
            v_registers[x] = nn;
            break;
        
        // 7xnn
        case 0x7000:
            v_registers[x] += nn;
            break;

        // Annn
        case 0xA000:
            I = nnn;
            break;
        
        // Dxyn
        case 0xD000: {
            uint8_t x_coordinate = v_registers[x];
            uint8_t y_coordinate = v_registers[y];
            x_coordinate = x_coordinate % 64;
            y_coordinate = y_coordinate % 32;
            v_registers[15] = 0;
            for (int i = 0; i < n; i++) {
                if (y_coordinate + i >= 32) break;
                uint8_t sprite_data = memory[I + i]; // each bit in this byte is a pixel
                for (int bit_index = 0; bit_index < 8; bit_index++) {
                    if (x_coordinate + bit_index >= 64) break; 
                    if (sprite_data & 0x80) { // test for the leftmost bit
                        bool target_bit_status = frame_buffer[y_coordinate + i][x_coordinate + bit_index];
                        if (target_bit_status) {
                            frame_buffer[y_coordinate + i][x_coordinate + bit_index] = 0;
                            v_registers[15] = 1;
                        }
                        else {
                            frame_buffer[y_coordinate + i][x_coordinate + bit_index] = 1;
                        }
                    }
                    sprite_data = sprite_data << 1;
                }
            }
            break;
        }

        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000:
                    v_registers[x] = v_registers[y];
                    break;

                case 0x0001:
                    v_registers[x] = v_registers[x] | v_registers[y];
                    break;

                case 0x0002:
                    v_registers[x] = v_registers[x] & v_registers[y];
                    break;

                case 0x0003:
                    v_registers[x] = v_registers[x] ^ v_registers[y];
                    break;

                case 0x0004:
                    if (v_registers[x] + v_registers[y] > 255) v_registers[15] = 1;
                    else v_registers[15] = 0;
                    v_registers[x] += v_registers[y];
                    break;

                case 0x0005:
                    if (v_registers[x] >= v_registers[y]) v_registers[15] = 1; // no borrowing occurs
                    else v_registers[15] = 0;
                    v_registers[x] = v_registers[x] - v_registers[y];
                    break;

                case 0x0007:
                    if (v_registers[y] >= v_registers[x]) v_registers[15] = 1; // no borrowing occurs
                    else v_registers[15] = 0;
                    v_registers[x] = v_registers[y] - v_registers[x];
                    break;

                case 0x0006: {
                    // uncomment for compatability
                    // v_registers[x] = v_registers[y];       
                    uint8_t rightmost_bit = v_registers[x] & 0x01;
                    v_registers[x] = v_registers[x] >> 1;
                    v_registers[15] = rightmost_bit;
                    break;
                }

                case 0x000E: {
                    // uncomment for compatability
                    // v_registers[x] = v_registers[y];       
                    uint8_t leftmost_bit = (v_registers[x] & 0x80) >> 7;
                    v_registers[x] = v_registers[x] << 1;
                    v_registers[15] = leftmost_bit;
                    break;
                }
            }
            break;

        // bnnn
        case 0xB000:
            pc = nnn + v_registers[0];
            break;

        // cxnn
        case 0xC000: 
            v_registers[x] = nn & ((uint8_t) (rand() % 256));
            break;
        
        case 0xE000:
            switch (opcode & 0x00FF) {
                // EX9E
                case 0x009E:
                    if (key_down_status[v_registers[x]]) pc += 2;
                    break;
                
                // EXA1
                case 0x00A1:
                    if (!key_down_status[v_registers[x]]) pc += 2;
                    break;
            }
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                // FX07
                case 0x0007:
                    v_registers[x] = delay_timer;
                    break;

                // FX15
                case 0x0015:
                    delay_timer = v_registers[x];
                    break;
                
                // FX18
                case 0x0018:
                    sound_timer = v_registers[x];
                    break;

                // FX1E
                case 0x001E:
                    I += v_registers[x];
                    break;

                // FX0A
                case 0x000A:
                    bool key_pressed = false;
                    uint8_t pressed_key = -1;
                    for (int i = 0; i <= 0xF; i++) {
                        if (key_down_status[i]) {
                            key_pressed = true;
                            pressed_key = i;
                            break;
                        } 
                    }
                    if (!key_pressed) pc -= 2;
                    else v_registers[x] = pressed_key;
                    break;

                // FX29
                case 0x0029:
                    I = 0x50 + (v_registers[x] * 5);
                    break;

                // FX33
                case 0x0033: {
                    uint8_t number = v_registers[x];
                    uint8_t hundred = number / 100;
                    number = number % 100;
                    uint8_t ten = number / 10;
                    number = number % 10;
                    uint8_t one = number;
                    memory[I] = hundred;
                    memory[I + 1] = ten;
                    memory[I + 2] = one;
                    break;
                }

                // FX55
                case 0x0055:
                    for (int i = 0; i <= x; i++) memory[I + i] = v_registers[i];
                    break;

                // FX65
                case 0x0065:
                    for (int i = 0; i <= x; i++) v_registers[i] = memory[I + i];
                    break;
            }
            break;
    }

}

int main(int argc, char *argv[]) {
    init();
    if (argc == 1) return 0;
    load_rom(argv[1]);
    InitWindow(256, 128, "chip 8 emulator");
    SetTargetFPS(60);
    InitAudioDevice();
    Sound beep = LoadSound("beep.wav");

    while (!WindowShouldClose()) {
        update_key_downs();
        for (int i = 0; i < 10; i++) process_opcode();
        if (delay_timer > 0) delay_timer--;
        if (sound_timer > 0) {
            sound_timer--;
            PlaySound(beep);
        }
        BeginDrawing();
        for (int y = 0; y < 128; y += 4) {
            for (int x = 0; x < 256; x += 4) {
                if (frame_buffer[y / 4][x / 4]) {
                    for (int dy = 0; dy < 4; dy++) {
                        for (int dx = 0; dx < 4; dx++) {
                            DrawPixel(x + dx, y + dy, GREEN);
                        }
                    }
                }
                else {
                    for (int dy = 0; dy < 4; dy++) {
                        for (int dx = 0; dx < 4; dx++) {
                            DrawPixel(x + dx, y + dy, BLACK);
                        }
                    }
                }
            }
        }
        EndDrawing();
    }
    UnloadSound(beep);
    CloseAudioDevice();
    CloseWindow();
}

