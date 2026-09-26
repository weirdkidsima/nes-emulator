#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu.h"

#define NES_WIDTH  256
#define NES_HEIGHT 240
#define SCALE      3

static void test_cpu_basic(void) {
    printf("=== CPU Basic Tests ===\n");
    CPU cpu;
    cpu_init(&cpu);

    cpu_write(&cpu, 0x0000, 0x42);
    cpu_write(&cpu, 0x1234, 0xCD);
    printf("%s: memory write/read\n",
        (cpu_read(&cpu, 0x0000) == 0x42 && cpu_read(&cpu, 0x1234) == 0xCD) ? "PASS" : "FAIL");

    cpu_write(&cpu, 0x8000, 0x34);
    cpu_write(&cpu, 0x8001, 0x12);
    printf("%s: read16 little-endian\n",
        (cpu_read16(&cpu, 0x8000) == 0x1234) ? "PASS" : "FAIL");

    cpu_set_flag(&cpu, FLAG_C, true);
    printf("%s: set carry\n", cpu_get_flag(&cpu, FLAG_C) ? "PASS" : "FAIL");
    cpu_set_flag(&cpu, FLAG_C, false);
    printf("%s: clear carry\n", !cpu_get_flag(&cpu, FLAG_C) ? "PASS" : "FAIL");

    cpu_update_zn(&cpu, 0x00);
    printf("%s: ZN for 0x00\n",
        (cpu_get_flag(&cpu, FLAG_Z) && !cpu_get_flag(&cpu, FLAG_N)) ? "PASS" : "FAIL");
    cpu_update_zn(&cpu, 0x80);
    printf("%s: ZN for 0x80\n",
        (!cpu_get_flag(&cpu, FLAG_Z) && cpu_get_flag(&cpu, FLAG_N)) ? "PASS" : "FAIL");
}

static void test_cpu_program(void) {
    printf("\n=== CPU Program Test ===\n");
    CPU cpu;
    cpu_init(&cpu);

    uint8_t program[] = {
        0xA9, 0x42,       // LDA #$42
        0x8D, 0x00, 0x02, // STA $0200
        0xA2, 0x10,       // LDX #$10
        0x8E, 0x01, 0x02, // STX $0201
        0xA0, 0x20,       // LDY #$20
        0x8C, 0x02, 0x02, // STY $0202
        0x4C, 0x00, 0x80, // JMP $8000
    };

    for (size_t i = 0; i < sizeof(program); i++) {
        cpu_write(&cpu, 0x8000 + i, program[i]);
    }

    cpu.pc = 0x8000;

    for (int i = 0; i < 6; i++) {
        cpu_step(&cpu);
    }

    printf("%s: LDA #$42 -> A = 0x42 (got 0x%02X)\n",
        (cpu.a == 0x42) ? "PASS" : "FAIL", cpu.a);
    printf("%s: STA $0200 -> mem[0x0200] = 0x42 (got 0x%02X)\n",
        (cpu_read(&cpu, 0x0200) == 0x42) ? "PASS" : "FAIL", cpu_read(&cpu, 0x0200));
    printf("%s: LDX #$10 -> X = 0x10 (got 0x%02X)\n",
        (cpu.x == 0x10) ? "PASS" : "FAIL", cpu.x);
    printf("%s: STX $0201 -> mem[0x0201] = 0x10 (got 0x%02X)\n",
        (cpu_read(&cpu, 0x0201) == 0x10) ? "PASS" : "FAIL", cpu_read(&cpu, 0x0201));
    printf("%s: LDY #$20 -> Y = 0x20 (got 0x%02X)\n",
        (cpu.y == 0x20) ? "PASS" : "FAIL", cpu.y);
    printf("%s: STY $0202 -> mem[0x0202] = 0x20 (got 0x%02X)\n",
        (cpu_read(&cpu, 0x0202) == 0x20) ? "PASS" : "FAIL", cpu_read(&cpu, 0x0202));

    cpu_step(&cpu);
    printf("%s: JMP $8000 -> PC = 0x8000 (got 0x%04X)\n",
        (cpu.pc == 0x8000) ? "PASS" : "FAIL", cpu.pc);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    test_cpu_basic();
    test_cpu_program();

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("NES Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        NES_WIDTH * SCALE, NES_HEIGHT * SCALE, SDL_WINDOW_SHOWN);
    if (!window) { SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, NES_WIDTH, NES_HEIGHT);

    uint32_t pixels[NES_WIDTH * NES_HEIGHT];
    for (int y = 0; y < NES_HEIGHT; y++)
        for (int x = 0; x < NES_WIDTH; x++) {
            uint8_t r = (uint8_t)(x * 255 / NES_WIDTH);
            uint8_t g = (uint8_t)(y * 255 / NES_HEIGHT);
            pixels[y * NES_WIDTH + x] = (0xFF << 24) | (r << 16) | (g << 8) | 128;
        }

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) running = false;
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