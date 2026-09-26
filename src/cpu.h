#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

// Размер адресного пространства NES - 64КБ
#define MEM_SIZE 65536

// Флаги регистра статуса (P)
#define FLAG_C 0x01  // Carry
#define FLAG_Z 0x02  // Zero
#define FLAG_I 0x04  // Interrupt disable
#define FLAG_D 0x08  // Decimal mode (в NES не используется, но флаг есть)
#define FLAG_B 0x10  // Break
#define FLAG_U 0x20  // Unused (всегда 1)
#define FLAG_V 0x40  // Overflow
#define FLAG_N 0x80  // Negative

// Структура процессора 6502
typedef struct {
    // Регистры
    uint8_t  a;      // Аккумулятор
    uint8_t  x;      // Индексный регистр X
    uint8_t  y;      // Индексный регистр Y
    uint8_t  sp;     // Указатель стека (Stack Pointer)
    uint16_t pc;     // Program Counter (счетчик)
    uint8_t  p;      // Регистр статуса (флаги)

    // Память (адресное пространство NES)
    uint8_t memory[MEM_SIZE];

    // Служебные поля
    uint64_t cycles; // Счётчик тактов
} CPU;

// Инициализация процессора
void cpu_init(CPU* cpu);

// Сброс процессора (Reset)
void cpu_reset(CPU* cpu);

// Чтение байта из памяти
uint8_t cpu_read(CPU* cpu, uint16_t addr);

// Запись байта в память
void cpu_write(CPU* cpu, uint16_t addr, uint8_t value);

// Чтение 16-битного значения (little-endian)
uint16_t cpu_read16(CPU* cpu, uint16_t addr);

// Работа с флагами
void cpu_set_flag(CPU* cpu, uint8_t flag, bool value);
bool cpu_get_flag(CPU* cpu, uint8_t flag);

// Обновление флагов Z и N по значению
void cpu_update_zn(CPU* cpu, uint8_t value);

#endif // CPU_H