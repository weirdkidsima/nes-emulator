#include "cpu.h"
#include <string.h>
#include <stdio.h>

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

uint16_t cpu_addr_imm(CPU* cpu) { return cpu->pc++; }
uint16_t cpu_addr_zp0(CPU* cpu) { return cpu_read(cpu, cpu->pc++); }
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

// === Инструкции: загрузка/сохранение (из 2a) ===

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
static uint8_t op_sta(CPU* cpu) { cpu_write(cpu, cpu->addr_abs, cpu->a); return 0; }
static uint8_t op_stx(CPU* cpu) { cpu_write(cpu, cpu->addr_abs, cpu->x); return 0; }
static uint8_t op_sty(CPU* cpu) { cpu_write(cpu, cpu->addr_abs, cpu->y); return 0; }
static uint8_t op_jmp(CPU* cpu) { cpu->pc = cpu->addr_abs; return 0; }
static uint8_t op_nop(CPU* cpu) { (void)cpu; return 0; }

// === Инструкции: логика ===

static uint8_t op_and(CPU* cpu) {
    cpu->a &= cpu_read(cpu, cpu->addr_abs);
    cpu_update_zn(cpu, cpu->a);
    return 1;
}
static uint8_t op_ora(CPU* cpu) {
    cpu->a |= cpu_read(cpu, cpu->addr_abs);
    cpu_update_zn(cpu, cpu->a);
    return 1;
}
static uint8_t op_eor(CPU* cpu) {
    cpu->a ^= cpu_read(cpu, cpu->addr_abs);
    cpu_update_zn(cpu, cpu->a);
    return 1;
}

// BIT: проверяет биты A & M, ставит Z, а N и V берёт из M
static uint8_t op_bit(CPU* cpu) {
    uint8_t m = cpu_read(cpu, cpu->addr_abs);
    uint8_t result = cpu->a & m;
    cpu_set_flag(cpu, FLAG_Z, result == 0);
    cpu_set_flag(cpu, FLAG_N, (m & 0x80) != 0);
    cpu_set_flag(cpu, FLAG_V, (m & 0x40) != 0);
    return 0;
}

// === Инструкции: сравнение ===

static void compare(CPU* cpu, uint8_t reg, uint8_t m) {
    // C = 1, если reg >= m
    cpu_set_flag(cpu, FLAG_C, reg >= m);
    uint8_t result = (uint8_t)(reg - m);
    cpu_update_zn(cpu, result);
}

static uint8_t op_cmp(CPU* cpu) { compare(cpu, cpu->a, cpu_read(cpu, cpu->addr_abs)); return 1; }
static uint8_t op_cpx(CPU* cpu) { compare(cpu, cpu->x, cpu_read(cpu, cpu->addr_abs)); return 0; }
static uint8_t op_cpy(CPU* cpu) { compare(cpu, cpu->y, cpu_read(cpu, cpu->addr_abs)); return 0; }

// === Инструкции: инкремент/декремент ===

static uint8_t op_inx(CPU* cpu) { cpu->x++; cpu_update_zn(cpu, cpu->x); return 0; }
static uint8_t op_iny(CPU* cpu) { cpu->y++; cpu_update_zn(cpu, cpu->y); return 0; }
static uint8_t op_dex(CPU* cpu) { cpu->x--; cpu_update_zn(cpu, cpu->x); return 0; }
static uint8_t op_dey(CPU* cpu) { cpu->y--; cpu_update_zn(cpu, cpu->y); return 0; }

static uint8_t op_inc(CPU* cpu) {
    uint8_t val = (uint8_t)(cpu_read(cpu, cpu->addr_abs) + 1);
    cpu_write(cpu, cpu->addr_abs, val);
    cpu_update_zn(cpu, val);
    return 0;
}
static uint8_t op_dec(CPU* cpu) {
    uint8_t val = (uint8_t)(cpu_read(cpu, cpu->addr_abs) - 1);
    cpu_write(cpu, cpu->addr_abs, val);
    cpu_update_zn(cpu, val);
    return 0;
}

// === Инструкции: пересылки ===

