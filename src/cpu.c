#include "cpu.h"
#include <string.h>
#include <stdio.h>

// Forward declaration
void cpu_init_opcode_table(void);

// === Инициализация ===

void cpu_init(CPU* cpu) {
    memset(cpu, 0, sizeof(CPU));
    cpu_init_opcode_table();
    cpu_reset(cpu);
}

void cpu_reset(CPU* cpu) {
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    cpu->sp = 0xFD;
    cpu->p = FLAG_U | FLAG_I;
    cpu->pc = 0x0000;
    cpu->cycles = 0;
    memset(cpu->memory, 0, MEM_SIZE);
}

// === Шина памяти ===

uint8_t cpu_read(CPU* cpu, uint16_t addr) {
    return cpu->memory[addr];
}

void cpu_write(CPU* cpu, uint16_t addr, uint8_t value) {
    cpu->memory[addr] = value;
}

uint16_t cpu_read16(CPU* cpu, uint16_t addr) {
    uint8_t lo = cpu_read(cpu, addr);
    uint8_t hi = cpu_read(cpu, addr + 1);
    return (uint16_t)((hi << 8) | lo);
}

// === Флаги ===

void cpu_set_flag(CPU* cpu, uint8_t flag, bool value) {
    if (value) cpu->p |= flag;
    else       cpu->p &= ~flag;
}

bool cpu_get_flag(CPU* cpu, uint8_t flag) {
    return (cpu->p & flag) != 0;
}

void cpu_update_zn(CPU* cpu, uint8_t value) {
    cpu_set_flag(cpu, FLAG_Z, value == 0);
    cpu_set_flag(cpu, FLAG_N, (value & 0x80) != 0);
}

// === Режимы адресации ===

uint16_t cpu_addr_imm(CPU* cpu) {
    return cpu->pc++;
}

uint16_t cpu_addr_zp0(CPU* cpu) {
    return cpu_read(cpu, cpu->pc++);
}

uint16_t cpu_addr_zpx(CPU* cpu) {
    uint8_t base = cpu_read(cpu, cpu->pc++);
    return (uint8_t)(base + cpu->x);
}

uint16_t cpu_addr_zpy(CPU* cpu) {
    uint8_t base = cpu_read(cpu, cpu->pc++);
    return (uint8_t)(base + cpu->y);
}

uint16_t cpu_addr_abs(CPU* cpu) {
    uint16_t addr = cpu_read16(cpu, cpu->pc);
    cpu->pc += 2;
    return addr;
}

uint16_t cpu_addr_abx(CPU* cpu) {
    uint16_t base = cpu_read16(cpu, cpu->pc);
    cpu->pc += 2;
    return base + cpu->x;
}

uint16_t cpu_addr_aby(CPU* cpu) {
    uint16_t base = cpu_read16(cpu, cpu->pc);
    cpu->pc += 2;
    return base + cpu->y;
}

uint16_t cpu_addr_ind(CPU* cpu) {
    uint16_t ptr = cpu_read16(cpu, cpu->pc);
    cpu->pc += 2;
    uint8_t lo = cpu_read(cpu, ptr);
    uint8_t hi = cpu_read(cpu, (ptr & 0xFF00) | (uint8_t)((ptr + 1) & 0x00FF));
    return (uint16_t)((hi << 8) | lo);
}

uint16_t cpu_addr_izx(CPU* cpu) {
    uint8_t base = cpu_read(cpu, cpu->pc++);
    uint8_t ptr = (uint8_t)(base + cpu->x);
    uint8_t lo = cpu_read(cpu, ptr);
    uint8_t hi = cpu_read(cpu, (uint8_t)(ptr + 1));
    return (uint16_t)((hi << 8) | lo);
}

uint16_t cpu_addr_izy(CPU* cpu) {
    uint8_t ptr = cpu_read(cpu, cpu->pc++);
    uint8_t lo = cpu_read(cpu, ptr);
    uint8_t hi = cpu_read(cpu, (uint8_t)(ptr + 1));
    uint16_t base = (uint16_t)((hi << 8) | lo);
    return base + cpu->y;
}

// === Инструкции ===

static uint8_t op_lda(CPU* cpu) {
    uint8_t val = cpu_read(cpu, cpu->addr_abs);
    cpu->a = val;
    cpu_update_zn(cpu, val);
    return 1;
}

static uint8_t op_ldx(CPU* cpu) {
    uint8_t val = cpu_read(cpu, cpu->addr_abs);
    cpu->x = val;
    cpu_update_zn(cpu, val);
    return 1;
}

static uint8_t op_ldy(CPU* cpu) {
    uint8_t val = cpu_read(cpu, cpu->addr_abs);
    cpu->y = val;
    cpu_update_zn(cpu, val);
    return 1;
}

static uint8_t op_sta(CPU* cpu) {
    cpu_write(cpu, cpu->addr_abs, cpu->a);
    return 0;
}

static uint8_t op_stx(CPU* cpu) {
    cpu_write(cpu, cpu->addr_abs, cpu->x);
    return 0;
}

static uint8_t op_sty(CPU* cpu) {
    cpu_write(cpu, cpu->addr_abs, cpu->y);
    return 0;
}

static uint8_t op_jmp(CPU* cpu) {
    cpu->pc = cpu->addr_abs;
    return 0;
}

static uint8_t op_nop(CPU* cpu) {
    (void)cpu;
    return 0;
}

// === Таблица опкодов ===

static Opcode opcode_table[256];

