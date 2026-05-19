/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
 *  Copyright (C) 2023-2025-2026 negativeExponent
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
 */

/* NES 2.0 Mapper 412 is a variant of mapper 45 where the
 * ASIC's PRG A21/CHR A20 output (set by bit 6 of the third write to $6000)
 * selects between regularly-banked CHR-ROM (=0) and 8 KiB of unbanked CHR-RAM (=1).
 * It is used solely for the Super 8-in-1 - 98格鬥天王＋熱血 (JY-302) multicart.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
} m412;

static uint8_t dipsw;

static SFORMAT StateReg[] = {
	{ m412.reg, 4, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x3F & ~(((m412.reg[1] << 4) & 0x20) | (m412.reg[1] & 0x10));
	uint16_t base = ((m412.reg[1] << 3) & 0x20) | ((m412.reg[1] >> 2) & 0x10);

	if (m412.reg[2] & 0x02) {
		uint16_t bank = m412.reg[2] >> 3;

		if (m412.reg[2] & 0x04) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else {
		setprg8(A, (base & ~mask) | (V & mask));
	}

	mmc3.wram = 0x80;
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m412.reg[1] & 0x20) ? 0x7F : 0xFF;
	uint16_t base = ((m412.reg[1] << 5) & 0x100) | (m412.reg[1] & 0x80);

	if (m412.reg[2] & 0x02) {
		setchr8(m412.reg[0] >> 2);
	} else {
		setchr1(A, (base & ~mask) | (V & mask));
	}
}

static DECLFR(ReadDIP) {
	return dipsw;
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		CartBW(A, V);
		if (!(m412.reg[1] & 0x01)) {
			m412.reg[A & 0x03] = V;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		}
	}
}

static void Reset(void) {
	memset(&m412, 0, sizeof(m412));
	dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m412, 0, sizeof(m412));
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x5000, 0x6FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper412_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateReg, ~0, 0, NULL);
}