static uint8_t op_tax(CPU* cpu) { cpu->x = cpu->a; cpu_update_zn(cpu, cpu->x); return 0; }
static uint8_t op_tay(CPU* cpu) { cpu->y = cpu->a; cpu_update_zn(cpu, cpu->y); return 0; }
static uint8_t op_txa(CPU* cpu) { cpu->a = cpu->x; cpu_update_zn(cpu, cpu->a); return 0; }
static uint8_t op_tya(CPU* cpu) { cpu->a = cpu->y; cpu_update_zn(cpu, cpu->a); return 0; }
static uint8_t op_tsx(CPU* cpu) { cpu->x = cpu->sp; cpu_update_zn(cpu, cpu->x); return 0; }
static uint8_t op_txs(CPU* cpu) { cpu->sp = cpu->x; return 0; }

// === Инструкции: арифметика (самое сложное!) ===

static uint8_t op_adc(CPU* cpu) {
    uint8_t m = cpu_read(cpu, cpu->addr_abs);
    uint8_t a = cpu->a;
    uint8_t c = cpu_get_flag(cpu, FLAG_C) ? 1 : 0;

    uint16_t sum = (uint16_t)a + (uint16_t)m + c;
    uint8_t result = (uint8_t)(sum & 0xFF);

    // Carry: если сумма больше 255
    cpu_set_flag(cpu, FLAG_C, sum > 0xFF);

    // Overflow: если знак A и знак M одинаковые, а знак результата — другой
    // (a ^ result) & (m ^ result) & 0x80
    cpu_set_flag(cpu, FLAG_V, ((a ^ result) & (m ^ result) & 0x80) != 0);

    cpu->a = result;
    cpu_update_zn(cpu, result);
    return 1;
}

static uint8_t op_sbc(CPU* cpu) {
    uint8_t m = cpu_read(cpu, cpu->addr_abs);
    uint8_t a = cpu->a;
    uint8_t c = cpu_get_flag(cpu, FLAG_C) ? 1 : 0;

    // SBC = A - M - (1 - C) = A + ~M + C
    uint8_t m_inv = (uint8_t)~m;
    uint16_t sum = (uint16_t)a + (uint16_t)m_inv + c;
    uint8_t result = (uint8_t)(sum & 0xFF);

    cpu_set_flag(cpu, FLAG_C, sum > 0xFF);
    cpu_set_flag(cpu, FLAG_V, ((a ^ result) & (m_inv ^ result) & 0x80) != 0);

    cpu->a = result;
    cpu_update_zn(cpu, result);
    return 1;
}

// === Инструкции: ветвления (REL) ===

