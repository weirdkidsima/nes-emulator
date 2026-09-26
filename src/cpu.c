#include "cpu.h"
#include <string.h>
#include <stdio.h>

void cpu_init(CPU* cpu) {
    memset(cpu, 0, sizeof(CPU));
    cpu_reset(cpu);
}

void cpu_reset(CPU* cpu) {
    // Значение после сброса NES
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xFD;
    cpu->p = FLAG_U | FLAG_I; // Unused = 1 Interrput disable = 1
    cpu->pc = 0x0000; // Реальный адрес позже загрузит картридж
    cpu->cycles = 0;

    // Память после сброса - нули
    memset(cpu->memory, 0, MEM_SIZE);
}

uint8_t cpu_read(CPU* cpu, uint16_t addr) {
    // Временно просто читаем из массива, далее маршрутизация RAM, PPU, APU, Картридж
    return cpu->memory[addr];
}

void cpu_write(CPU* cpu, uint16_t addr, uint8_t value) {
    // Тоже пока массив
    cpu->memory[addr] = value;
}

uint16_t cpu_read16(CPU* cpu, uint16_t addr) {
    // Little_endian младший бит первый
    uint8_t lo = cpu_read(cpu, addr);
    uint8_t hi = cpu_read(cpu, addr + 1);
    return (uint16_t)((hi << 8) | lo);
}

void cpu_set_flag(CPU* cpu, uint8_t flag, bool value) {
    if(value) {
        cpu->p |= flag;
    } else {
        cpu ->p &= -flag;
    }
}

bool cpu_get_flag(CPU* cpu, uint8_t flag) {
    return (cpu->p & flag) != 0;
}

void cpu_update_zn(CPU* cpu, uint8_t value) {
    // Zero flag установлен если value 0
    cpu_set_flag(cpu, FLAG_Z, value == 0);
    // Negative flag установлен если старший бит 1
    cpu_set_flag(cpu, FLAG_N, (value & 0x80) != 0);
}