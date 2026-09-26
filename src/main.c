#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu.h"

#define NES_WIDTH  256
#define NES_HEIGHT 240
#define SCALE      3

static void test_arithmetic(void) {
    printf("\n=== Arithmetic Tests ===\n");
    CPU cpu;

    // ADC: 0x10 + 0x20 = 0x30, без переноса
    cpu_init(&cpu);
    cpu.a = 0x10;
    cpu_set_flag(&cpu, FLAG_C, false);
    uint8_t prog1[] = { 0x69, 0x20 }; // ADC #$20
    for (size_t i = 0; i < sizeof(prog1); i++) cpu_write(&cpu, 0x8000 + i, prog1[i]);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: ADC 0x10+0x20 = 0x30 (got 0x%02X)\n",
        (cpu.a == 0x30) ? "PASS" : "FAIL", cpu.a);
    printf("%s: ADC carry = 0\n", (!cpu_get_flag(&cpu, FLAG_C)) ? "PASS" : "FAIL");

    // ADC: 0xFF + 0x01 = 0x00, carry = 1, zero = 1
    cpu_init(&cpu);
    cpu.a = 0xFF;
    cpu_set_flag(&cpu, FLAG_C, false);
    for (size_t i = 0; i < sizeof(prog1); i++) cpu_write(&cpu, 0x8000 + i, prog1[i]);
    cpu_write(&cpu, 0x8001, 0x01);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: ADC 0xFF+0x01 = 0x00 (got 0x%02X)\n",
        (cpu.a == 0x00) ? "PASS" : "FAIL", cpu.a);
    printf("%s: ADC carry = 1\n", (cpu_get_flag(&cpu, FLAG_C)) ? "PASS" : "FAIL");
    printf("%s: ADC zero = 1\n", (cpu_get_flag(&cpu, FLAG_Z)) ? "PASS" : "FAIL");

    // ADC: 0x50 + 0x50 = 0xA0, overflow = 1
    cpu_init(&cpu);
    cpu.a = 0x50;
    cpu_set_flag(&cpu, FLAG_C, false);
    for (size_t i = 0; i < sizeof(prog1); i++) cpu_write(&cpu, 0x8000 + i, prog1[i]);
    cpu_write(&cpu, 0x8001, 0x50);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: ADC 0x50+0x50 = 0xA0 (got 0x%02X)\n",
        (cpu.a == 0xA0) ? "PASS" : "FAIL", cpu.a);
    printf("%s: ADC overflow = 1\n", (cpu_get_flag(&cpu, FLAG_V)) ? "PASS" : "FAIL");

    // SBC: 0x50 - 0x10 = 0x40
    cpu_init(&cpu);
    cpu.a = 0x50;
    cpu_set_flag(&cpu, FLAG_C, true);
    uint8_t prog2[] = { 0xE9, 0x10 }; // SBC #$10
    for (size_t i = 0; i < sizeof(prog2); i++) cpu_write(&cpu, 0x8000 + i, prog2[i]);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: SBC 0x50-0x10 = 0x40 (got 0x%02X)\n",
        (cpu.a == 0x40) ? "PASS" : "FAIL", cpu.a);
    printf("%s: SBC carry = 1 (no borrow)\n", (cpu_get_flag(&cpu, FLAG_C)) ? "PASS" : "FAIL");

    // SBC: 0x10 - 0x20 = 0xF0, carry = 0
    cpu_init(&cpu);
    cpu.a = 0x10;
    cpu_set_flag(&cpu, FLAG_C, true);
    for (size_t i = 0; i < sizeof(prog2); i++) cpu_write(&cpu, 0x8000 + i, prog2[i]);
    cpu_write(&cpu, 0x8001, 0x20);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: SBC 0x10-0x20 = 0xF0 (got 0x%02X)\n",
        (cpu.a == 0xF0) ? "PASS" : "FAIL", cpu.a);
    printf("%s: SBC carry = 0 (borrow)\n", (!cpu_get_flag(&cpu, FLAG_C)) ? "PASS" : "FAIL");
}

