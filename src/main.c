#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu.h"

#define NES_WIDTH  256
#define NES_HEIGHT 240
#define SCALE      3

static void test_cpu(void) {
    printf("=== CPU Tests ===\n");

    CPU cpu;
    cpu_init(&cpu);

    // Тест 1: Запись/чтение памяти
    cpu_write(&cpu, 0x0000, 0x42);
    cpu_write(&cpu, 0x0001, 0xAB);
    cpu_write(&cpu, 0x1234, 0xCD);

    if (cpu_read(&cpu, 0x0000) != 0x42) {
        printf("FAIL: memory write/read at 0x0000\n");
    } else if (cpu_read(&cpu, 0x1234) != 0xCD) {
        printf("FAIL: memory write/read at 0x1234\n");
    } else {
        printf("PASS: memory write/read\n");
    }

    // Тест 2: Чтение 16-бит (little-endian)
    cpu_write(&cpu, 0x8000, 0x34);
    cpu_write(&cpu, 0x8001, 0x12);
    uint16_t val16 = cpu_read16(&cpu, 0x8000);
    if (val16 != 0x1234) {
        printf("FAIL: read16 expected 0x1234, got 0x%04X\n", val16);
    } else {
        printf("PASS: read16 (little-endian)\n");
    }

    // Тест 3: Флаги
    cpu_set_flag(&cpu, FLAG_C, true);
    if (!cpu_get_flag(&cpu, FLAG_C)) {
        printf("FAIL: set/get carry flag\n");
    } else {
        printf("PASS: set/get carry flag\n");
    }

    cpu_set_flag(&cpu, FLAG_C, false);
    if (cpu_get_flag(&cpu, FLAG_C)) {
        printf("FAIL: clear carry flag\n");
    } else {
        printf("PASS: clear carry flag\n");
    }

    // Тест 4: Z и N флаги
    cpu_update_zn(&cpu, 0x00);
    if (!cpu_get_flag(&cpu, FLAG_Z) || cpu_get_flag(&cpu, FLAG_N)) {
        printf("FAIL: update_zn for 0x00\n");
    } else {
        printf("PASS: update_zn for 0x00 (Z=1, N=0)\n");
    }

    cpu_update_zn(&cpu, 0x80);
    if (cpu_get_flag(&cpu, FLAG_Z) || !cpu_get_flag(&cpu, FLAG_N)) {
        printf("FAIL: update_zn for 0x80\n");
    } else {
        printf("PASS: update_zn for 0x80 (Z=0, N=1)\n");
    }

    cpu_update_zn(&cpu, 0x42);
    if (cpu_get_flag(&cpu, FLAG_Z) || cpu_get_flag(&cpu, FLAG_N)) {
        printf("FAIL: update_zn for 0x42\n");
    } else {
        printf("PASS: update_zn for 0x42 (Z=0, N=0)\n");
    }

    // Тест 5: Reset
    cpu_reset(&cpu);
    if (cpu.pc != 0x0000 || cpu.sp != 0xFD || cpu.a != 0) {
        printf("FAIL: cpu_reset\n");
    } else {
        printf("PASS: cpu_reset\n");
    }

    printf("=== All tests done ===\n");
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    test_cpu();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "NES Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        NES_WIDTH * SCALE,
        NES_HEIGHT * SCALE,
        SDL_WINDOW_SHOWN
    );
    if (!window) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        NES_WIDTH,
        NES_HEIGHT
    );
    if (!texture) {
        fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    uint32_t pixels[NES_WIDTH * NES_HEIGHT];
    for (int y = 0; y < NES_HEIGHT; y++) {
        for (int x = 0; x < NES_WIDTH; x++) {
            uint8_t r = (uint8_t)(x * 255 / NES_WIDTH);
            uint8_t g = (uint8_t)(y * 255 / NES_HEIGHT);
            uint8_t b = 128;
            pixels[y * NES_WIDTH + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        }
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        SDL_UpdateTexture(texture, NULL, pixels, NES_WIDTH * sizeof(uint32_t));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}