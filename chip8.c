#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <SDL.h>

#define MEMORY_SIZE 4096
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define PIXEL_SCALE   10  // Scale factor (adjust as needed)

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
uint8_t framebuffer[DISPLAY_WIDTH][DISPLAY_HEIGHT]; // CHIP-8 Framebuffer
uint8_t key[16] = {0}; // Store the state of each key

int running = 1;

SDL_Window *window;
SDL_Renderer *renderer;

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
    printf("[DEBUG] Rendering screen...\n");
    // Set the background color (black)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Set the draw color for pixels (white)
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    // Loop through the framebuffer and draw pixels
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            if (framebuffer[x][y] == 1) {  // Properly accessing 2D array
                SDL_Rect pixel_rect = {
                    x * PIXEL_SCALE,
                    y * PIXEL_SCALE,
                    PIXEL_SCALE,
                    PIXEL_SCALE
                };
                SDL_RenderFillRect(renderer, &pixel_rect);
            }
        }
    }

    // Present the updated screen
    SDL_RenderPresent(renderer);
}

void handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = 0;  // Quit when window is closed
        } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            int keyState = (event.type == SDL_KEYDOWN) ? 1 : 0;
            switch (event.key.keysym.sym) {
                case SDLK_1: key[0x1] = keyState; break;
                case SDLK_2: key[0x2] = keyState; break;
                case SDLK_3: key[0x3] = keyState; break;
                case SDLK_4: key[0xC] = keyState; break;
                case SDLK_q: key[0x4] = keyState; break;
                case SDLK_w: key[0x5] = keyState; break;
                case SDLK_e: key[0x6] = keyState; break;
                case SDLK_r: key[0xD] = keyState; break;
                case SDLK_a: key[0x7] = keyState; break;
                case SDLK_s: key[0x8] = keyState; break;
                case SDLK_d: key[0x9] = keyState; break;
                case SDLK_f: key[0xE] = keyState; break;
                case SDLK_z: key[0xA] = keyState; break;
                case SDLK_x: key[0x0] = keyState; break;
                case SDLK_c: key[0xB] = keyState; break;
                case SDLK_v: key[0xF] = keyState; break;
                case SDLK_ESCAPE: running = 0; break;  // Quit with ESC
            }
        }
    }
}

void executeCycle() {
    printf("[DEBUG] PC Before Execution: 0x%03X\n", PC);
    uint16_t opcode = (memory[PC] << 8) | memory[PC + 1];
    printf("Executing opcode: 0x%04X at PC=0x%03X\n", opcode, PC);

    switch (opcode & 0xF000) {
        case 0xA000:
            I = opcode & 0x0FFF;
            printf("[DEBUG] Set I = 0x%X (Memory[I] = 0x%02X)\n", I, memory[I]);
            PC += 2;
            break;

        case 0x0000:
            if (opcode == 0x00E0) {
                memset(framebuffer, 0, sizeof(framebuffer));
                PC += 2;
            } else if (opcode == 0x00EE) {
                PC = stack[--SP];
                PC += 2;
            }
            break;

        case 0x1000:
            PC = opcode & 0x0FFF;
            printf("[DEBUG] Jump to PC=0x%03X\n", PC);
            break;

        case 0x2000:
            stack[SP++] = PC + 2;
            PC = opcode & 0x0FFF;
            printf("[DEBUG] Call Subroutine: New PC=0x%03X\n", PC);
            break;

        case 0x6000:
            V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            PC += 2;
            break;

        case 0xD000: {
            uint8_t x = V[(opcode & 0x0F00) >> 8] % DISPLAY_WIDTH;
            uint8_t y = V[(opcode & 0x00F0) >> 4] % DISPLAY_HEIGHT;
            uint8_t height = opcode & 0x000F;
            printf("[DEBUG] Drawing sprite at (%d, %d) with height %d\n", x, y, height);
            printf("[DEBUG] Sprite Data at I=%X: ", I);
            for (int i = 0; i < height; i++) {
                printf("%02X ", memory[I + i]);
            }
            printf("\n");

            V[0xF] = 0;
            
            for (int row = 0; row < height; row++) {
                uint8_t spriteByte = memory[I + row]; // Get the sprite data from memory

                for (int col = 0; col < 8; col++) {
                    if ((spriteByte & (0x80 >> col)) != 0) {  // Check each bit in the sprite row
                        int pixelX = (x + col) % DISPLAY_WIDTH;
                        int pixelY = (y + row) % DISPLAY_HEIGHT;

                        // XOR the pixel in the framebuffer
                        framebuffer[pixelX][pixelY] ^= 1;

                        // Collision detection (if we erase a pixel, VF is set to 1)
                        if (framebuffer[pixelX][pixelY] == 0) {
                            V[0xF] = 1; 
                        }
                    }
                }
            }


            PC += 2;
            break;
        }

        case 0x7000:
            V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
            PC += 2;
            break;

        case 0xF000:
            switch (opcode & 0x00FF) {
                case 0x0015:
                    delay_timer = V[(opcode & 0x0F00) >> 8];
                    PC += 2;
                    break;

                // Add more 0xFxxx opcodes here as needed
                default:
                    printf("Unknown opcode: 0x%X\n", opcode);
            }
            break;

        case 0x3000: // 3XNN: Skip if VX == NN
            if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF)) {
                PC += 4;
            } else {
                PC += 2;
            }
            break;

        case 0x4000: // 4XNN: Skip if VX != NN
            if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF)) {
                PC += 4;
            } else {
                PC += 2;
            }
            break;

        case 0xE000:
            switch (opcode & 0x00FF) {
                case 0x009E:
                    if (key[V[(opcode & 0x0F00) >> 8]]) {
                        PC += 4; // Skip next instruction
                    } else {
                        PC += 2;
                    }
                    break;

                case 0x00A1:
                    if (!key[V[(opcode & 0x0F00) >> 8]]) {
                        PC += 4;
                    } else {
                        PC += 2;
                    }
                    break;

                default:
                    printf("Unknown opcode: 0x%X\n", opcode);
                    PC += 2;
            }
            break;

        // ... (keep other cases)
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
    memset(framebuffer, 0, sizeof(framebuffer));
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

int initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL Initialization failed: %s\n", SDL_GetError());
    return 1;
    }

    // Create SDL Window
    window = SDL_CreateWindow("CHIP-8 Emulator",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              DISPLAY_WIDTH * PIXEL_SCALE, DISPLAY_HEIGHT * PIXEL_SCALE,
                              SDL_WINDOW_SHOWN);

    if (!window) {
        printf("SDL Window creation failed: %s\n", SDL_GetError());
        return 1;
    }

    // Create SDL Renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("SDL Renderer creation failed: %s\n", SDL_GetError());
        return 1;
    }

    printf("[Init] SDL initialized!\n");
    return 0;
}

int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    printf("[Init] Starting emulator...\n");

    if (argc < 2) {
        printf("[Warning] Usage: %s <ROM file>\n", argv[0]);
        return 1;
    }

    if (initSDL() != 0) {
        return 1;  // SDL failed, exit
    }


    printf("[Init] Loading ROM: %s\n", argv[1]);
    initialize();
    loadROM(argv[1]);
    //run();

    while (running) {
        handleInput();
        executeCycle();
        SDL_Delay(1600);      // ~60Hz timing
        renderScreen(0);    // Ensure the framebuffer updates
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    printf("[Shutdown] SDL closed, exiting.\n");
    return 0;
}