static void test_logic(void) {
    printf("\n=== Logic Tests ===\n");
    CPU cpu;

    // AND: 0xF0 & 0x0F = 0x00, Z=1
    cpu_init(&cpu);
    cpu.a = 0xF0;
    uint8_t prog[] = { 0x29, 0x0F };
    for (size_t i = 0; i < sizeof(prog); i++) cpu_write(&cpu, 0x8000 + i, prog[i]);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: AND 0xF0 & 0x0F = 0x00 (got 0x%02X)\n",
        (cpu.a == 0x00) ? "PASS" : "FAIL", cpu.a);
    printf("%s: AND zero = 1\n", (cpu_get_flag(&cpu, FLAG_Z)) ? "PASS" : "FAIL");

    // ORA: 0xF0 | 0x0F = 0xFF, N=1
    cpu_init(&cpu);
    cpu.a = 0xF0;
    uint8_t prog2[] = { 0x09, 0x0F };
    for (size_t i = 0; i < sizeof(prog2); i++) cpu_write(&cpu, 0x8000 + i, prog2[i]);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: ORA 0xF0 | 0x0F = 0xFF (got 0x%02X)\n",
        (cpu.a == 0xFF) ? "PASS" : "FAIL", cpu.a);
    printf("%s: ORA negative = 1\n", (cpu_get_flag(&cpu, FLAG_N)) ? "PASS" : "FAIL");

    // EOR: 0xFF ^ 0xFF = 0x00, Z=1
    cpu_init(&cpu);
    cpu.a = 0xFF;
    uint8_t prog3[] = { 0x49, 0xFF };
    for (size_t i = 0; i < sizeof(prog3); i++) cpu_write(&cpu, 0x8000 + i, prog3[i]);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: EOR 0xFF ^ 0xFF = 0x00 (got 0x%02X)\n",
        (cpu.a == 0x00) ? "PASS" : "FAIL", cpu.a);
}

static void test_compare(void) {
    printf("\n=== Compare Tests ===\n");
    CPU cpu;

    // CMP: 0x42 vs 0x42 -> Z=1, C=1
    cpu_init(&cpu);
    cpu.a = 0x42;
    uint8_t prog[] = { 0xC9, 0x42 };
    for (size_t i = 0; i < sizeof(prog); i++) cpu_write(&cpu, 0x8000 + i, prog[i]);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: CMP 0x42 vs 0x42 -> Z=1, C=1\n",
        (cpu_get_flag(&cpu, FLAG_Z) && cpu_get_flag(&cpu, FLAG_C)) ? "PASS" : "FAIL");

    // CMP: 0x42 vs 0x50 -> Z=0, C=0
    cpu_init(&cpu);
    cpu.a = 0x42;
    for (size_t i = 0; i < sizeof(prog); i++) cpu_write(&cpu, 0x8000 + i, prog[i]);
    cpu_write(&cpu, 0x8001, 0x50);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: CMP 0x42 vs 0x50 -> Z=0, C=0\n",
        (!cpu_get_flag(&cpu, FLAG_Z) && !cpu_get_flag(&cpu, FLAG_C)) ? "PASS" : "FAIL");

    // CMP: 0x50 vs 0x42 -> Z=0, C=1
    cpu_init(&cpu);
    cpu.a = 0x50;
    for (size_t i = 0; i < sizeof(prog); i++) cpu_write(&cpu, 0x8000 + i, prog[i]);
    cpu_write(&cpu, 0x8001, 0x42);
    cpu.pc = 0x8000;
    cpu_step(&cpu);
    printf("%s: CMP 0x50 vs 0x42 -> Z=0, C=1\n",
        (!cpu_get_flag(&cpu, FLAG_Z) && cpu_get_flag(&cpu, FLAG_C)) ? "PASS" : "FAIL");
}

static void test_sum_1_to_10(void) {
    printf("\n=== Program: Sum 1..10 ===\n");
    CPU cpu;
    cpu_init(&cpu);

    // Программа: сумма чисел от 1 до 10
    //   LDX #$00       ; X = 0 (счётчик)
    //   LDA #$00       ; A = 0 (сумма)
    // loop:
    //   INX            ; X++
    //   STX $0201      ; mem[0x0201] = X
    //   ADC $0201      ; A += mem[0x0201]
    //   CPX #$0A       ; X == 10?
    //   BNE loop       ; нет — повтор
    //   STA $0200      ; mem[0x0200] = A (сумма)
    //   JMP $800F      ; бесконечный цикл (пока)

    uint8_t program[] = {
        0xA2, 0x00,       // 0x8000: LDX #$00
        0xA9, 0x00,       // 0x8002: LDA #$00
        // loop (0x8004):
        0xE8,             // 0x8004: INX
        0x8E, 0x01, 0x02, // 0x8005: STX $0201
        0x6D, 0x01, 0x02, // 0x8008: ADC $0201
        0xE0, 0x0A,       // 0x800B: CPX #$0A
        0xD0, 0xF5,       // 0x800D: BNE loop  (смещение -11 = 0xF5)
        0x8D, 0x00, 0x02, // 0x800F: STA $0200
        0x4C, 0x0F, 0x80, // 0x8012: JMP $800F
    };

    for (size_t i = 0; i < sizeof(program); i++) {
        cpu_write(&cpu, 0x8000 + i, program[i]);
    }
    cpu.pc = 0x8000;

    for (int i = 0; i < 100; i++) {
        cpu_step(&cpu);
    }

    uint8_t sum = cpu_read(&cpu, 0x0200);
    printf("%s: sum 1..10 = 55 (got %d)\n",
        (sum == 55) ? "PASS" : "FAIL", sum);
    printf("     A = %d, X = %d, PC = 0x%04X\n", cpu.a, cpu.x, cpu.pc);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    test_arithmetic();
    test_logic();
    test_compare();
    test_sum_1_to_10();

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