static void build_opcode_table(void) {
    for (int i = 0; i < 256; i++) {
        opcode_table[i] = (Opcode){ "???", op_nop, AM_NON, 2 };
    }

    // LDA
    opcode_table[0xA9] = (Opcode){ "LDA", op_lda, AM_IMM, 2 };
    opcode_table[0xA5] = (Opcode){ "LDA", op_lda, AM_ZP0, 3 };
    opcode_table[0xB5] = (Opcode){ "LDA", op_lda, AM_ZPX, 4 };
    opcode_table[0xAD] = (Opcode){ "LDA", op_lda, AM_ABS, 4 };
    opcode_table[0xBD] = (Opcode){ "LDA", op_lda, AM_ABX, 4 };
    opcode_table[0xB9] = (Opcode){ "LDA", op_lda, AM_ABY, 4 };
    opcode_table[0xA1] = (Opcode){ "LDA", op_lda, AM_IZX, 6 };
    opcode_table[0xB1] = (Opcode){ "LDA", op_lda, AM_IZY, 5 };

    // LDX
    opcode_table[0xA2] = (Opcode){ "LDX", op_ldx, AM_IMM, 2 };
    opcode_table[0xA6] = (Opcode){ "LDX", op_ldx, AM_ZP0, 3 };
    opcode_table[0xB6] = (Opcode){ "LDX", op_ldx, AM_ZPY, 4 };
    opcode_table[0xAE] = (Opcode){ "LDX", op_ldx, AM_ABS, 4 };
    opcode_table[0xBE] = (Opcode){ "LDX", op_ldx, AM_ABY, 4 };

    // LDY
    opcode_table[0xA0] = (Opcode){ "LDY", op_ldy, AM_IMM, 2 };
    opcode_table[0xA4] = (Opcode){ "LDY", op_ldy, AM_ZP0, 3 };
    opcode_table[0xB4] = (Opcode){ "LDY", op_ldy, AM_ZPX, 4 };
    opcode_table[0xAC] = (Opcode){ "LDY", op_ldy, AM_ABS, 4 };
    opcode_table[0xBC] = (Opcode){ "LDY", op_ldy, AM_ABX, 4 };

    // STA
    opcode_table[0x85] = (Opcode){ "STA", op_sta, AM_ZP0, 3 };
    opcode_table[0x95] = (Opcode){ "STA", op_sta, AM_ZPX, 4 };
    opcode_table[0x8D] = (Opcode){ "STA", op_sta, AM_ABS, 4 };
    opcode_table[0x9D] = (Opcode){ "STA", op_sta, AM_ABX, 5 };
    opcode_table[0x99] = (Opcode){ "STA", op_sta, AM_ABY, 5 };
    opcode_table[0x81] = (Opcode){ "STA", op_sta, AM_IZX, 6 };
    opcode_table[0x91] = (Opcode){ "STA", op_sta, AM_IZY, 6 };

    // STX
    opcode_table[0x86] = (Opcode){ "STX", op_stx, AM_ZP0, 3 };
    opcode_table[0x96] = (Opcode){ "STX", op_stx, AM_ZPY, 4 };
    opcode_table[0x8E] = (Opcode){ "STX", op_stx, AM_ABS, 4 };

    // STY
    opcode_table[0x84] = (Opcode){ "STY", op_sty, AM_ZP0, 3 };
    opcode_table[0x94] = (Opcode){ "STY", op_sty, AM_ZPX, 4 };
    opcode_table[0x8C] = (Opcode){ "STY", op_sty, AM_ABS, 4 };

    // JMP
    opcode_table[0x4C] = (Opcode){ "JMP", op_jmp, AM_ABS, 3 };
    opcode_table[0x6C] = (Opcode){ "JMP", op_jmp, AM_IND, 5 };

    // NOP
    opcode_table[0xEA] = (Opcode){ "NOP", op_nop, AM_IMP, 2 };
}

// === Fetch-decode-execute ===

void cpu_step(CPU* cpu) {
    cpu->opcode = cpu_read(cpu, cpu->pc++);
    Opcode op = opcode_table[cpu->opcode];

    switch (op.mode) {
        case AM_IMP: cpu->addr_abs = 0; break;
        case AM_ACC: cpu->addr_abs = 0; break;
        case AM_IMM: cpu->addr_abs = cpu_addr_imm(cpu); break;
        case AM_ZP0: cpu->addr_abs = cpu_addr_zp0(cpu); break;
        case AM_ZPX: cpu->addr_abs = cpu_addr_zpx(cpu); break;
        case AM_ZPY: cpu->addr_abs = cpu_addr_zpy(cpu); break;
        case AM_ABS: cpu->addr_abs = cpu_addr_abs(cpu); break;
        case AM_ABX: cpu->addr_abs = cpu_addr_abx(cpu); break;
        case AM_ABY: cpu->addr_abs = cpu_addr_aby(cpu); break;
        case AM_IND: cpu->addr_abs = cpu_addr_ind(cpu); break;
        case AM_IZX: cpu->addr_abs = cpu_addr_izx(cpu); break;
        case AM_IZY: cpu->addr_abs = cpu_addr_izy(cpu); break;
        case AM_REL: {
            int8_t offset = (int8_t)cpu_read(cpu, cpu->pc++);
            cpu->addr_abs = cpu->pc + offset;
            break;
        }
        default: break;
    }

    op.func(cpu);
    cpu->cycles += op.cycles;
}

void cpu_clock(CPU* cpu, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        cpu_step(cpu);
    }
}

// Вызывается из cpu_init
void cpu_init_opcode_table(void) {
    build_opcode_table();
}