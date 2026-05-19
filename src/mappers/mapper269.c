/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 269
 *   Games Xplosion 121-in-1
 *   15000-in-1
 *   18000-in-1
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
	uint8_t cmd;
} m269;

static void SetCHRBank(uint16_t A, uint16_t V) {
	uint16_t mask = 0xFF >> (~m269.reg[2] & 0xF);
	uint16_t base = ((m269.reg[3] << 6) & 0x1000) | ((m269.reg[2] << 4) & 0xF00) | m269.reg[0];

	setchr1(A, (base & ~mask) | (V & mask));
}

static void SetPRGBank(uint16_t A, uint16_t V) {
	uint16_t mask = ~m269.reg[3] & 0x3F;
	uint16_t base = ((m269.reg[3] << 2) & 0x100) | m269.reg[1];

	setprg8(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	if (!(m269.reg[3] & 0x80)) {
		m269.reg[m269.cmd] = V;
		m269.cmd = (m269.cmd + 1) & 3;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m269, 0, sizeof(m269));
	m269.reg[2] = 0x0F;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m269, 0, sizeof(m269));
	m269.reg[2] = 0x0F;
	MMC3_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

static uint8_t unscrambleCHR(uint8_t data) {
	return (((data & 0x01) << 6)
		| ((data & 0x02) << 3)
		| ((data & 0x04) << 0)
		| ((data & 0x08) >> 3)
		| ((data & 0x10) >> 3)
		| ((data & 0x20) >> 2)
		| ((data & 0x40) >> 1)
		| ((data & 0x80) << 0));
}

void Mapper269_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_cwrap = SetCHRBank;
	MMC3_pwrap = SetPRGBank;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(m269.reg, 4, 0, "EXPR");
	AddExState(&m269.cmd, 1, 0, "CMD0");

	if (ROM.chr.size == 0) {
		size_t i;
		if (CHRRAM) {
			free(CHRRAM);
		}
		CHRRAM = (uint8_t *)FCEU_malloc(PRGsize[0]);
		ROM.chr.data = CHRRAM;
		SetupCartCHRMapping(0, ROM.chr.data, PRGsize[0], 0);
		/* unscramble CHR data from PRG */
		for (i = 0; i < PRGsize[0]; i++) {
			ROM.chr.data[i] = unscrambleCHR(ROM.prg.data[i]);
		}
	}
}
