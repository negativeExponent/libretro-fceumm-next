/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022 Cluster
 *  Copyright (C) 2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Very complicated homebrew multicart m342.mapper with.
 * The code is so obscured and weird because it's ported from Verilog CPLD source code:
 * https://github.com/ClusterM/coolgirl-famicom-multicart/blob/master/CoolGirl_mappers.vh
 *
 * Range: $5000-$5FFF
 *
 * Mask: $5007
 *
 * All registers are $00 on power-on and reset.
 *
 * $5xx0
 * 7  bit  0
 * ---- ----
 * PPPP PPPP
 * |||| ||||
 * ++++-++++-- PRG base offset (A29-A22)
 *
 * $5xx1
 * 7  bit  0
 * ---- ----
 * PPPP PPPP
 * |||| ||||
 * ++++-++++-- PRG base offset (A21-A14)
 *
 * $5xx2
 * 7  bit  0
 * ---- ----
 * AMMM MMMM
 * |||| ||||
 * |+++-++++-- PRG mask (A20-A14, inverted+anded with PRG address)
 * +---------- CHR mask (A18, inverted+anded with CHR address)
 *
 * $5xx3
 * 7  bit  0
 * ---- ----
 * BBBC CCCC
 * |||| ||||
 * |||+-++++-- CHR bank A (bits 7-3)
 * +++-------- PRG banking mode (see below)
 *
 * $5xx4
 * 7  bit  0
 * ---- ----
 * DDDE EEEE
 * |||| ||||
 * |||+-++++-- CHR mask (A17-A13, inverted+anded with CHR address)
 * +++-------- CHR banking mode (see below)
 *
 * $5xx5
 * 7  bit  0
 * ---- ----
 * CDDE EEWW
 * |||| ||||
 * |||| ||++-- 8KiB WRAM page at $6000-$7FFF
 * |+++-++---- PRG bank A (bits 5-1)
 * +---------- CHR bank A (bit 8)
 *
 * $5xx6
 * 7  bit  0
 * ---- ----
 * FFFM MMMM
 * |||| ||||
 * |||+ ++++-- Mapper code (bits 4-0, see below)
 * +++-------- Flags 2-0, functionality depends on selected m342.mapper
 *
 * $5xx7
 * 7  bit  0
 * ---- ----
 * LMTR RSNO
 * |||| |||+-- Enable WRAM (read and write) at $6000-$7FFF
 * |||| ||+--- Allow writes to CHR RAM
 * |||| |+---- Allow writes to flash chip
 * |||+-+----- Mirroring (00=vertical, 01=horizontal, 10=1Sa, 11=1Sb)
 * ||+-------- Enable four-screen mode
 * |+-- ------ Mapper code (bit 5, see below)
 * +---------- Lockout bit (prevent further writes to all registers)
 *
 */

#include "mapinc.h"

#define SAVE_FLASH_SIZE	  (1024 * 1024 * 8)
#define FLASH_SECTOR_SIZE (128 * 1024)
#define ROM_CHIP		  0x00
#define WRAM_CHIP		  0x10
#define FLASH_CHIP		  0x11
#define CFI_CHIP		  0x13

static uint32_t CHR_SIZE = 0;
static uint8_t *SAVE_FLASH = NULL;
static uint8_t *CFI = NULL;

static struct {
	uint8_t mapper;
	uint8_t flags;
	uint8_t lockout;
	uint8_t fourscreen;
	uint8_t mirroring;
	uint8_t map_rom_on_6000;

	uint8_t sram_enabled;
	uint8_t sram_page;

	uint32_t prg_base;
	uint32_t prg_mask;
	uint8_t prg_mode;
	uint8_t prg_bank_6000;
	uint8_t prg_bank_a;
	uint8_t prg_bank_b;
	uint8_t prg_bank_c;
	uint8_t prg_bank_d;

	uint32_t prg_bank_6000_mapped;
	uint32_t prg_bank_a_mapped;
	uint32_t prg_bank_b_mapped;
	uint32_t prg_bank_c_mapped;
	uint32_t prg_bank_d_mapped;

	uint16_t chr_bank_a;
	uint16_t chr_bank_b;
	uint16_t chr_bank_c;
	uint16_t chr_bank_d;
	uint16_t chr_bank_e;
	uint16_t chr_bank_f;
	uint16_t chr_bank_g;
	uint16_t chr_bank_h;
	uint8_t can_write_chr;
	uint32_t chr_mask;
	uint8_t chr_mode;

	uint8_t TKSMIR[8];

	uint8_t can_write_flash;
	uint8_t flash_state;
	uint16_t flash_buffer_a[10];
	uint8_t flash_buffer_v[10];
	uint8_t cfi_mode;

	/* for MMC1 */
	struct {
		uint64_t lreset;
		uint8_t load_register;
	} mmc1;
	/* for MMC2/MMC4 */
	struct {
		uint8_t latch0;
		uint8_t latch1;
	} mmc2and4;
	/* for MMC3 */
	struct {
		uint8_t internal;
		/* for MMC3 scanline-based interrupts, counts A12 rises after long A12 falls */
		uint8_t irq_enabled;
		uint8_t irq_latch;
		uint8_t irq_counter;
		uint8_t irq_reload;
	} mmc3;
	/* for MMC5 scanline-based interrupts, counts dummy PPU reads */
	struct {
		uint8_t irq_enabled;
		uint8_t irq_line;
		uint8_t irq_out;
	} mmc5;
	/* for VRC3 CPU-based interrupts */
	struct {
		uint16_t irq_value;
		uint8_t irq_control;
		uint16_t irq_latch;
	} vrc3;
	/* for VRC4 CPU-based interrupts */
	struct {
		uint8_t irq_value;
		uint8_t irq_control;
		uint8_t irq_latch;
		uint8_t irq_prescaler;
		uint8_t irq_prescaler_counter;
	} vrc4;
	/* for mapper #69 */
	struct {
		uint8_t internal;
		/* for Sunsoft FME-7 */
		uint8_t irq_enabled;
		uint8_t counter_enabled;
		uint16_t irq_value;
	} mapper69;
	/* for mapper #112 */
	struct {
		uint8_t internal;
	} mapper112;
	/* for mapper #163 */
	struct {
		uint8_t latch;
		uint8_t r0;
		uint8_t r1;
		uint8_t r2;
		uint8_t r3;
		uint8_t r4;
		uint8_t r5;
	} mapper163;
	/* For mapper #90 */
	struct {
		uint8_t xor ;
		uint8_t mul1;
		uint8_t mul2;
	} mapper90;
	/* for mapper #18 */
	struct {
		uint16_t irq_value;
		uint8_t irq_control;
		uint16_t irq_latch;
	} mapper18;
	/* for mapper #65 */
	struct {
		uint8_t irq_enabled;
		uint16_t irq_value;
		uint16_t irq_latch;
	} mapper65;
	/* for mapper #42 (only Baby Mario) */
	struct {
		uint8_t irq_enabled;
		uint16_t irq_value;
	} mapper42;
	/* for mapper #83 */
	struct {
		uint8_t irq_enabled_latch;
		uint8_t irq_enabled;
		uint16_t irq_counter;
	} mapper83;
	/* for mapper #67 */
	struct {
		uint8_t irq_enabled;
		uint8_t irq_latch;
		uint16_t irq_counter;
	} mapper67;
} m342;

static uint8_t show_error_log = 0;
static uint8_t vrc24_compatibility = 0;

