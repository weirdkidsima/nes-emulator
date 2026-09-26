#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_SIZE 65536

#define FLAG_C 0x01
#define FLAG_Z 0x02
#define FLAG_I 0x04
#define FLAG_D 0x08
#define FLAG_B 0x10
#define FLAG_U 0x20
#define FLAG_V 0x40
#define FLAG_N 0x80

typedef enum {
    AM_IMP, AM_ACC, AM_IMM, AM_ZP0, AM_ZPX, AM_ZPY,
    AM_REL, AM_ABS, AM_ABX, AM_ABY, AM_IND, AM_IZX, AM_IZY, AM_NON,
} AddressingMode;

typedef struct CPU CPU;
typedef uint8_t (*OpFunc)(CPU*);

struct CPU {
    uint8_t  a;
    uint8_t  x;
    uint8_t  y;
    uint8_t  sp;
    uint16_t pc;
    uint8_t  p;

    uint8_t memory[MEM_SIZE];

    uint64_t cycles;

    uint16_t addr_abs;
    uint8_t  opcode;
};

typedef struct {
    const char* name;
    OpFunc      func;
    AddressingMode mode;
    uint8_t     cycles;
} Opcode;

void cpu_init(CPU* cpu);
void cpu_reset(CPU* cpu);

uint8_t cpu_read(CPU* cpu, uint16_t addr);
void    cpu_write(CPU* cpu, uint16_t addr, uint8_t value);
uint16_t cpu_read16(CPU* cpu, uint16_t addr);

void cpu_set_flag(CPU* cpu, uint8_t flag, bool value);
bool cpu_get_flag(CPU* cpu, uint8_t flag);
void cpu_update_zn(CPU* cpu, uint8_t value);

void cpu_step(CPU* cpu);
void cpu_clock(CPU* cpu, uint32_t n);

uint16_t cpu_addr_imm(CPU* cpu);
uint16_t cpu_addr_zp0(CPU* cpu);
uint16_t cpu_addr_zpx(CPU* cpu);
uint16_t cpu_addr_zpy(CPU* cpu);
uint16_t cpu_addr_abs(CPU* cpu);
uint16_t cpu_addr_abx(CPU* cpu);
uint16_t cpu_addr_aby(CPU* cpu);
uint16_t cpu_addr_ind(CPU* cpu);
uint16_t cpu_addr_izx(CPU* cpu);
uint16_t cpu_addr_izy(CPU* cpu);

#endif // CPU_H