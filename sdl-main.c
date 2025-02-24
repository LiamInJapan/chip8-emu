#include <stdio.h>
#include <SDL.h>

int main(int argc, char **argv) {
    setbuf(stdout, NULL);
    printf("[Init] Starting emulator...\n");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }
    printf("[Init] SDL initialized!\n");

    SDL_Window *win = SDL_CreateWindow(
        "CHIP-8 Test", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        640, 320, SDL_WINDOW_SHOWN
    );
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("[Init] Window created!\n");

    SDL_Event e;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = 0;
        }
        SDL_Delay(16);
    }

    SDL_DestroyWindow(win);
    SDL_Quit();
    printf("[Shutdown] SDL closed, exiting.\n");
    return 0;
}