/* Micron 4-gbit memory CFI data */
static const uint8_t cfi_data[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x51, 0x52, 0x59, 0x02, 0x00, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x27, 0x36, 0x00, 0x00, 0x06,
	0x06, 0x09, 0x13, 0x03, 0x05, 0x03, 0x02, 0x1E,
	0x02, 0x00, 0x06, 0x00, 0x01, 0xFF, 0x03, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
	0x50, 0x52, 0x49, 0x31, 0x33, 0x14, 0x02, 0x01,
	0x00, 0x08, 0x00, 0x00, 0x02, 0xB5, 0xC5, 0x05,
	0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static INLINE uint32_t PRGBank8_mapped(uint8_t bank) {
	return ((m342.prg_base << 1) | (bank & ((~(m342.prg_mask << 1) & 0xFE) | 1)));
}

static INLINE uint8_t isFlash(uint32_t mapped) {
	return (SAVE_FLASH != NULL && m342.prg_bank_a_mapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8);
}

static void SyncPRG(void) {
	uint8_t REG_A_CHIP, REG_B_CHIP, REG_C_CHIP, REG_D_CHIP;

	m342.prg_bank_6000_mapped = PRGBank8_mapped(m342.prg_bank_6000);
	m342.prg_bank_a_mapped = PRGBank8_mapped(m342.prg_bank_a);
	m342.prg_bank_b_mapped = PRGBank8_mapped(m342.prg_bank_b);
	m342.prg_bank_c_mapped = PRGBank8_mapped(m342.prg_bank_c);
	m342.prg_bank_d_mapped = PRGBank8_mapped(m342.prg_bank_d);
	REG_A_CHIP = isFlash(m342.prg_bank_a_mapped) ? FLASH_CHIP : ROM_CHIP;
	REG_B_CHIP = isFlash(m342.prg_bank_b_mapped) ? FLASH_CHIP : ROM_CHIP;
	REG_C_CHIP = isFlash(m342.prg_bank_c_mapped) ? FLASH_CHIP : ROM_CHIP;
	REG_D_CHIP = isFlash(m342.prg_bank_d_mapped) ? FLASH_CHIP : ROM_CHIP;

	if (!m342.cfi_mode || !SAVE_FLASH) {
		switch (m342.prg_mode & 0x07) {
		default:
		case 0:
			setprg16r(REG_A_CHIP, 0x8000, m342.prg_bank_a_mapped >> 1);
			setprg16r(REG_C_CHIP, 0xC000, m342.prg_bank_c_mapped >> 1);
			break;
		case 1:
			setprg16r(REG_C_CHIP, 0x8000, m342.prg_bank_c_mapped >> 1);
			setprg16r(REG_A_CHIP, 0xC000, m342.prg_bank_a_mapped >> 1);
			break;
		case 4:
			setprg8r(REG_A_CHIP, 0x8000, m342.prg_bank_a_mapped);
			setprg8r(REG_B_CHIP, 0xA000, m342.prg_bank_b_mapped);
			setprg8r(REG_C_CHIP, 0xC000, m342.prg_bank_c_mapped);
			setprg8r(REG_D_CHIP, 0xE000, m342.prg_bank_d_mapped);
			break;
		case 5:
			setprg8r(REG_C_CHIP, 0x8000, m342.prg_bank_c_mapped);
			setprg8r(REG_B_CHIP, 0xA000, m342.prg_bank_b_mapped);
			setprg8r(REG_A_CHIP, 0xC000, m342.prg_bank_a_mapped);
			setprg8r(REG_D_CHIP, 0xE000, m342.prg_bank_d_mapped);
			break;
		case 6:
			setprg32r(REG_B_CHIP, 0x8000, m342.prg_bank_b_mapped >> 2);
			break;
		case 7:
			setprg32r(REG_A_CHIP, 0x8000, m342.prg_bank_a_mapped >> 2);
			break;
		}
	} else {
		setprg32r(CFI_CHIP, 0x8000, 0);
	}

	if (m342.map_rom_on_6000) {
		setprg8(0x6000, m342.prg_bank_6000_mapped); /* Map ROM on $6000-$7FFF */
	} else if (WRAMSIZE) {
		setprg8r(WRAM_CHIP, 0x6000, m342.sram_page); /* Select SRAM page */
	}
}

static void SyncCHR(void) {
	/* calculate CHR shift */
	uint8_t chr_shift = ((m342.mapper == 24) && (m342.flags & 0x02)) ? 1 : 0;

	/* enable or disable writes to CHR RAM, setup CHR mask */
	SetupCartCHRMapping(0, ROM.chr.data, ((((~m342.chr_mask & 0x3F) + 1) * 0x2000 - 1) & (CHR_SIZE - 1)) + 1, m342.can_write_chr);

	switch (m342.chr_mode & 0x07) {
	default:
	case 0:
		setchr8(m342.chr_bank_a >> 3 >> chr_shift);
		break;
	case 1:
		setchr4(0x0000, m342.mapper163.latch >> chr_shift);
		setchr4(0x1000, m342.mapper163.latch >> chr_shift);
		break;
	case 2:
		setchr2(0x0000, m342.chr_bank_a >> 1 >> chr_shift);
		m342.TKSMIR[0] = m342.TKSMIR[1] = m342.chr_bank_a;
		setchr2(0x0800, m342.chr_bank_c >> 1 >> chr_shift);
		m342.TKSMIR[2] = m342.TKSMIR[3] = m342.chr_bank_c;
		setchr1(0x1000, m342.chr_bank_e >> chr_shift);
		m342.TKSMIR[4] = m342.chr_bank_e;
		setchr1(0x1400, m342.chr_bank_f >> chr_shift);
		m342.TKSMIR[5] = m342.chr_bank_f;
		setchr1(0x1800, m342.chr_bank_g >> chr_shift);
		m342.TKSMIR[6] = m342.chr_bank_g;
		setchr1(0x1C00, m342.chr_bank_h >> chr_shift);
		m342.TKSMIR[7] = m342.chr_bank_h;
		break;
	case 3:
		setchr1(0x0000, m342.chr_bank_e >> chr_shift);
		m342.TKSMIR[0] = m342.chr_bank_e;
		setchr1(0x0400, m342.chr_bank_f >> chr_shift);
		m342.TKSMIR[1] = m342.chr_bank_f;
		setchr1(0x0800, m342.chr_bank_g >> chr_shift);
		m342.TKSMIR[2] = m342.chr_bank_g;
		setchr1(0x0C00, m342.chr_bank_h >> chr_shift);
		m342.TKSMIR[3] = m342.chr_bank_h;
		setchr2(0x1000, m342.chr_bank_a >> 1 >> chr_shift);
		m342.TKSMIR[4] = m342.TKSMIR[5] = m342.chr_bank_a;
		setchr2(0x1800, m342.chr_bank_c >> 1 >> chr_shift);
		m342.TKSMIR[6] = m342.TKSMIR[7] = m342.chr_bank_c;
		break;
	case 4:
		setchr4(0x0000, m342.chr_bank_a >> 2 >> chr_shift);
		setchr4(0x1000, m342.chr_bank_e >> 2 >> chr_shift);
		break;
	case 5:
		if (!m342.mmc2and4.latch0) {
			setchr4(0x0000, m342.chr_bank_a >> 2 >> chr_shift);
		} else {
			setchr4(0x0000, m342.chr_bank_b >> 2 >> chr_shift);
		}
		if (!m342.mmc2and4.latch1) {
			setchr4(0x1000, m342.chr_bank_e >> 2 >> chr_shift);
		} else {
			setchr4(0x1000, m342.chr_bank_f >> 2 >> chr_shift);
		}
		break;
	case 6:
		setchr2(0x0000, m342.chr_bank_a >> 1 >> chr_shift);
		setchr2(0x0800, m342.chr_bank_c >> 1 >> chr_shift);
		setchr2(0x1000, m342.chr_bank_e >> 1 >> chr_shift);
		setchr2(0x1800, m342.chr_bank_g >> 1 >> chr_shift);
		break;
	case 7:
		setchr1(0x0000, m342.chr_bank_a >> chr_shift);
		setchr1(0x0400, m342.chr_bank_b >> chr_shift);
		setchr1(0x0800, m342.chr_bank_c >> chr_shift);
		setchr1(0x0C00, m342.chr_bank_d >> chr_shift);
		setchr1(0x1000, m342.chr_bank_e >> chr_shift);
		setchr1(0x1400, m342.chr_bank_f >> chr_shift);
		setchr1(0x1800, m342.chr_bank_g >> chr_shift);
		setchr1(0x1C00, m342.chr_bank_h >> chr_shift);
		break;
	}
}

static void SyncMirror(void) {
	if (m342.fourscreen) {
		setmirror(MI_4);
	} else {
		if (!((m342.mapper == 20) && (m342.flags & 0x01))) { /* Mapper #189? */
			switch (m342.mirroring) {
			case 0:
				setmirror(MI_V);
				break;
			case 1:
				setmirror(MI_H);
				break;
			case 2:
				setmirror(MI_0);
				break;
			case 3:
				setmirror(MI_1);
				break;
			}
		}
	}
}

static void Sync(void) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static DECLFW(COOLGIRL_Flash_Write) {
	if (m342.flash_state < sizeof(m342.flash_buffer_a) / sizeof(m342.flash_buffer_a[0])) {
		m342.flash_buffer_a[m342.flash_state] = A & 0x0FFF;
		m342.flash_buffer_v[m342.flash_state] = V;
		m342.flash_state++;

		/* enter CFI mode */
		if ((m342.flash_state == 1) &&
			(m342.flash_buffer_a[0] == 0x0AAA) && (m342.flash_buffer_v[0] == 0x98)) {
			m342.cfi_mode = 1;
			m342.flash_state = 0;
		}

		/* sector erase */
		if ((m342.flash_state == 6) &&
			(m342.flash_buffer_a[0] == 0x0AAA) && (m342.flash_buffer_v[0] == 0xAA) &&
			(m342.flash_buffer_a[1] == 0x0555) && (m342.flash_buffer_v[1] == 0x55) &&
			(m342.flash_buffer_a[2] == 0x0AAA) && (m342.flash_buffer_v[2] == 0x80) &&
			(m342.flash_buffer_a[3] == 0x0AAA) && (m342.flash_buffer_v[3] == 0xAA) &&
			(m342.flash_buffer_a[4] == 0x0555) && (m342.flash_buffer_v[4] == 0x55) &&
			(m342.flash_buffer_v[5] == 0x30)) {
			int sector = m342.prg_bank_a_mapped * 0x2000 / FLASH_SECTOR_SIZE;
			uint32_t sector_address = sector * FLASH_SECTOR_SIZE;
			uint32_t i;
			for (i = sector_address; i < sector_address + FLASH_SECTOR_SIZE; i++) {
				SAVE_FLASH[i % SAVE_FLASH_SIZE] = 0xFF;
			}
			FCEU_printf("Flash sector #%d is erased: 0x%08x - 0x%08x.\n", sector, sector_address, sector_address + FLASH_SECTOR_SIZE - 1);
			m342.flash_state = 0;
		}

		/* write byte */
		if ((m342.flash_state == 4) &&
			(m342.flash_buffer_a[0] == 0x0AAA) && (m342.flash_buffer_v[0] == 0xAA) &&
			(m342.flash_buffer_a[1] == 0x0555) && (m342.flash_buffer_v[1] == 0x55) &&
			(m342.flash_buffer_a[2] == 0x0AAA) && (m342.flash_buffer_v[2] == 0xA0)) {
			/*int sector = m342.prg_bank_a_mapped * 0x2000 / FLASH_SECTOR_SIZE; */
			uint32_t flash_addr = m342.prg_bank_a_mapped * 0x2000 + (A % 0x8000);
			if (SAVE_FLASH[flash_addr % SAVE_FLASH_SIZE] != 0xFF) {
				if (!(show_error_log & 2)) {
					FCEU_PrintError("Error: can't write to 0x%08x, flash sector is not erased.\n", flash_addr);
					show_error_log |= 2; /* show error log only on reset or power-on */
				}
			} else {
				SAVE_FLASH[flash_addr % SAVE_FLASH_SIZE] = V;
			}
			m342.flash_state = 0;
		}
	}

	/* not a command */
	if (((A & 0xFFF) != 0x0AAA) && ((A & 0xFFF) != 0x0555)) {
		m342.flash_state = 0;
	}

	/* reset */
	if (V == 0xF0) {
		m342.flash_state = 0;
		m342.cfi_mode = 0;
	}

	SyncPRG();
}

static DECLFW(Write4F) {
	if (m342.sram_enabled && (A >= 0x6000) && (A < 0x8000) && !m342.map_rom_on_6000) {
		CartBW(A, V); /* SRAM is enabled and writable */
	}

	if (SAVE_FLASH && m342.can_write_flash && (A >= 0x8000)) {
		/* writing flash */
		COOLGIRL_Flash_Write(A, V);
	}

	/* block two writes in a row */
	if ((timestampbase + timestamp) < (m342.mmc1.lreset + 2)) {
		return;
	}
	m342.mmc1.lreset = timestampbase + timestamp;

	if ((A >= 0x5000) && (A < 0x6000) && !m342.lockout) {
		switch (A & 0x07) {
		case 0:
			/* use bits 29-27 to simulate flash memory */
			m342.prg_base = (m342.prg_base & 0xFF) | (V << 8);
			break;
		case 1:
			m342.prg_base = (m342.prg_base & 0xFF00) | V;
			break;
		case 2:
			m342.chr_mask = (m342.chr_mask & 0x1F) | ((V & 0x80) >> 2);
			m342.prg_mask = V & 0x7F;
			break;
		case 3:
			m342.prg_mode = V >> 5;
			m342.chr_bank_a = (m342.chr_bank_a & 0x07) | (V << 3);
			break;
		case 4:
			m342.chr_mode = V >> 5;
			m342.chr_mask = (m342.chr_mask & 0x20) | (V & 0x1F);
			break;
		case 5:
			m342.chr_bank_a = (m342.chr_bank_a & 0xFF) | ((V & 0x80) << 1);
			m342.prg_bank_a = (m342.prg_bank_a & 0xC1) | ((V & 0x7C) >> 1);
			m342.sram_page = V & 0x03;
			break;
		case 6:
			m342.flags = V >> 5;
			m342.mapper = (m342.mapper & 0x20) | (V & 0x1F);
			break;
		case 7:
			m342.lockout = V >> 7;
			m342.mapper = (m342.mapper & 0x1F) | ((V & 0x40) >> 1);
			m342.fourscreen = (V & 0x20) >> 5;
			m342.mirroring = (V & 0x18) >> 3;
			m342.can_write_flash = (V & 0x04) >> 2;
			m342.can_write_chr = (V & 0x02) >> 1;
			m342.sram_enabled = V & 0x01;
			switch (m342.mapper) {
			case 14:
				/* Mapper #65 - Irem's H3001 */
				m342.prg_bank_b = 1;
				break;
			case 17:
				/* MMC2 */
				m342.prg_bank_b = ~2;
				break;
			case 23:
				/* Mapper #42 */
				m342.map_rom_on_6000 = 1;
				break;
			}
			break;
		}
		if (m342.lockout) {
			FCEU_printf(" mapper code = %2d flags: %02x four-screen = %d\n", m342.mapper, m342.flags, m342.fourscreen);
		}
	}

	/* $0000-$7FFF */
	if (A < 0x8000) {
		/* Mapper #163 */
		if (m342.mapper == 6) {
			if (A == 0x5101) {
				if (m342.mapper163.r4 && !V) {
					m342.mapper163.r5 ^= 1;
				}
				m342.mapper163.r4 = V;
			} else if ((A == 0x5100) && (V == 6)) {
				m342.prg_mode = m342.prg_mode & 0xFE;
				m342.prg_bank_b = 12;
			} else {
				if ((A & 0x7000) == 0x5000) {
					switch ((A & 0x300) >> 8) {
					case 2:
						m342.prg_mode |= 1;
						m342.prg_bank_a = (m342.prg_bank_a & 0x3F) | ((V & 0x03) << 6);
						m342.mapper163.r0 = V;
						break;
					case 0:
						m342.prg_mode |= 1;
						m342.prg_bank_a = (m342.prg_bank_a & 0xC3) | ((V & 0x0F) << 2);
						m342.chr_mode = (m342.chr_mode & 0xFE) | (V >> 7);
						m342.mapper163.r1 = V;
						break;
					case 3:
						m342.mapper163.r2 = V;
						break;
					case 1:
						m342.mapper163.r3 = V;
						break;
					}
				}
			}
		}

		/* Mapper #87 */
		if (m342.mapper == 12) {
			/* $6000-$7FFF */
			if ((A & 0x6000) == 0x6000) {
				m342.chr_bank_a = (m342.chr_bank_a & 0xE7) | ((V & 0x01) << 4) | ((V & 0x02) << 2);
			}
		}

		/* Mapper #90 - JY */
		/*
		if (m342.mapper == 13)
		{
			switch (A)
			{
			case 0x5800: m342.mapper90.mul1 = V; break;
			case 0x5801: m342.mapper90.mul2 = V; break;
			}
		}
		*/

		/* MMC5 (not really) */
		if (m342.mapper == 15) {
			switch (A) {
			case 0x5105:
				if (V == 0xFF) {
					m342.fourscreen = 1;
				} else {
					m342.fourscreen = 0;
					switch (((V >> 2) & 0x01) | ((V >> 3) & 0x02)) {
					case 0:
						m342.mirroring = 2;
						break;
					case 1:
						m342.mirroring = 0;
						break;
					case 2:
						m342.mirroring = 1;
						break;
					case 3:
						m342.mirroring = 3;
						break;
					}
				}
				break;
			case 0x5115:
				m342.prg_bank_a = V & 0x1E;
				m342.prg_bank_b = (V & 0x1E) | 1;
				break;
			case 0x5116:
				m342.prg_bank_c = V & 0x1F;
				break;
			case 0x5117:
				m342.prg_bank_d = V & 0x1F;
				break;
			case 0x5120:
				m342.chr_bank_a = V;
				break;
			case 0x5121:
				m342.chr_bank_b = V;
				break;
			case 0x5122:
				m342.chr_bank_c = V;
				break;
			case 0x5123:
				m342.chr_bank_d = V;
				break;
			case 0x5128:
				m342.chr_bank_e = V;
				break;
			case 0x5129:
				m342.chr_bank_f = V;
				break;
			case 0x512A:
				m342.chr_bank_g = V;
				break;
			case 0x512B:
				m342.chr_bank_h = V;
				break;
			case 0x5203:
				X6502_IRQEnd(FCEU_IQEXT);
				m342.mmc5.irq_out = 0;
				m342.mmc5.irq_line = V;
				break;
			case 0x5204:
				X6502_IRQEnd(FCEU_IQEXT);
				m342.mmc5.irq_out = 0;
				m342.mmc5.irq_enabled = (V & 0x80) >> 7;
				break;
			}
		}

		/* Mapper #189 */
		if (m342.mapper == 20) {
			/* $4120-$7FFF */
			if ((m342.flags & 0x02) && (A >= 0x4120)) {
				m342.prg_bank_a = (m342.prg_bank_a & 0xC3) | ((V & 0x0F) << 2) | ((V & 0xF0) >> 2);
			}
		}

		/* Mappers #79 and #146 - NINA-03/06 and Sachen 3015: (flag0 = 1) */
		if (m342.mapper == 27) {
			if ((A & 0x6100) == 0x4100) {
				m342.chr_bank_a = (m342.chr_bank_a & 0xC7) | ((V & 0x07) << 3);
				m342.prg_bank_a = (m342.chr_bank_a & 0xF8) | ((V & 0x08) >> 1);
			}
		}

		/* Mapper #133 */
		if (m342.mapper == 28) {
			if ((A & 0x6100) == 0x4100) {
				m342.chr_bank_a = (m342.chr_bank_a & 0xE7) | ((V & 0x03) << 3);
				m342.prg_bank_a = (m342.chr_bank_a & 0xF8) | (V & 0x04);
			}
		}

		/* Mapper #184 */
		if (m342.mapper == 31) {
			if ((A & 0x6000) == 0x6000) {
				m342.chr_bank_a = (m342.chr_bank_a & 0xE3) | ((V & 0x07) << 2);
				m342.chr_bank_e = (m342.chr_bank_e & 0xE3) | ((V & 0x30) >> 2) | 0x10;
			}
		}

		/* Mapper #38 */
		if (m342.mapper == 32) {
			if ((A & 0x7000) == 0x7000) {
				m342.prg_bank_a = (m342.prg_bank_a & 0xF7) | ((V & 0x03) << 2);
				m342.chr_bank_a = (m342.chr_bank_a & 0xE7) | ((V & 0x0C) << 1);
			}
		}
	} else { /* $8000-$FFFF */
		/* Mapper #2 - UxROM */
		/* flag0 - m342.mapper #71 - for Fire Hawk only. */
		/* other m342.mapper-#71 games are UxROM */
		if (m342.mapper == 1) {
			if (!(m342.flags & 0x01) || ((A & 0x7000) != 0x1000)) {
				/* UxROM_BITSIZE = 4 */
				m342.prg_bank_a = (m342.prg_bank_a & 0xC1) | ((V & 0x1F) << 1);
				if (m342.flags & 0x02) {
					/* One screen m342.mirroring select, CHR RAM bank, PRG ROM bank */
					m342.mirroring = 0x02 | (V >> 7);
					m342.chr_bank_a = (m342.chr_bank_a & 0xFC) | ((V & 0x60) >> 5);
				}
			} else {
				/* CodeMasters, blah. Mirroring control used only by Fire Hawk */
				m342.mirroring = 0x02 | ((V >> 4) & 0x01);
			}
		}

		/* Mapper #3 - CNROM */
		if (m342.mapper == 2) {
			m342.chr_bank_a = (m342.chr_bank_a & 0x07) | ((V & 0x1F) << 3);
		}

		/* Mapper #78 - Holy Diver */
		if (m342.mapper == 3) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xF1) | ((V & 0x07) << 1);
			m342.chr_bank_a = (m342.chr_bank_a & 0x87) | ((V & 0xF0) >> 1);
			m342.mirroring = ((V >> 3) & 0x01) ^ 0x01;
		}

		/* Mapper #97 - Irem's TAM-S1 */
		if (m342.mapper == 4) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xC1) | ((V & 0x1F) << 1);
			m342.mirroring = (V >> 7) ^ 0x01;
		}

		/* Mapper #93 - Sunsoft-2 */
		if (m342.mapper == 5) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xF1) | ((V & 0x70) >> 3);
			m342.can_write_chr = V & 0x01;
		}

		/* Mapper #18 */
		if (m342.mapper == 7) {
			switch (((A & 0x7000) >> 10) | (A & 0x03)) {
			case 0: /* $8000 */
				m342.prg_bank_a = (m342.prg_bank_a & 0xF0) | (V & 0x0F);
				break;
			case 1: /* $8001 */
				m342.prg_bank_a = (m342.prg_bank_a & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 2: /* $8002 */
				m342.prg_bank_b = (m342.prg_bank_b & 0xF0) | (V & 0x0F);
				break;
			case 3: /* $8003 */
				m342.prg_bank_b = (m342.prg_bank_b & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 4: /* $9000 */
				m342.prg_bank_c = (m342.prg_bank_c & 0xF0) | (V & 0x0F);
				break;
			case 5: /* $9001 */
				m342.prg_bank_c = (m342.prg_bank_c & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 6:
				break;
			case 7:
				break;
			case 8: /* $A000 */
				m342.chr_bank_a = (m342.chr_bank_a & 0xF0) | (V & 0x0F);
				break;
			case 9: /* $A001 */
				m342.chr_bank_a = (m342.chr_bank_a & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 10: /* $A002 */
				m342.chr_bank_b = (m342.chr_bank_b & 0xF0) | (V & 0x0F);
				break;
			case 11: /* $A003 */
				m342.chr_bank_b = (m342.chr_bank_b & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 12: /* $B000 */
				m342.chr_bank_c = (m342.chr_bank_c & 0xF0) | (V & 0x0F);
				break;
			case 13: /* $B001 */
				m342.chr_bank_c = (m342.chr_bank_c & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 14: /* $B002 */
				m342.chr_bank_d = (m342.chr_bank_d & 0xF0) | (V & 0x0F);
				break;
			case 15: /* $B003 */
				m342.chr_bank_d = (m342.chr_bank_d & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 16: /* $C000 */
				m342.chr_bank_e = (m342.chr_bank_e & 0xF0) | (V & 0x0F);
				break;
			case 17: /* $C001 */
				m342.chr_bank_e = (m342.chr_bank_e & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 18: /* $C002 */
				m342.chr_bank_f = (m342.chr_bank_f & 0xF0) | (V & 0x0F);
				break;
			case 19: /* $C003 */
				m342.chr_bank_f = (m342.chr_bank_f & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 20: /* $D000 */
				m342.chr_bank_g = (m342.chr_bank_g & 0xF0) | (V & 0x0F);
				break;
			case 21: /* $D001 */
				m342.chr_bank_g = (m342.chr_bank_g & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 22: /* $D002 */
				m342.chr_bank_h = (m342.chr_bank_h & 0xF0) | (V & 0x0F);
				break;
			case 23: /* $D003 */
				m342.chr_bank_h = (m342.chr_bank_h & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 24: /* $E000 */
				m342.mapper18.irq_latch = (m342.mapper18.irq_latch & 0xFFF0) | (V & 0x0F);
				break;
			case 25: /* $E001 */
				m342.mapper18.irq_latch = (m342.mapper18.irq_latch & 0xFF0F) | ((V & 0x0F) << 4);
				break;
			case 26: /* $E002 */
				m342.mapper18.irq_latch = (m342.mapper18.irq_latch & 0xF0FF) | ((V & 0x0F) << 8);
				break;
			case 27: /* $E003 */
				m342.mapper18.irq_latch = (m342.mapper18.irq_latch & 0x0FFF) | ((V & 0x0F) << 12);
				break;
			case 28: /* $F000 */
				X6502_IRQEnd(FCEU_IQEXT);
				m342.mapper18.irq_value = m342.mapper18.irq_latch;
				break;
			case 29: /* $F001 */
				X6502_IRQEnd(FCEU_IQEXT);
				m342.mapper18.irq_control = V & 0x0F;
				break;
			case 30:
				switch (A & 0x03) {
				case 0:
					m342.mirroring = 1;
					break; /* Horz */
				case 1:
					m342.mirroring = 0;
					break; /* Vert */
				case 2:
					m342.mirroring = 2;
					break; /* 1SsA */
				case 3:
					m342.mirroring = 3;
					break; /* 1SsB */
				}
				break;
			case 31:
				break; /* sound */
			}
		}

		/* Mapper #7 - AxROM, m342.mapper #241 - BNROM */
		if (m342.mapper == 8) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xC3) | ((V & 0x0F) << 2);
			if (!(m342.flags & 0x01)) {
				m342.mirroring = 0x02 | ((V >> 4) & 0x01);
			}
		}

		/* Mapper #228 - Cheetahmen II */
		if (m342.mapper == 9) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xC3) | ((A & 0x0780) >> 5);
			m342.chr_bank_a = (m342.chr_bank_a & 0x07) | ((A & 0x0007) << 5) | ((V & 0x03) << 3);
			m342.mirroring = (A >> 13) & 0x01;
		}

		/* Mapper #11 - ColorDreams */
		if (m342.mapper == 10) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xF3) | ((V & 0x03) << 2);
			m342.chr_bank_a = (m342.chr_bank_a & 0x87) | ((V & 0xF0) >> 1);
		}

		/* Mapper #66 - GxROM */
		if (m342.mapper == 11) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xF3) | ((V & 0x30) >> 2);
			m342.chr_bank_a = (m342.chr_bank_a & 0xE7) | ((V & 0x03) << 3);
		}

		/* Mapper #90 - JY */
		if (m342.mapper == 13) {
			switch (A & 0xF000) {
			case 0x8000:
				switch (A & 0x03) {
				case 0:
					m342.prg_bank_a = (m342.prg_bank_a & 0xC0) | (V & 0x3F);
					break;
				case 1:
					m342.prg_bank_b = (m342.prg_bank_b & 0xC0) | (V & 0x3F);
					break;
				case 2:
					m342.prg_bank_c = (m342.prg_bank_c & 0xC0) | (V & 0x3F);
					break;
				case 3:
					m342.prg_bank_d = (m342.prg_bank_d & 0xC0) | (V & 0x3F);
					break;
				}
				break;
			case 0x9000:
				switch (A & 0x07) {
				case 0:
					m342.chr_bank_a = V;
					break; /* $9000 */
				case 1:
					m342.chr_bank_b = V;
					break; /* $9001 */
				case 2:
					m342.chr_bank_c = V;
					break; /* $9002 */
				case 3:
					m342.chr_bank_d = V;
					break; /* $9003 */
				case 4:
					m342.chr_bank_e = V;
					break; /* $9004 */
				case 5:
					m342.chr_bank_f = V;
					break; /* $9005 */
				case 6:
					m342.chr_bank_g = V;
					break; /* $9006 */
				case 7:
					m342.chr_bank_h = V;
					break; /* $9007 */
				}
				break;
			case 0xC000:
				/* use MMC3's IRQs */
				switch (A & 0x07) {
				case 0:
					if (V & 0x01) {
						m342.mmc3.irq_enabled = 1;
					} else {
						X6502_IRQEnd(FCEU_IQEXT);
						m342.mmc3.irq_enabled = 0;
					}
					break;
				case 1:
					break; /* who cares about this shit? */
				case 2:
					m342.mmc3.irq_enabled = 0;
					X6502_IRQEnd(FCEU_IQEXT);
					break;
				case 3:
					m342.mmc3.irq_enabled = 1;
					break;
				case 4:
					break; /* prescaler? who cares? */
				case 5:
					m342.mmc3.irq_latch = V ^ m342.mapper90.xor;
					m342.mmc3.irq_reload = 1;
					break;
				case 6:
					m342.mapper90.xor = V;
					break;
				case 7:
					break; /* meh */
				}
				break;
			case 0xD000:
				if ((A & 0x03) == 1) {
					m342.mirroring = V & 0x03;
				}
				break;
			}
		}

		/* Mapper #65 - Irem's H3001 */
		if (m342.mapper == 14) {
			switch (((A & 0x7000) >> 9) | (A & 0x07)) {
			case 0: /* $8000 */
				m342.prg_bank_a = (m342.prg_bank_a & 0xC0) | (V & 0x3F);
				break;
			case 9: /* $9001, m342.mirroring */
				m342.mirroring = (V >> 7) & 0x01;
				break;
			case 11: /* $9003, enable IRQ */
				X6502_IRQEnd(FCEU_IQEXT);
				m342.mapper65.irq_enabled = V >> 7;
				break;
			case 12: /* $9004 */
				X6502_IRQEnd(FCEU_IQEXT); /* mapper65_irq_out = 0; */ /* ack */
				m342.mapper65.irq_value = m342.mapper65.irq_latch; /* $9004, IRQ reload */
				break;
			case 13: /* $9005, IRQ high V */
				m342.mapper65.irq_latch = (m342.mapper65.irq_latch & 0x00FF) | (V << 8);
				break;
			case 14: /* $9006, IRQ low V */
				m342.mapper65.irq_latch = (m342.mapper65.irq_latch & 0xFF00) | V;
				break;
			case 16: /* $A000 */
				m342.prg_bank_b = (m342.prg_bank_b & 0xC0) | (V & 0x3F);
				break;
			case 24: /* $B000 */
				m342.chr_bank_a = V;
				break;
			case 25: /* $B001 */
				m342.chr_bank_b = V;
				break;
			case 26: /* $B002 */
				m342.chr_bank_c = V;
				break;
			case 27: /* $B003 */
				m342.chr_bank_d = V;
				break;
			case 28: /* $B004 */
				m342.chr_bank_e = V;
				break;
			case 29: /* $B005 */
				m342.chr_bank_f = V;
				break;
			case 30: /* $B006 */
				m342.chr_bank_g = V;
				break;
			case 31: /* $B007 */
				m342.chr_bank_h = V;
				break;
			case 32: /* $C000 */
				m342.prg_bank_c = (m342.prg_bank_c & 0xC0) | (V & 0x3F);
				break;
			}
		}

		/* Mapper #1 - MMC1 */
		/*
		r0 - load register
		flag0 - 16KB of SRAM (SOROM)
		*/
		if (m342.mapper == 16) {
			if (V & 0x80) {
				/* reset */
				m342.mmc1.load_register = (m342.mmc1.load_register & 0xC0) | 0x20;
				m342.prg_mode = 0;
				m342.prg_bank_c = (m342.prg_bank_c & 0xE0) | 0x1E;
			} else {
				m342.mmc1.load_register = (m342.mmc1.load_register & 0xC0) | ((V & 0x01) << 5) | ((m342.mmc1.load_register & 0x3E) >> 1);
				if (m342.mmc1.load_register & 0x01) {
					switch (A & 0xE000) {
					case 0x8000:
						if ((m342.mmc1.load_register & 0x18) == 0x18) {
							m342.prg_mode = 0;
							m342.prg_bank_c = (m342.prg_bank_c & 0xE1) | 0x1E;
						} else if ((m342.mmc1.load_register & 0x18) == 0x10) {
							m342.prg_mode = 1;
							m342.prg_bank_c = (m342.prg_bank_c & 0xE1);
						} else {
							m342.prg_mode = 7;
						}
						if (m342.mmc1.load_register & 0x20) {
							m342.chr_mode = 4;
						} else {
							m342.chr_mode = 0;
						}
						m342.mirroring = ((m342.mmc1.load_register >> 1) & 0x03) ^ 0x02;
						break;
					case 0xA000:
						m342.chr_bank_a = (m342.chr_bank_a & 0x83) | ((m342.mmc1.load_register & 0x3E) << 1);
						if (m342.flags & 0x01) {
							/* (m342.flags[0]) - 16KB of SRAM */
							/* PRG RAM page #2 is battery backed */
							m342.sram_page = 0x02 | (((m342.mmc1.load_register >> 4) & 0x01) ^ 0x01);
						}
						m342.prg_bank_a = (m342.prg_bank_a & 0xDF) | (m342.mmc1.load_register & 0x20); /* for SUROM, 512k PRG support */
						m342.prg_bank_c = (m342.prg_bank_c & 0xDF) | (m342.mmc1.load_register & 0x20); /* for SUROM, 512k PRG support */
						break;
					case 0xC000:
						m342.chr_bank_e = (m342.chr_bank_e & 0x83) | ((m342.mmc1.load_register & 0x3E) << 1);
						break;
					case 0xE000:
						m342.prg_bank_a = (m342.prg_bank_a & 0xE1) | (m342.mmc1.load_register & 0x1E);
						m342.sram_enabled = ((m342.mmc1.load_register >> 5) & 0x01) ^ 0x01;
						break;
					}
					m342.mmc1.load_register = 0x20;
				}
			}
		}

		/* Mapper #9 and #10 - MMC2 and MMC4 */
		/* flag0 - 0=MMC2, 1=MMC4 */
		if (m342.mapper == 17) {
			switch ((A >> 12) & 0x07) {
			case 2: /* $A000-$AFFF */
				if (!(m342.flags & 0x01)) {
					/* MMC2 */
					m342.prg_bank_a = (m342.prg_bank_a & 0xF0) | (V & 0x0F);
				} else {
					/* MMC4 */
					m342.prg_bank_a = (m342.prg_bank_a & 0xE1) | ((V & 0x0F) << 1);
				}
				break;
			case 3: /* $B000-$BFFF */
				m342.chr_bank_a = (m342.chr_bank_a & 0x83) | ((V & 0x1F) << 2);
				break;
			case 4: /* $C000-$CFFF */
				m342.chr_bank_b = (m342.chr_bank_b & 0x83) | ((V & 0x1F) << 2);
				break;
			case 5: /* $D000-$DFFF */
				m342.chr_bank_e = (m342.chr_bank_e & 0x83) | ((V & 0x1F) << 2);
				break;
			case 6: /* $E000-$EFFF */
				m342.chr_bank_f = (m342.chr_bank_f & 0x83) | ((V & 0x1F) << 2);
				break;
			case 7: /* $F000-$FFFF */
				m342.mirroring = V & 0x01;
				break;
			}
		}

		/* Mapper #152 */
		if (m342.mapper == 18) {
			m342.chr_bank_a = (m342.chr_bank_a & 0x87) | ((V & 0x0F) << 3);
			m342.prg_bank_a = (m342.prg_bank_a & 0xF1) | ((V & 0x70) >> 3);
			m342.mirroring = 0x02 | (V >> 7);
		}

		/* Mapper #73 - VRC3 */
		if (m342.mapper == 19) {
			switch (A & 0xF000) {
			case 0x8000:
				m342.vrc3.irq_latch = (m342.vrc3.irq_latch & 0xFFF0) | (V & 0x0F);
				break;
			case 0x9000:
				m342.vrc3.irq_latch = (m342.vrc3.irq_latch & 0xFF0F) | ((V & 0x0F) << 4);
				break;
			case 0xA000:
				m342.vrc3.irq_latch = (m342.vrc3.irq_latch & 0xF0FF) | ((V & 0x0F) << 8);
				break;
			case 0xB000:
				m342.vrc3.irq_latch = (m342.vrc3.irq_latch & 0x0FFF) | ((V & 0x0F) << 12);
				break;
			case 0xC000:
				X6502_IRQEnd(FCEU_IQEXT); /* ack */
				m342.vrc3.irq_control = (m342.vrc3.irq_control & 0xF8) | (V & 0x07);
				if (m342.vrc3.irq_control & 0x02) {
					m342.vrc3.irq_value = m342.vrc3.irq_latch;
				}
				break;
			case 0xD000:
				X6502_IRQEnd(FCEU_IQEXT); /* ack */
				m342.vrc3.irq_control = (m342.vrc3.irq_control & 0xFD) | (m342.vrc3.irq_control & 0x01) << 1;
				break;
			case 0xE000:
				break;
			case 0xF000:
				m342.prg_bank_a = (m342.prg_bank_a & 0xF1) | ((V & 0x07) << 1);
				break;
			}
		}

		/* Mapper #4 - MMC3/MMC6 */
		/*
		flag0 - TxSROM
		flag1 - m342.mapper #189
		*/
		if (m342.mapper == 20) {
			switch (((A & 0x6000) >> 12) | (A & 0x01)) {
			case 0: /* $8000-$9FFE, even */
				m342.mmc3.internal = (m342.mmc3.internal & 0xF8) | (V & 0x07);
				if (!(m342.flags & 0x02) && !(m342.flags & 0x04)) {
					if (V & 0x40) {
						m342.prg_mode = 5;
					} else {
						m342.prg_mode = 4;
					}
				}
				if (!(m342.flags & 0x04)) {
					if (V & 0x80) {
						m342.chr_mode = 3;
					} else {
						m342.chr_mode = 2;
					}
				}
				break;
			case 1: /* $8001-$9FFF, odd */
				switch (m342.mmc3.internal & 0x07) {
				case 0:
					m342.chr_bank_a = V;
					break;
				case 1:
					m342.chr_bank_c = V;
					break;
				case 2:
					m342.chr_bank_e = V;
					break;
				case 3:
					m342.chr_bank_f = V;
					break;
				case 4:
					m342.chr_bank_g = V;
					break;
				case 5:
					m342.chr_bank_h = V;
					break;
				case 6:
					if (!(m342.flags & 0x02)) {
						m342.prg_bank_a = V;
					}
					break;
				case 7:
					if (!(m342.flags & 0x02)) {
						m342.prg_bank_b = V;
					}
					break;
				}
				break;
			case 2: /* $A000-$BFFE, even (m342.mirroring) */
				if (!(m342.flags & 0x04)) {
					m342.mirroring = V & 0x01;
				}
				break;
			case 3: /* RAM protect... no */
				break;
			case 4: /* $C000-$DFFE, even (IRQ latch) */
				m342.mmc3.irq_latch = V;
				break;
			case 5: /* $C001-$DFFF, odd */
				m342.mmc3.irq_reload = 1;
				break;
			case 6: /* $E000-$FFFE, even */
				X6502_IRQEnd(FCEU_IQEXT);
				m342.mmc3.irq_enabled = 0;
				break;
			case 7: /* $E001-$FFFF, odd */
				if (!(m342.flags & 0x04)) {
					m342.mmc3.irq_enabled = 1;
				}
				break;
			}
		}

		/* Mapper #112 */
		if (m342.mapper == 21) {
			switch (A & 0xE000) {
			case 0x8000:
				m342.mapper112.internal = (m342.mapper112.internal & 0xF8) | (V & 0x07);
				break;
			case 0xA000:
				switch (m342.mapper112.internal & 0x07) {
				case 0:
					m342.prg_bank_a = (m342.prg_bank_a & 0xC0) | (V & 0x3F);
					break;
				case 1:
					m342.prg_bank_b = (m342.prg_bank_b & 0xC0) | (V & 0x3F);
					break;
				case 2:
					m342.chr_bank_a = V;
					break;
				case 3:
					m342.chr_bank_c = V;
					break;
				case 4:
					m342.chr_bank_e = V;
					break;
				case 5:
					m342.chr_bank_f = V;
					break;
				case 6:
					m342.chr_bank_g = V;
					break;
				case 7:
					m342.chr_bank_h = V;
					break;
				}
				break;
			case 0xC000:
				break;
			case 0xE000:
				m342.mirroring = V & 0x01;
				break;
			}
		}

		/* Mappers #33 + #48 - Taito */
		/* flag0=0 - #33, flag0=1 - #48 */
		if (m342.mapper == 22) {
			switch (((A & 0x6000) >> 11) | (A & 0x03)) {
			case 0: /* $8000, PRG Reg 0 (8k @ $8000) */
				m342.prg_bank_a = (m342.prg_bank_a & 0xC0) | (V & 0x3F);
				if (!(m342.flags & 0x01)) { /* 33 */
					m342.mirroring = (V >> 6) & 0x01;
				}
				break;
			case 1: /* $8001, PRG Reg 1 (8k @ $A000) */
				m342.prg_bank_b = (m342.prg_bank_b & 0xC0) | (V & 0x3F);
				break;
			case 2: /* $8002, CHR Reg 0 (2k @ $0000) */
				m342.chr_bank_a = V << 1;
				break;
			case 3: /* $8003, CHR Reg 1 (2k @ $0800) */
				m342.chr_bank_c = V << 1;
				break;
			case 4: /* $A000, CHR Reg 2 (1k @ $1000) */
				m342.chr_bank_e = V;
				break;
			case 5: /* $A001, CHR Reg 2 (1k @ $1400) */
				m342.chr_bank_f = V;
				break;
			case 6: /* $A002, CHR Reg 2 (1k @ $1800) */
				m342.chr_bank_g = V;
				break;
			case 7: /* $A003, CHR Reg 2 (1k @ $1C00) */
				m342.chr_bank_h = V;
				break;
			case 12: /* $E000, m342.mirroring, for m342.mapper #48 */
				if (m342.flags & 0x01) { /* 48 */
					m342.mirroring = (V >> 6) & 0x01;
				}
				break;
			case 8: /* $C000, IRQ latch */
				m342.mmc3.irq_latch = V ^ 0xFF;
				break;
			case 9: /* $C001, IRQ reload */
				m342.mmc3.irq_reload = 1;
				break;
			case 10: /* $C002, IRQ enable */
				m342.mmc3.irq_enabled = 1;
				break;
			case 11: /* $C003, IRQ disable & ack */
				m342.mmc3.irq_enabled = 0;
				X6502_IRQEnd(FCEU_IQEXT); /* ack */
				break;
			}
		}

		/* Mapper #42 */
		if (m342.mapper == 23) {
			switch (((A & 0x4000) >> 12) | (A & 0x03)) {
			case 0: /* $8000, CHR Reg (8k @ $8000) */
				m342.chr_bank_a = (m342.chr_bank_a & 0xE0) | ((V & 0x1F) << 3);
				break;
			case 4: /* $E000, PRG Reg (8k @ $6000) */
				m342.prg_bank_6000 = (m342.prg_bank_6000 & 0xF0) | (V & 0x0F);
				break;
			case 5: /* Mirroring */
				m342.mirroring = (V >> 3) & 0x01;
				break;
			case 6: /* IRQ */
				m342.mapper42.irq_enabled = (V & 0x02) >> 1;
				if (!m342.mapper42.irq_enabled) {
					X6502_IRQEnd(FCEU_IQEXT);
					m342.mapper42.irq_value = 0;
				}
				break;
			}
		}

		/* Mapper #23 - VRC2/4 */
		/*
		flag0 - switches A0 and A1 lines. 0=A0,A1 like VRC2b (m342.mapper #23), 1=A1,A0 like VRC2a(#22), VRC2c(#25)
		flag1 - divides CHR bank select by two (m342.mapper #22, VRC2a)
		*/
		if (m342.mapper == 24) {
			uint8_t vrc_2b_hi = 0;
			uint8_t vrc_2b_low = 0;
			uint8_t vrc_2b_addr = 0;

			if (vrc24_compatibility) {
				/* Compatibility code - for rom variants using the older firmware */
				vrc_2b_hi = ((A >> 1) & 0x01) | ((A >> 3) & 0x01) | ((A >> 5) & 0x01) | ((A >> 7) & 0x01);
				vrc_2b_low = (A & 0x01) | ((A >> 2) & 0x01) | ((A >> 4) & 0x01) | ((A >> 6) & 0x01);
				vrc_2b_addr =
					(((m342.flags & 0x01) ? vrc_2b_low : vrc_2b_hi) << 1) |
					((m342.flags & 0x01) ? vrc_2b_hi : vrc_2b_low);
			} else {
				/* Updated code, not compatible with earlier VRC24 cart variants */
				switch (m342.flags & 0x05) {
				case 0:
					vrc_2b_hi = (((A >> 7) & 0x01) | ((A >> 2) & 0x01)); /* m342.mapper #21 */
					vrc_2b_low = (((A >> 6) & 0x01) | ((A >> 1) & 0x01)); /* m342.mapper #21 */
					break;
				case 1:
					vrc_2b_hi = (A & 0x01); /* m342.mapper #22 */
					vrc_2b_low = ((A >> 1) & 0x01); /* m342.mapper #22 */
					break;
				case 4:
					vrc_2b_hi = (((A >> 5) & 0x01) | ((A >> 3) & 0x01) | ((A >> 1) & 0x01)); /* m342.mapper #23 */
					vrc_2b_low = (((A >> 4) & 0x01) | ((A >> 2) & 0x01) | (A & 0x01)); /* m342.mapper #23 */
					break;
				default:
					vrc_2b_hi = (((A >> 2) & 0x01) | (A & 0x01)); /* m342.mapper #25 */
					vrc_2b_low = (((A >> 3) & 0x01) | ((A >> 1) & 0x01)); /* m342.mapper #25 */
					break;
				}
				vrc_2b_addr = (vrc_2b_hi << 1) | vrc_2b_low;
			}

			switch (((A >> 10) & 0x1C) | vrc_2b_addr) {
			case 0: /* $8000-$8003, PRG0 */
			case 1:
			case 2:
			case 3:
				m342.prg_bank_a = (m342.prg_bank_a & 0xE0) | (V & 0x1F);
				break;
			case 4: /* $9000-$9001, m342.mirroring */
			case 5:
				/* VRC2 - using games are usually well - behaved and only write 0 or 1 to this register, */
				/* but Wai Wai World in one instance writes $FF instead */
				if (V != 0xFF) {
					m342.mirroring = V & 0x03;
				}
				break;
			case 6: /* $9002-$9004, PRG swap */
			case 7:
				m342.prg_mode = (m342.prg_mode & 0xFE) | ((V >> 1) & 0x01);
				break;
			case 8: /* $A000-$A003, PRG1 */
			case 9:
			case 10:
			case 11:
				m342.prg_bank_b = (m342.prg_bank_b & 0xE0) | (V & 0x1F);
				break;
			case 12: /* $B000, CHR0 low */
				m342.chr_bank_a = (m342.chr_bank_a & 0xF0) | (V & 0x0F);
				break;
			case 13: /* $B001, CHR0 hi */
				m342.chr_bank_a = (m342.chr_bank_a & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 14: /* $B002, CHR1 low */
				m342.chr_bank_b = (m342.chr_bank_b & 0xF0) | (V & 0x0F);
				break;
			case 15: /* $B003, CHR1 hi */
				m342.chr_bank_b = (m342.chr_bank_b & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 16: /* $C000, CHR2 low */
				m342.chr_bank_c = (m342.chr_bank_c & 0xF0) | (V & 0x0F);
				break;
			case 17: /* $C001, CHR2 hi */
				m342.chr_bank_c = (m342.chr_bank_c & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 18: /* $C002, CHR3 low */
				m342.chr_bank_d = (m342.chr_bank_d & 0xF0) | (V & 0x0F);
				break;
			case 19: /* $C003, CHR3 hi */
				m342.chr_bank_d = (m342.chr_bank_d & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 20: /* $D000, CHR4 low */
				m342.chr_bank_e = (m342.chr_bank_e & 0xF0) | (V & 0x0F);
				break;
			case 21: /* $D001, CHR4 hi */
				m342.chr_bank_e = (m342.chr_bank_e & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 22: /* $D002, CHR5 low */
				m342.chr_bank_f = (m342.chr_bank_f & 0xF0) | (V & 0x0F);
				break;
			case 23: /* $D003, CHR5 hi */
				m342.chr_bank_f = (m342.chr_bank_f & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 24: /* $E000, CHR6 low */
				m342.chr_bank_g = (m342.chr_bank_g & 0xF0) | (V & 0x0F);
				break;
			case 25: /* $E001, CHR6 hi */
				m342.chr_bank_g = (m342.chr_bank_g & 0x0F) | ((V & 0x0F) << 4);
				break;
			case 26: /* $E002, CHR7 low */
				m342.chr_bank_h = (m342.chr_bank_h & 0xF0) | (V & 0x0F);
				break;
			case 27: /* $E003, CHR7 hi */
				m342.chr_bank_h = (m342.chr_bank_h & 0x0F) | ((V & 0x0F) << 4);
				break;
			}
			if ((A & 0x7000) == 0x7000) {
				switch (vrc_2b_addr) {
				case 0: /* IRQ latch low */
					m342.vrc4.irq_latch = (m342.vrc4.irq_latch & 0xF0) | (V & 0x0F);
					break;
				case 1: /* IRQ latch hi */
					m342.vrc4.irq_latch = (m342.vrc4.irq_latch & 0x0F) | ((V & 0x0F) << 4);
					break;
				case 2: /* IRQ control */
					X6502_IRQEnd(FCEU_IQEXT); /* ack */
					m342.vrc4.irq_control = (m342.vrc4.irq_control & 0xF8) | (V & 0x07); /* mode, enabled, enabled after ack */
					if (m342.vrc4.irq_control & 0x02) { /* if E is set */
						m342.vrc4.irq_prescaler_counter = 0; /* reset prescaler */
						m342.vrc4.irq_prescaler = 0;
						m342.vrc4.irq_value = m342.vrc4.irq_latch; /* reload with latch */
					}
					break;
				case 3: /* IRQ ack */
					X6502_IRQEnd(FCEU_IQEXT);
					m342.vrc4.irq_control = (m342.vrc4.irq_control & 0xFD) | (m342.vrc4.irq_control & 0x01) << 1;
					break;
				}
			}
		}

		/* Mapper #69 - Sunsoft FME-7 */
		if (m342.mapper == 25) {
			uint8_t reg = (A & 0x6000) >> 13;
			if (reg == 0) {
				m342.mapper69.internal = (m342.mapper69.internal & 0xF0) | (V & 0x0F);
			}
			if (reg == 1) {
				switch (m342.mapper69.internal & 0x0F) {
				case 0: /* CHR0 */
					m342.chr_bank_a = V;
					break;
				case 1: /* CHR1 */
					m342.chr_bank_b = V;
					break;
				case 2: /* CHR2 */
					m342.chr_bank_c = V;
					break;
				case 3: /* CHR3 */
					m342.chr_bank_d = V;
					break;
				case 4: /* CHR4 */
					m342.chr_bank_e = V;
					break;
				case 5: /* CHR5 */
					m342.chr_bank_f = V;
					break;
				case 6: /* CHR6 */
					m342.chr_bank_g = V;
					break;
				case 7: /* CHR7 */
					m342.chr_bank_h = V;
					break;
				case 8: /* PRG0 */
					m342.sram_enabled = (V >> 7) & 0x01;
					m342.map_rom_on_6000 = ((V >> 6) & 0x01) ^ 0x01;
					m342.prg_bank_6000 = V & 0x3F;
					break;
				case 9: /* PRG1 */
					m342.prg_bank_a = (m342.prg_bank_a & 0xC0) | (V & 0x3F);
					break;
				case 10: /* PRG2 */
					m342.prg_bank_b = (m342.prg_bank_b & 0xC0) | (V & 0x3F);
					break;
				case 11: /* PRG3 */
					m342.prg_bank_c = (m342.prg_bank_c & 0xC0) | (V & 0x3F);
					break;
				case 12: /* m342.mirroring */
					m342.mirroring = V & 0x03;
					break;
				case 13:
					X6502_IRQEnd(FCEU_IQEXT); /* ack */
					m342.mapper69.counter_enabled = V >> 7;
					m342.mapper69.irq_enabled = V & 0x01;
					break;
				case 14: /* IRQ low */
					m342.mapper69.irq_value = (m342.mapper69.irq_value & 0xFF00) | V;
					break;
				case 15: /* IRQ high */
					m342.mapper69.irq_value = (m342.mapper69.irq_value & 0x00FF) | (V << 8);
					break;
				}
			}
		}

		/* Mapper #32 - Irem's G-101 */
		if (m342.mapper == 26) {
			switch (A & 0xF000) {
			case 0x8000:
				m342.prg_bank_a = (m342.prg_bank_a & 0xC0) | (V & 0x3F);
				break;
			case 0x9000:
				m342.prg_mode = (m342.prg_mode & 0x06) | ((V >> 1) & 0x01);
				m342.mirroring = V & 0x01;
				break;
			case 0xA000:
				m342.prg_bank_b = (m342.prg_bank_b & 0xC0) | (V & 0x3F);
				break;
			case 0xB000:
				switch (A & 0x07) {
				case 0:
					m342.chr_bank_a = V;
					break;
				case 1:
					m342.chr_bank_b = V;
					break;
				case 2:
					m342.chr_bank_c = V;
					break;
				case 3:
					m342.chr_bank_d = V;
					break;
				case 4:
					m342.chr_bank_e = V;
					break;
				case 5:
					m342.chr_bank_f = V;
					break;
				case 6:
					m342.chr_bank_g = V;
					break;
				case 7:
					m342.chr_bank_h = V;
					break;
				}
				break;
			}
		}

		/* Mapper #36 is assigned to TXC's PCB 01-22000-400 */
		if (m342.mapper == 29) {
			if ((A & 0x7FFE) == 0x7FFE) {
				m342.prg_bank_a = (m342.prg_bank_a & 0xC3) | ((V & 0xF0) >> 2);
				m342.chr_bank_a = (m342.chr_bank_a & 0x87) | ((V & 0x0F) << 3);
			}
		}

		/* Mapper #70 */
		if (m342.mapper == 30) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xE1) | ((V & 0xF0) >> 3);
			m342.chr_bank_a = (m342.chr_bank_a & 0x87) | ((V & 0x0F) << 3);
		}

		/* Mapper #75 - VRC1 */
		if (m342.mapper == 34) {
			switch (A & 0xF000) {
			case 0x8000:
				m342.prg_bank_a = (m342.prg_bank_a & 0xF0) | (V & 0x0F);
				break;
			case 0x9000:
				m342.mirroring = V & 1;
				m342.chr_bank_a = (m342.chr_bank_a & 0xBF) | ((V & 0x02) << 5);
				m342.chr_bank_e = (m342.chr_bank_a & 0xBF) | ((V & 0x04) << 4);
				break;
			case 0xA000: /* $A000-$AFFF */
				m342.prg_bank_b = (m342.prg_bank_b & 0xF0) | (V & 0x0F);
				break;
			case 0xC000:
				m342.prg_bank_c = (m342.prg_bank_c & 0xF0) | (V & 0x0F);
				break;
			case 0xE000:
				m342.chr_bank_a = (m342.chr_bank_a & 0xC3) | ((V & 0x0F) << 2);
				break;
			case 0xF000:
				m342.chr_bank_e = (m342.chr_bank_e & 0xC3) | ((V & 0x0F) << 2);
				break;
			}
		}

		/* Mapper #83 - Cony/Yoko */
		/* TODO: Check for m342.flags 4 needed? */
		if (m342.mapper == 35) {
			switch (A & 0x8300) {
			case 0x8000:
				m342.prg_bank_a = (m342.prg_bank_a & 0xE1) | ((V & 0x0F) << 1);
				break;
			case 0x8100: /* $81xx */
				m342.mirroring = V & 0x03;
				m342.prg_mode = (m342.prg_mode & 0x03) | ((V >> 2) & 0x04);
				m342.map_rom_on_6000 = (V & 0x20) >> 5;
				m342.mapper83.irq_enabled_latch = (V & 0x80) >> 7;
				break;
			case 0x8200:
				if (!(A & 0x01)) {
					X6502_IRQEnd(FCEU_IQEXT);
					m342.mapper83.irq_counter = (m342.mapper83.irq_counter & 0xFF00) | V;
				} else {
					m342.mapper83.irq_enabled = m342.mapper83.irq_enabled_latch;
					m342.mapper83.irq_counter = (m342.mapper83.irq_counter & 0x00FF) | (V << 8);
				}
				break;
			case 0x8300:
				if (!(A & 0x10)) {
					switch (A & 0x03) {
					case 0:
						m342.prg_bank_a = V;
						break;
					case 1:
						m342.prg_bank_b = V;
						break;
					case 2:
						m342.prg_bank_b = V;
						break;
					case 3:
						/* TODO: Verify this */
						m342.prg_bank_6000 = V;
						break;
					}
				} else {
					if (!(m342.flags & 0x04)) {
						switch (A & 0x07) {
						case 0:
							m342.chr_bank_a = V;
							break;
						case 1:
							m342.chr_bank_b = V;
							break;
						case 2:
							m342.chr_bank_c = V;
							break;
						case 3:
							m342.chr_bank_d = V;
							break;
						case 4:
							m342.chr_bank_e = V;
							break;
						case 5:
							m342.chr_bank_f = V;
							break;
						case 6:
							m342.chr_bank_g = V;
							break;
						case 7:
							m342.chr_bank_h = V;
							break;
						}
					} else {
						switch (A & 0x07) {
						/* TODO: verify CHR mask */
						case 0:
							m342.chr_bank_a = (m342.chr_bank_a & 0x01) | (V << 1);
							break;
						case 1:
							m342.chr_bank_c = (m342.chr_bank_c & 0x01) | (V << 1);
							break;
						case 6:
							m342.chr_bank_e = (m342.chr_bank_e & 0x01) | (V << 1);
							break;
						case 7:
							m342.chr_bank_g = (m342.chr_bank_g & 0x01) | (V << 1);
							break;
						}
					}
				}
				break;
			}
		}

		/* Mapper #67 - Sunsoft-3 */
		if (m342.mapper == 36) {
			if (A & 0x800) {
				switch (A & 0xF800) {
				case 0x8800:
					m342.chr_bank_a = (m342.chr_bank_a & 0x81) | ((V & 0x3F) << 1);
					break;
				case 0x9800:
					m342.chr_bank_c = (m342.chr_bank_c & 0x81) | ((V & 0x3F) << 1);
					break;
				case 0xA800:
					m342.chr_bank_e = (m342.chr_bank_e & 0x81) | ((V & 0x3F) << 1);
					break;
				case 0xB800:
					m342.chr_bank_g = (m342.chr_bank_g & 0x81) | ((V & 0x3F) << 1);
					break;
				case 0xC800:
					m342.mapper67.irq_latch = ~m342.mapper67.irq_latch;
					if (m342.mapper67.irq_latch) {
						m342.mapper67.irq_counter = (m342.mapper67.irq_counter & 0x00FF) | (V << 8);
					} else {
						m342.mapper67.irq_counter = (m342.mapper67.irq_counter & 0xFF00) | V;
					}
					break;
				case 0xD800:
					m342.mapper67.irq_latch = 0;
					m342.mapper67.irq_enabled = (V & 0x10) >> 4;
					break;
				case 0xE800:
					m342.mirroring = V & 0x03;
					break;
				case 0xF800:
					m342.prg_bank_a = (m342.prg_bank_a & 0xE1) | ((V & 0x0F) << 1);
					break;
				}
			} else {
				/* Interrupt Acknowledge ($8000) */
				X6502_IRQEnd(FCEU_IQEXT);
			}
		}

		/* Mapper #89 - Sunsoft-2 chip on the Sunsoft-3 board */
		if (m342.mapper == 37) {
			m342.prg_bank_a = (m342.prg_bank_a & 0xF1) | ((V & 0x70) >> 3);
			m342.chr_bank_a = (m342.chr_bank_a & 0x87) | ((V & 0x80) >> 1) | ((V & 0x07) << 3);
			m342.mirroring = 2 | ((V & 0x08) >> 3);
		}
	}

	Sync();
}

static DECLFR(Read47) {
	if ((m342.mapper == 0) && (A >= 0x5000) && (A < 0x6000))
		return 0;

	/* Mapper #163 */
	if (m342.mapper == 6) {
		if ((A & 0x7700) == 0x5100) {
			return m342.mapper163.r2 | m342.mapper163.r0 | m342.mapper163.r1 | ~m342.mapper163.r3;
		}
		if ((A & 0x7700) == 0x5500) {
			return (m342.mapper163.r5 & 1) ? m342.mapper163.r2 : m342.mapper163.r1;
		}
	}

	/* Mapper #90 - JY */
	if (m342.mapper == 13) {
		if ((A == 0x5800)) {
			return (m342.mapper90.mul1 * m342.mapper90.mul2) & 0xFF;
		}
		if ((A == 0x5801)) {
			return ((m342.mapper90.mul1 * m342.mapper90.mul2) >> 8) & 0xFF;
		}
	}

	/* MMC5 */
	if (m342.mapper == 15) {
		if (A == 0x5204) {
			uint8_t ppuon = (PPU[1] & 0x18);
			uint8_t ret = (m342.mmc5.irq_out << 7) | (!ppuon || ((scanline + 1) >= 241) ? 0 : 0x40);
			X6502_IRQEnd(FCEU_IQEXT);
			m342.mmc5.irq_out = 0;
			return ret;
		}
	}

	/* Mapper #36 is assigned to TXC's PCB 01-22000-400 */
	if ((m342.mapper == 29) && ((A & 0xE100) == 0x4100)) {
		return (m342.prg_bank_a & 0x0C) << 2;
	}

	/* Mapper #83 - Cony/Yoko */
	if ((m342.mapper == 35) && ((A & 0x7000) == 0x5000)) {
		return (m342.flags & 3);
	}

	if (m342.sram_enabled && !m342.map_rom_on_6000 && (A >= 0x6000) && (A < 0x8000)) {
		return CartBR(A); /* SRAM */
	}

	if (m342.map_rom_on_6000 && (A >= 0x6000) && (A < 0x8000)) {
		return CartBR(A); /* PRG */
	}

	return cpu.openbus; /* Open bus */
}

static void HBIRQHook(void) {
	/* for MMC3 and MMC3-based */
	if (m342.mmc3.irq_reload || !m342.mmc3.irq_counter) {
		m342.mmc3.irq_counter = m342.mmc3.irq_latch;
		m342.mmc3.irq_reload = 0;
	} else {
		m342.mmc3.irq_counter--;
	}
	if (!m342.mmc3.irq_counter && m342.mmc3.irq_enabled) {
		X6502_IRQBegin(FCEU_IQEXT);
	}

	/* for MMC5 */
	if (m342.mmc5.irq_line == (scanline + 1)) {
		if (m342.mmc5.irq_enabled) {
			X6502_IRQBegin(FCEU_IQEXT);
			m342.mmc5.irq_out = 1;
		}
	}

	/* for m342.mapper #163 */
	if (scanline == 239) {
		m342.mapper163.latch = 0;
		SyncCHR();
	} else if (scanline == 127) {
		m342.mapper163.latch = 1;
		SyncCHR();
	}
}

static void CPUIRQHook(int a) {
	while (a--) {
		/* Mapper #23 - VRC4 */
		if (m342.vrc4.irq_control & 0x02) {
			m342.vrc4.irq_prescaler++; /* count prescaler */
			if ((!(m342.vrc4.irq_prescaler_counter & 0x02) && (m342.vrc4.irq_prescaler == 114)) ||
				((m342.vrc4.irq_prescaler_counter & 0x02) && (m342.vrc4.irq_prescaler == 113))) {
				m342.vrc4.irq_prescaler = 0;
				m342.vrc4.irq_prescaler_counter++;
				if (m342.vrc4.irq_prescaler_counter == 3) {
					m342.vrc4.irq_prescaler_counter = 0;
				}
				m342.vrc4.irq_value++;
				if (m342.vrc4.irq_value == 0) { /* if (carry) */
					X6502_IRQBegin(FCEU_IQEXT);
					m342.vrc4.irq_value = m342.vrc4.irq_latch;
				}
			}
		}

		/* Mapper #73 - VRC3 */
		if (m342.vrc3.irq_control & 0x02) {
			if (m342.vrc3.irq_control & 0x04) { /* 8-bit mode */
				m342.vrc3.irq_value = (m342.vrc3.irq_value & 0xFF00) | ((m342.vrc3.irq_value + 1) & 0xFF);
				if ((m342.vrc3.irq_value & 0xFF) == 0) {
					X6502_IRQBegin(FCEU_IQEXT);
					m342.vrc3.irq_value = (m342.vrc3.irq_value & 0xFF00) | (m342.vrc3.irq_latch & 0xFF);
				}
			} else { /* 16-bit */
				m342.vrc3.irq_value += 1;
				if (m342.vrc3.irq_value == 0) {
					X6502_IRQBegin(FCEU_IQEXT);
					m342.vrc3.irq_value = m342.vrc3.irq_latch;
				}
			}
		}

		/* Mapper #69 - Sunsoft FME-7 */
		if (m342.mapper69.counter_enabled) {
			m342.mapper69.irq_value--;
			if (m342.mapper69.irq_value == 0xFFFF) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}

		/* Mapper #18 */
		if (m342.mapper18.irq_control & 0x01) {
			uint8_t carry = (m342.mapper18.irq_value & 0x0F) - 1;
			m342.mapper18.irq_value = (m342.mapper18.irq_value & 0xFFF0) | (carry & 0x0F);
			carry = (carry >> 4) & 0x01;
			if (!(m342.mapper18.irq_control & 0x08)) {
				carry = ((m342.mapper18.irq_value >> 4) & 0x0F) - carry;
				m342.mapper18.irq_value = (m342.mapper18.irq_value & 0xFF0F) | ((carry & 0x0F) << 4);
				carry = (carry >> 4) & 0x01;
			}
			if (!(m342.mapper18.irq_control & 0x0C)) {
				carry = ((m342.mapper18.irq_value >> 8) & 0x0F) - carry;
				m342.mapper18.irq_value = (m342.mapper18.irq_value & 0xF0FF) | ((carry & 0x0F) << 8);
				carry = (carry >> 4) & 0x01;
			}
			if (!(m342.mapper18.irq_control & 0x0E)) {
				carry = ((m342.mapper18.irq_value >> 12) & 0x0F) - carry;
				m342.mapper18.irq_value = (m342.mapper18.irq_value & 0x0FFF) | ((carry & 0x0F) << 12);
				carry = (carry >> 4) & 0x01;
			}
			if (carry) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}

		/* Mapper #65 - Irem's H3001 */
		if (m342.mapper65.irq_enabled) {
			if (m342.mapper65.irq_value != 0) {
				m342.mapper65.irq_value--;
				if (!m342.mapper65.irq_value) {
					X6502_IRQBegin(FCEU_IQEXT);
				}
			}
		}

		/* Mapper #42 */
		if (m342.mapper42.irq_enabled) {
			m342.mapper42.irq_value++;
			if (m342.mapper42.irq_value >> 15) {
				m342.mapper42.irq_value = 0;
			}
			if (((m342.mapper42.irq_value >> 13) & 0x03) == 0x03) {
				X6502_IRQBegin(FCEU_IQEXT);
			} else {
				X6502_IRQEnd(FCEU_IQEXT);
			}
		}

		/* Mapper #83 - Cony/Yoko */
		if (m342.mapper83.irq_enabled) {
			if (m342.mapper83.irq_counter == 0) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
			m342.mapper83.irq_counter--;
		}

		/* Mapper #67 - Sunsoft-3 */
		if (m342.mapper67.irq_enabled) {
			m342.mapper67.irq_counter--;
			if (m342.mapper67.irq_counter == 0xFFFF) {
				X6502_IRQBegin(FCEU_IQEXT); /* fire IRQ */
				m342.mapper67.irq_enabled = 0; /* disable IRQ */
			}
		}
	}
}

static void PPUHook(uint32_t A) {
	/* For TxROM */
	if ((m342.mapper == 20) && (m342.flags & 0x01)) {
		setmirror(MI_0 + (m342.TKSMIR[(A & 0x1FFF) >> 10] >> 7));
	}

	/* Mapper #9 and #10 - MMC2 and MMC4 */
	if (m342.mapper == 17) {
		if ((A >> 4) == 0xFD) {
			m342.mmc2and4.latch0 = 0;
			SyncCHR();
		}
		if ((A >> 4) == 0xFE) {
			m342.mmc2and4.latch0 = 1;
			SyncCHR();
		}
		if ((A >> 4) == 0x1FD) {
			m342.mmc2and4.latch1 = 0;
			SyncCHR();
		}
		if ((A >> 4) == 0x1FE) {
			m342.mmc2and4.latch1 = 1;
			SyncCHR();
		}
	}
}

static void Reset(void) {
	memset(&m342, 0, sizeof(m342));

	m342.prg_mask = 0xF8;
	m342.prg_bank_a = 0;
	m342.prg_bank_b = 1;
	m342.prg_bank_c = ~1;
	m342.prg_bank_d = ~0;

	m342.chr_bank_a = 0;
	m342.chr_bank_b = 1;
	m342.chr_bank_c = 2;
	m342.chr_bank_d = 3;
	m342.chr_bank_e = 4;
	m342.chr_bank_f = 5;
	m342.chr_bank_g = 6;
	m342.chr_bank_h = 7;

	Sync();
}

static void Power(void) {
	FCEU_CheatAddRAM(32, 0x6000, WRAM);
	SetReadHandler(0x4020, 0x7FFF, Read47);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0xFFFF, Write4F);
	Reset();
}

static void Close(void) {
	if (SAVE_FLASH) {
		FCEU_gfree(SAVE_FLASH);
	}

	if (CFI) {
		FCEU_gfree(CFI);
	}

	SAVE_FLASH = CFI = NULL;
}

static void StateRestore(int version) {
	Sync();
	m342.mmc1.lreset = 0;
}

#define ExState(var, varname)   AddExState(&var, sizeof(var), 0, varname)
#define ExStateLE(var, varname) AddExState(&var, sizeof(var) | FCEUSTATE_RLSB, 0, varname)

void Mapper342_Init(CartInfo *info) {
	int i;

	info->Power = Power;
	info->Reset = Reset;
	info->Close = Close;
	GameStateRestore = StateRestore;
	GameHBIRQHook = HBIRQHook;
	MapIRQHook = CPUIRQHook;
	PPU_hook = PPUHook;

	CHR_SIZE = info->CHRRamSize ? info->CHRRamSize : (512 * 1024) /* non-iNES2 or UNIF */;
	WRAMSIZE = (info->PRGRamSize + info->PRGRamSaveSize) ? (info->PRGRamSize + info->PRGRamSaveSize) : (32 * 1024);

	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
		SetupCartPRGMapping(WRAM_CHIP, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (info->battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}

	if (info->battery) {
		SAVE_FLASH = (uint8_t *)FCEU_malloc(SAVE_FLASH_SIZE);
		SetupCartPRGMapping(FLASH_CHIP, SAVE_FLASH, SAVE_FLASH_SIZE, 1);
		info->SaveGame[1] = SAVE_FLASH;
		info->SaveGameLen[1] = SAVE_FLASH_SIZE;
	}

	CFI = (uint8_t *)FCEU_malloc(sizeof(cfi_data) * 2);
	for (i = 0; i < (int)sizeof(cfi_data); i++) {
		CFI[i * 2] = CFI[i * 2 + 1] = cfi_data[i];
	}
	SetupCartPRGMapping(CFI_CHIP, CFI, sizeof(cfi_data) * 2, 0);

	switch (info->PRGCRC32) {
	/* Earlier version of roms using VRC24 were using incorrect m342.flags
	 * which are now incompatible with later updates of Coolgirl firmware causing
	 * graphics to be broken. This enables compatibily code to handle early roms while
	 * keeping the latest and updated VRC24 handling intact for newer roms when they become available.
	 */
	case 0xCF0FE3F3: /* 0b89251f6d63d49586ee3bbe41914ab7.unif */
	case 0x62BEFE75: /* 320 ¿úÓ äÑ¡ñ¿.unf */
	case 0xBF617289: /* d61e011f86b13bed9a22d3c56326e689.unif */
	case 0x3A70CB07: /* (MMK-02A-01) Gradius 10-in-1.nes */
	case 0x5352A128: /* (MMK-02D-00) Coolgirl 11-in-1.nes */
	case 0x861F89A0: /* (MMK-02E-00) Coolgirl 7-in-1.nes */
	case 0xA25CD951: /* (MMK-02F-00) Shooting Game 16-in-1.nes */
	case 0xE368F1F6: /* (MMK-033-00) Game 150-in-1.nes */
	case 0x49EE3B04: /* (YG-6014) Super Captain Tsubasa 2 Hack 6-in-1.nes (not using m342.mapper code 24 but whatever) */
	case 0x7B85868B: /* (Yhc-4006-00) Super Konami 80-in-1.nes */
	case 0x2F0D22CD: /* (Yhc-BS-8165-01) Super Plane Game 11-in-1.nes */
	case 0x242B9218: /* (Yhc-CK-124-07) Super Konami 3-in-1.nes */
	case 0x94D8A822: /* MMK-034-01 */
	case 0xFF50D601: /* MMK-02A-03 */
	case 0xD12DF3B3: /* MMK-02A-04 */
	case 0xACB76337: /* 101-in-1 */
	case 0x741EBB17: /* 3-in-1 */
	case 0xAF4D32E2: /* 41-in-1 */
	case 0x2986A65A: /* Super Game 5-in-1 */
	case 0x25894753: /* MMK-02C-00 */
	case 0x24A086E6: /* 524-in-1 */
	case 0x2EE18D15: /* 4-in-1 (Contra by Rika) (Unl) */
		vrc24_compatibility = 1;
		FCEU_printf(" Mapper in compatibility mode.\n");
		break;
	default:
		vrc24_compatibility = 0;
		break;
	}

	ExState(m342.sram_enabled, "SREN");
	ExState(m342.sram_page, "SRPG");
	ExState(m342.can_write_chr, "SRWR");
	ExState(m342.map_rom_on_6000, "MR6K");
	ExState(m342.flags, "FLGS");
	ExState(m342.mapper, "MPPR");
	ExState(m342.can_write_flash, "FLWR");
	ExState(m342.mirroring, "MIRR");
	ExState(m342.fourscreen, "4SCR");
	ExState(m342.lockout, "LOCK");

	ExState(m342.prg_base, "PBAS");
	ExStateLE(m342.prg_mask, "PMSK");
	ExState(m342.prg_mode, "PMOD");
	ExState(m342.prg_bank_6000, "P6BN");
	ExState(m342.prg_bank_a, "PABN");
	ExState(m342.prg_bank_b, "PBBN");
	ExState(m342.prg_bank_c, "PCBN");
	ExState(m342.prg_bank_d, "PDBN");
	ExStateLE(m342.prg_bank_6000_mapped, "P6BM");
	ExStateLE(m342.prg_bank_a_mapped, "PABM");
	ExStateLE(m342.prg_bank_b_mapped, "PBBM");
	ExStateLE(m342.prg_bank_c_mapped, "PCBM");
	ExStateLE(m342.prg_bank_d_mapped, "PDBM");

	ExStateLE(m342.chr_mask, "CMSK");
	ExState(m342.chr_mode, "CMOD");
	ExStateLE(m342.chr_bank_a, "CABN");
	ExStateLE(m342.chr_bank_b, "CBBN");
	ExStateLE(m342.chr_bank_c, "CCBN");
	ExStateLE(m342.chr_bank_d, "CDBN");
	ExStateLE(m342.chr_bank_e, "CEBN");
	ExStateLE(m342.chr_bank_f, "CFBN");
	ExStateLE(m342.chr_bank_g, "CGBN");
	ExStateLE(m342.chr_bank_h, "CHBN");

	ExState(m342.mmc2and4.latch0, "PPU0");
	ExState(m342.mmc2and4.latch1, "PPU1");

	ExState(m342.mmc1.lreset, "LRST");
	ExState(m342.mmc1.load_register, "MC1R");

	ExState(m342.mmc3.internal, "MC3I");

	ExState(m342.mapper69.internal, "M69I");
	ExState(m342.mapper112.internal, "112I");
	ExState(m342.mapper163.latch, "163L");
	ExState(m342.mapper163.r0, "1630");
	ExState(m342.mapper163.r1, "1631");
	ExState(m342.mapper163.r2, "1632");
	ExState(m342.mapper163.r3, "1633");
	ExState(m342.mapper163.r4, "1634");
	ExState(m342.mapper163.r5, "1635");

	ExState(m342.mapper90.mul1, "MUL1");
	ExState(m342.mapper90.mul2, "MUL2");

	ExState(m342.mmc3.irq_enabled, "M4IE");
	ExState(m342.mmc3.irq_latch, "M4IL");
	ExState(m342.mmc3.irq_counter, "M4IC");
	ExState(m342.mmc3.irq_reload, "M4IR");

	ExState(m342.mmc5.irq_enabled, "M5IE");
	ExState(m342.mmc5.irq_line, "M5IL");
	ExState(m342.mmc5.irq_out, "M5IO");

	ExState(m342.mapper18.irq_value, "18IV");
	ExState(m342.mapper18.irq_control, "18IC");
	ExState(m342.mapper18.irq_latch, "18IL");

	ExState(m342.mapper65.irq_enabled, "65IE");
	ExStateLE(m342.mapper65.irq_value, "65IV");
	ExStateLE(m342.mapper65.irq_latch, "65IL");

	ExState(m342.mapper69.irq_enabled, "69IE");
	ExState(m342.mapper69.counter_enabled, "69CE");
	ExStateLE(m342.mapper69.irq_value, "69IV");

	ExState(m342.vrc4.irq_value, "V4IV");
	ExState(m342.vrc4.irq_control, "V4IC");
	ExState(m342.vrc4.irq_latch, "V4IL");
	ExState(m342.vrc4.irq_prescaler, "V4PP");
	ExState(m342.vrc4.irq_prescaler_counter, "V4PC");

	ExStateLE(m342.vrc3.irq_value, "V3IV");
	ExState(m342.vrc3.irq_control, "V3IC");
	ExStateLE(m342.vrc3.irq_latch, "V3IL");

	ExState(m342.mapper42.irq_enabled, "42IE");
	ExStateLE(m342.mapper42.irq_value, "42IV");

	ExState(m342.mapper83.irq_enabled_latch, "M83L");
	ExState(m342.mapper83.irq_enabled, "M83I");
	ExStateLE(m342.mapper83.irq_counter, "M83C");

	ExState(m342.mapper90.xor, "90XR");

	ExState(m342.mapper67.irq_enabled, "67IE");
	ExState(m342.mapper67.irq_latch, "67IL");
	ExStateLE(m342.mapper67.irq_counter, "67IC");

	ExState(m342.flash_state, "FLST");
	ExStateLE(m342.flash_buffer_a[0], "FLB0");
	ExStateLE(m342.flash_buffer_a[1], "FLB1");
	ExStateLE(m342.flash_buffer_a[2], "FLB2");
	ExStateLE(m342.flash_buffer_a[3], "FLB3");
	ExStateLE(m342.flash_buffer_a[4], "FLB4");
	ExStateLE(m342.flash_buffer_a[5], "FLB5");
	ExStateLE(m342.flash_buffer_a[6], "FLB6");
	ExStateLE(m342.flash_buffer_a[7], "FLB7");
	ExStateLE(m342.flash_buffer_a[8], "FLB8");
	ExStateLE(m342.flash_buffer_a[9], "FLB9");
	ExState(m342.flash_buffer_v, "FLBV");

	ExState(m342.cfi_mode, "CFIM");
}
