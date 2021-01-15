#ifndef MEM_H_
#define MEM_H_

#define _HALF_KB 512
#define _1_KB 1024
#define _2_KB 2048
#define _4_KB 4096
#define _8_KB 8192
#define _16_KB 16384

#define RESET_PC 0xFFFC
#define RESET_STKP 0xFD
#define BASE_STKP 0x0100

#define IRQ_PC 0xFFFE
#define NMI_PC 0xFFFA

/* Addresses ranges */
#define SYSTEM_RAM_ADDR_LOWER 0x0000
#define SYSTEM_RAM_ADDR_UPPER 0x1FFF

#define PATTERN_ADDR_LOWER 0x0000
#define PATTERN_ADDR_UPPER 0x1FFF

#define NAME_TABLE_ADDR_LOWER 0x2000
#define NAME_TABLE_ADDR_UPPER 0x3EFF

#define SYSTEM_PPU_ADDR_LOWER 0x2000
#define SYSTEM_PPU_ADDR_UPPER 0x3FFF

#define PALETTE_ADDR_LOWER 0x3F00
#define PALETTE_ADDR_UPPER 0x3FFF

#define CONTROLLER_ADDR_LOWER 0x4016
#define CONTROLLER_ADDR_UPPER 0x4017

/* OAM */
#define OAM_ADDR 0x4014

#endif
