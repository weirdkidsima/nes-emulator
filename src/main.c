#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu.h"

#define NES_WIDTH  256
#define NES_HEIGHT 240
#define SCALE      3

static void test_stack(void) {
    printf("\n=== Stack Tests ===\n");
    CPU cpu;
    cpu_init(&cpu);
    cpu.sp = 0xFD;

    // PHA: положить A в стек
    cpu.a = 0x42;
    uint8_t prog1[] = { 0x48 }; // PHA
    cpu_write(&cpu, 0x8000, prog1[0]);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: PHA -> mem[0x01FD] = 0x42 (got 0x%02X)\n",
        (cpu_read(&cpu, 0x01FD) == 0x42) ? "PASS" : "FAIL", cpu_read(&cpu, 0x01FD));
    printf("%s: PHA -> SP = 0xFC (got 0x%02X)\n",
        (cpu.sp == 0xFC) ? "PASS" : "FAIL", cpu.sp);

    // PLA: снять со стека
    cpu.a = 0x00;
    uint8_t prog2[] = { 0x68 }; // PLA
    cpu_write(&cpu, 0x8000, prog2[0]);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: PLA -> A = 0x42 (got 0x%02X)\n",
        (cpu.a == 0x42) ? "PASS" : "FAIL", cpu.a);
    printf("%s: PLA -> SP = 0xFD (got 0x%02X)\n",
        (cpu.sp == 0xFD) ? "PASS" : "FAIL", cpu.sp);

    // PHP/PLP
    cpu.p = FLAG_U | FLAG_I | FLAG_C;
    cpu_write(&cpu, 0x8000, 0x08); // PHP
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: PHP -> SP = 0xFC (got 0x%02X)\n",
        (cpu.sp == 0xFC) ? "PASS" : "FAIL", cpu.sp);

    cpu.p = FLAG_U; // Сбросим всё
    cpu_write(&cpu, 0x8000, 0x28); // PLP
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: PLP -> C = 1 (got %d)\n",
        (cpu_get_flag(&cpu, FLAG_C)) ? "PASS" : "FAIL", cpu_get_flag(&cpu, FLAG_C));
    printf("%s: PLP -> B cleared\n",
        (!cpu_get_flag(&cpu, FLAG_B)) ? "PASS" : "FAIL");
}

static void test_jsr_rts(void) {
    printf("\n=== JSR/RTS Tests ===\n");
    CPU cpu;
    cpu_init(&cpu);

    // Программа:
    // 0x8000: JSR $8010   ; вызов подпрограммы
    // 0x8003: LDA #$55    ; после возврата
    // 0x8005: JMP $8005   ; стоп (цикл)
    // ...
    // 0x8010: LDA #$42    ; подпрограмма
    // 0x8012: RTS

    uint8_t program[] = {
        0x20, 0x10, 0x80, // 0x8000: JSR $8010
        0xA9, 0x55,       // 0x8003: LDA #$55
        0x4C, 0x05, 0x80, // 0x8005: JMP $8005
    };
    for (size_t i = 0; i < sizeof(program); i++) cpu_write(&cpu, 0x8000 + i, program[i]);

    uint8_t sub[] = {
        0xA9, 0x42, // 0x8010: LDA #$42
        0x60,       // 0x8012: RTS
    };
    for (size_t i = 0; i < sizeof(sub); i++) cpu_write(&cpu, 0x8010 + i, sub[i]);

    cpu.pc = 0x8000;
    cpu_step(&cpu); // JSR
    printf("%s: JSR -> PC = 0x8010 (got 0x%04X)\n",
        (cpu.pc == 0x8010) ? "PASS" : "FAIL", cpu.pc);

    cpu_step(&cpu); // LDA #$42
    printf("%s: sub LDA -> A = 0x42 (got 0x%02X)\n",
        (cpu.a == 0x42) ? "PASS" : "FAIL", cpu.a);

    cpu_step(&cpu); // RTS
    printf("%s: RTS -> PC = 0x8003 (got 0x%04X)\n",
        (cpu.pc == 0x8003) ? "PASS" : "FAIL", cpu.pc);

    cpu_step(&cpu); // LDA #$55
    printf("%s: after RTS LDA -> A = 0x55 (got 0x%02X)\n",
        (cpu.a == 0x55) ? "PASS" : "FAIL", cpu.a);
}

static void test_flags(void) {
    printf("\n=== Flag Instructions ===\n");
    CPU cpu;
    cpu_init(&cpu);

    cpu.p = 0;
    cpu_write(&cpu, 0x8000, 0x38); // SEC
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: SEC -> C = 1\n", cpu_get_flag(&cpu, FLAG_C) ? "PASS" : "FAIL");

    cpu_write(&cpu, 0x8000, 0x18); // CLC
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: CLC -> C = 0\n", !cpu_get_flag(&cpu, FLAG_C) ? "PASS" : "FAIL");

    cpu_write(&cpu, 0x8000, 0x78); // SEI
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: SEI -> I = 1\n", cpu_get_flag(&cpu, FLAG_I) ? "PASS" : "FAIL");

    cpu_write(&cpu, 0x8000, 0x58); // CLI
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: CLI -> I = 0\n", !cpu_get_flag(&cpu, FLAG_I) ? "PASS" : "FAIL");

    cpu_set_flag(&cpu, FLAG_V, true);
    cpu_write(&cpu, 0x8000, 0xB8); // CLV
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: CLV -> V = 0\n", !cpu_get_flag(&cpu, FLAG_V) ? "PASS" : "FAIL");
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    test_stack();
    test_jsr_rts();
    test_flags();

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