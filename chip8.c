#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define MEMORY_SIZE 4096
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

// Function prototypes (declare functions before use)
void initialize();
void loadROM(const char *filename);
void executeCycle();
void renderScreen(int debug);
void run();

// CHIP-8 system memory & registers
uint8_t memory[MEMORY_SIZE];
uint8_t V[16];         // 16 registers (V0 - VF)
uint16_t I;            // Index register
uint16_t PC;           // Program counter
uint16_t stack[16];    // Stack (for subroutine calls)
uint8_t SP;            // Stack pointer
uint8_t delay_timer;   // Delay timer
uint8_t sound_timer;   // Sound timer
uint8_t display[DISPLAY_WIDTH * DISPLAY_HEIGHT];  // Monochrome framebuffer

// CHIP-8 fontset (stored at the start of memory)
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

void chip8_sleep(int microseconds) {
    struct timespec ts;
    ts.tv_sec = microseconds / 1000000;  // Convert to seconds
    ts.tv_nsec = (microseconds % 1000000) * 1000;  // Convert to nanoseconds
    nanosleep(&ts, NULL);
}

void renderScreen(int debug) {
    printf("\033[H\033[J");
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            printf(display[y * DISPLAY_WIDTH + x] ? "#" : ".");
        }
        printf("\n");
    }
    if (debug) {
        chip8_sleep(50000);
    }
}

void executeCycle() {
    printf("[DEBUG] PC Before Execution: 0x%03X\n", PC);
    // Fetch opcode (each instruction is 2 bytes)
    uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];

    printf("Executing opcode: 0x%04X at PC=0x%03X\n", opcode, PC);

    // Decode & Execute
    switch (opcode & 0xF000) {
        case 0xA000:
            // ANNN: Set I = NNN
            I = opcode & 0x0FFF;
            printf("[DEBUG] Set I = 0x%X (Memory[I] = 0x%02X)\n", I, memory[I]);
            PC += 2;
            break;

        case 0x0000:
            if (opcode == 0x00E0) {
                // 00E0 - Clear screen
                memset(display, 0, sizeof(display));
                PC += 2;
            } else if (opcode == 0x00EE) {
                // 00EE - Return from subroutine
                PC = stack[--SP];
                PC += 2;
            }
            break;
        case 0x1000:
            // 1NNN - Jump to address NNN
            PC = opcode & 0x0FFF;
            printf("[DEBUG] Jump to PC=0x%03X\n", PC);
            break;

        case 0x2000:  // CALL NNN (Subroutine)
            stack[SP++] = PC + 2;
            PC = opcode & 0x0FFF;
            printf("[DEBUG] Call Subroutine: New PC=0x%03X\n", PC);
            break;

        case 0x6000:
            // 6XNN - Set VX = NN
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            PC += 2;
            break;

        case 0xD000: {  // DXYN - Draw sprite at (VX, VY)
            uint8_t x = V[(opcode & 0x0F00) >> 8] % DISPLAY_WIDTH;
            uint8_t y = V[(opcode & 0x00F0) >> 4] % DISPLAY_HEIGHT;
            uint8_t height = opcode & 0x000F;

            printf("[DEBUG] Drawing sprite at (%d, %d) with height %d\n", x, y, height);
            printf("[DEBUG] Sprite Data at I=%X: ", I);
            
            for (int i = 0; i < height; i++) {
                printf("%02X ", memory[I + i]);  // Print sprite row data
            }
            
            printf("\n");

            // Sprite Drawing Code (unchanged)
            V[0xF] = 0; // Reset collision flag
            for (int row = 0; row < height; row++) {
                uint8_t spriteByte = memory[I + row];  // Read sprite row from memory

                for (int col = 0; col < 8; col++) {
                    if ((spriteByte & (0x80 >> col)) != 0) {  // Check if bit is set
                        int index = (y + row) * DISPLAY_WIDTH + (x + col);

                        if (display[index] == 1) {
                            V[0xF] = 1;  // Collision detected
                        }

                        display[index] ^= 1;  // XOR toggle pixel
                    }
                }
            }

            PC += 2;
            break;
        }

        case 0x7000:
            // 7XNN: Add NN to VX (without carry)
            V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
            PC += 2;
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x15:
                    // FX15: Set delay timer = VX
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                    PC += 2;
                    break;
            }
            break;
        
        default:
            printf("[WARNING] Unknown opcode: 0x%04X at PC=0x%03X\n", opcode, PC);
            PC += 2;
    }
}

void loadROM(const char *filename) {
    FILE *rom = fopen(filename, "rb");
    if (!rom) {
        printf("Error: Cannot open ROM file: %s\n", filename);
        exit(1);
    }

    fread(memory + 0x200, 1, MEMORY_SIZE - 0x200, rom);
    fclose(rom);

    printf("[Init] Loaded ROM: First 15 bytes at 0x200: ");
    for (int i = 0; i < 15; i++) {
        printf("%02X ", memory[0x200 + i]);
    }
    printf("\n");
    printf("[DEBUG] Checking Memory at 0x200 - 0x210:\n");
    for (int i = 0; i < 16; i++) {
        printf("0x%03X: 0x%02X\n", 0x200 + i, memory[0x200 + i]);
    }
}

// Initialize CHIP-8 state
void initialize() {
    PC = 0x200;  // Programs start at 0x200
    I = 0;
    SP = 0;
    delay_timer = 0;
    sound_timer = 0;

    memset(memory, 0, sizeof(memory));
    printf("[Init] CHIP-8 Initialized. Starting PC = 0x%X\n", PC);
    memset(V, 0, sizeof(V));
    memset(display, 0, sizeof(display));
    memset(stack, 0, sizeof(stack));

    // Load fontset into memory
    memcpy(memory, fontset, sizeof(fontset));
}

void run() {
    while (1) {
        executeCycle();

        // Update timers
        if (delay_timer > 0) delay_timer--;
        if (sound_timer > 0) sound_timer--;

        renderScreen(0); // Update display
        chip8_sleep(16000);
    }
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("[Warning] Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }

    setbuf(stdout, NULL);
    printf("[Init] Loading ROM: %s\n", argv[1]);
    initialize();
    loadROM(argv[1]);
    run();

    return 0;
}