static uint8_t op_beq(CPU* cpu) {
    if (cpu_get_flag(cpu, FLAG_Z)) {
        cpu->pc = cpu->addr_abs;
        return 1; // +1 такт если переход
    }
    return 0;
}
static uint8_t op_bne(CPU* cpu) {
    if (!cpu_get_flag(cpu, FLAG_Z)) {
        cpu->pc = cpu->addr_abs;
        return 1;
    }
    return 0;
}
static uint8_t op_bcs(CPU* cpu) {
    if (cpu_get_flag(cpu, FLAG_C)) {
        cpu->pc = cpu->addr_abs;
        return 1;
    }
    return 0;
}
static uint8_t op_bcc(CPU* cpu) {
    if (!cpu_get_flag(cpu, FLAG_C)) {
        cpu->pc = cpu->addr_abs;
        return 1;
    }
    return 0;
}
static uint8_t op_bmi(CPU* cpu) {
    if (cpu_get_flag(cpu, FLAG_N)) {
        cpu->pc = cpu->addr_abs;
        return 1;
    }
    return 0;
}
static uint8_t op_bpl(CPU* cpu) {
    if (!cpu_get_flag(cpu, FLAG_N)) {
        cpu->pc = cpu->addr_abs;
        return 1;
    }
    return 0;
}
static uint8_t op_bvs(CPU* cpu) {
    if (cpu_get_flag(cpu, FLAG_V)) {
        cpu->pc = cpu->addr_abs;
        return 1;
    }
    return 0;
}
static uint8_t op_bvc(CPU* cpu) {
    if (!cpu_get_flag(cpu, FLAG_V)) {
        cpu->pc = cpu->addr_abs;
        return 1;
    }
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

    // AND
    opcode_table[0x29] = (Opcode){ "AND", op_and, AM_IMM, 2 };
    opcode_table[0x25] = (Opcode){ "AND", op_and, AM_ZP0, 3 };
    opcode_table[0x35] = (Opcode){ "AND", op_and, AM_ZPX, 4 };
    opcode_table[0x2D] = (Opcode){ "AND", op_and, AM_ABS, 4 };
    opcode_table[0x3D] = (Opcode){ "AND", op_and, AM_ABX, 4 };
    opcode_table[0x39] = (Opcode){ "AND", op_and, AM_ABY, 4 };
    opcode_table[0x21] = (Opcode){ "AND", op_and, AM_IZX, 6 };
    opcode_table[0x31] = (Opcode){ "AND", op_and, AM_IZY, 5 };

    // ORA
    opcode_table[0x09] = (Opcode){ "ORA", op_ora, AM_IMM, 2 };
    opcode_table[0x05] = (Opcode){ "ORA", op_ora, AM_ZP0, 3 };
    opcode_table[0x15] = (Opcode){ "ORA", op_ora, AM_ZPX, 4 };
    opcode_table[0x0D] = (Opcode){ "ORA", op_ora, AM_ABS, 4 };
    opcode_table[0x1D] = (Opcode){ "ORA", op_ora, AM_ABX, 4 };
    opcode_table[0x19] = (Opcode){ "ORA", op_ora, AM_ABY, 4 };
    opcode_table[0x01] = (Opcode){ "ORA", op_ora, AM_IZX, 6 };
    opcode_table[0x11] = (Opcode){ "ORA", op_ora, AM_IZY, 5 };

    // EOR
    opcode_table[0x49] = (Opcode){ "EOR", op_eor, AM_IMM, 2 };
    opcode_table[0x45] = (Opcode){ "EOR", op_eor, AM_ZP0, 3 };
    opcode_table[0x55] = (Opcode){ "EOR", op_eor, AM_ZPX, 4 };
    opcode_table[0x4D] = (Opcode){ "EOR", op_eor, AM_ABS, 4 };
    opcode_table[0x5D] = (Opcode){ "EOR", op_eor, AM_ABX, 4 };
    opcode_table[0x59] = (Opcode){ "EOR", op_eor, AM_ABY, 4 };
    opcode_table[0x41] = (Opcode){ "EOR", op_eor, AM_IZX, 6 };
    opcode_table[0x51] = (Opcode){ "EOR", op_eor, AM_IZY, 5 };

    // BIT
    opcode_table[0x24] = (Opcode){ "BIT", op_bit, AM_ZP0, 3 };
    opcode_table[0x2C] = (Opcode){ "BIT", op_bit, AM_ABS, 4 };

    // CMP
    opcode_table[0xC9] = (Opcode){ "CMP", op_cmp, AM_IMM, 2 };
    opcode_table[0xC5] = (Opcode){ "CMP", op_cmp, AM_ZP0, 3 };
    opcode_table[0xD5] = (Opcode){ "CMP", op_cmp, AM_ZPX, 4 };
    opcode_table[0xCD] = (Opcode){ "CMP", op_cmp, AM_ABS, 4 };
    opcode_table[0xDD] = (Opcode){ "CMP", op_cmp, AM_ABX, 4 };
    opcode_table[0xD9] = (Opcode){ "CMP", op_cmp, AM_ABY, 4 };
    opcode_table[0xC1] = (Opcode){ "CMP", op_cmp, AM_IZX, 6 };
    opcode_table[0xD1] = (Opcode){ "CMP", op_cmp, AM_IZY, 5 };

    // CPX
    opcode_table[0xE0] = (Opcode){ "CPX", op_cpx, AM_IMM, 2 };
    opcode_table[0xE4] = (Opcode){ "CPX", op_cpx, AM_ZP0, 3 };
    opcode_table[0xEC] = (Opcode){ "CPX", op_cpx, AM_ABS, 4 };

    // CPY
    opcode_table[0xC0] = (Opcode){ "CPY", op_cpy, AM_IMM, 2 };
    opcode_table[0xC4] = (Opcode){ "CPY", op_cpy, AM_ZP0, 3 };
    opcode_table[0xCC] = (Opcode){ "CPY", op_cpy, AM_ABS, 4 };

    // INX/INY/DEX/DEY
    opcode_table[0xE8] = (Opcode){ "INX", op_inx, AM_IMP, 2 };
    opcode_table[0xC8] = (Opcode){ "INY", op_iny, AM_IMP, 2 };
    opcode_table[0xCA] = (Opcode){ "DEX", op_dex, AM_IMP, 2 };
    opcode_table[0x88] = (Opcode){ "DEY", op_dey, AM_IMP, 2 };

    // INC/DEC
    opcode_table[0xE6] = (Opcode){ "INC", op_inc, AM_ZP0, 5 };
    opcode_table[0xF6] = (Opcode){ "INC", op_inc, AM_ZPX, 6 };
    opcode_table[0xEE] = (Opcode){ "INC", op_inc, AM_ABS, 6 };
    opcode_table[0xFE] = (Opcode){ "INC", op_inc, AM_ABX, 7 };
    opcode_table[0xC6] = (Opcode){ "DEC", op_dec, AM_ZP0, 5 };
    opcode_table[0xD6] = (Opcode){ "DEC", op_dec, AM_ZPX, 6 };
    opcode_table[0xCE] = (Opcode){ "DEC", op_dec, AM_ABS, 6 };
    opcode_table[0xDE] = (Opcode){ "DEC", op_dec, AM_ABX, 7 };

    // Пересылки
    opcode_table[0xAA] = (Opcode){ "TAX", op_tax, AM_IMP, 2 };
    opcode_table[0xA8] = (Opcode){ "TAY", op_tay, AM_IMP, 2 };
    opcode_table[0x8A] = (Opcode){ "TXA", op_txa, AM_IMP, 2 };
    opcode_table[0x98] = (Opcode){ "TYA", op_tya, AM_IMP, 2 };
    opcode_table[0xBA] = (Opcode){ "TSX", op_tsx, AM_IMP, 2 };
    opcode_table[0x9A] = (Opcode){ "TXS", op_txs, AM_IMP, 2 };

    // ADC
    opcode_table[0x69] = (Opcode){ "ADC", op_adc, AM_IMM, 2 };
    opcode_table[0x65] = (Opcode){ "ADC", op_adc, AM_ZP0, 3 };
    opcode_table[0x75] = (Opcode){ "ADC", op_adc, AM_ZPX, 4 };
    opcode_table[0x6D] = (Opcode){ "ADC", op_adc, AM_ABS, 4 };
    opcode_table[0x7D] = (Opcode){ "ADC", op_adc, AM_ABX, 4 };
    opcode_table[0x79] = (Opcode){ "ADC", op_adc, AM_ABY, 4 };
    opcode_table[0x61] = (Opcode){ "ADC", op_adc, AM_IZX, 6 };
    opcode_table[0x71] = (Opcode){ "ADC", op_adc, AM_IZY, 5 };

    // SBC
    opcode_table[0xE9] = (Opcode){ "SBC", op_sbc, AM_IMM, 2 };
    opcode_table[0xE5] = (Opcode){ "SBC", op_sbc, AM_ZP0, 3 };
    opcode_table[0xF5] = (Opcode){ "SBC", op_sbc, AM_ZPX, 4 };
    opcode_table[0xED] = (Opcode){ "SBC", op_sbc, AM_ABS, 4 };
    opcode_table[0xFD] = (Opcode){ "SBC", op_sbc, AM_ABX, 4 };
    opcode_table[0xF9] = (Opcode){ "SBC", op_sbc, AM_ABY, 4 };
    opcode_table[0xE1] = (Opcode){ "SBC", op_sbc, AM_IZX, 6 };
    opcode_table[0xF1] = (Opcode){ "SBC", op_sbc, AM_IZY, 5 };

    // Ветвления
    opcode_table[0xF0] = (Opcode){ "BEQ", op_beq, AM_REL, 2 };
    opcode_table[0xD0] = (Opcode){ "BNE", op_bne, AM_REL, 2 };
    opcode_table[0xB0] = (Opcode){ "BCS", op_bcs, AM_REL, 2 };
    opcode_table[0x90] = (Opcode){ "BCC", op_bcc, AM_REL, 2 };
    opcode_table[0x30] = (Opcode){ "BMI", op_bmi, AM_REL, 2 };
    opcode_table[0x10] = (Opcode){ "BPL", op_bpl, AM_REL, 2 };
    opcode_table[0x70] = (Opcode){ "BVS", op_bvs, AM_REL, 2 };
    opcode_table[0x50] = (Opcode){ "BVC", op_bvc, AM_REL, 2 };
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

    uint8_t extra = op.func(cpu);
    cpu->cycles += op.cycles + extra;
}

void cpu_clock(CPU* cpu, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        cpu_step(cpu);
    }
}

void cpu_init_opcode_table(void) {
    build_opcode_table();
}