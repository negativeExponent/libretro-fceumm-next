/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/* NES 2.0 Mapper 410 is a variant of mapper 45 where the
 * ASIC's PRG A21/CHR A20 output (set by bit 6 of the third write to $6000)
 * selects between regularly-banked CHR-ROM (=0) and 8 KiB of unbanked CHR-RAM (=1).
 * It is used solely for the Super 8-in-1 - 98格鬥天王＋熱血 (JY-302) multicart.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
	uint8_t cmd;
} m410;

static SFORMAT StateRegs[] = {
	{ m410.reg, 4, "REGS" },
	{ &m410.cmd, 1, "CMD0" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ~m410.reg[3] & 0x3F;
	uint16_t base = ((m410.reg[2] << 2) & 0x300) | m410.reg[1];

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (!(m410.reg[2] & 0x40)) {
		uint16_t mask = 0xFF >> (~m410.reg[2] & 0x0F);
		uint16_t base = ((m410.reg[2] << 4) & 0xF00) | m410.reg[0];

		setchr1(A, (base & ~mask) | (V & mask));
	} else {
		setchr8r(0x10, 0);
	}
}

static DECLFW(WriteReg) {
	if (!(m410.reg[3] & 0x40)) {
		m410.reg[m410.cmd] = V;
		m410.cmd = (m410.cmd + 1) & 0x03;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m410, 0, sizeof(m410));
	m410.reg[2] = 0x0F;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m410, 0, sizeof(m410));
	m410.reg[2] = 0x0F;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper410_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);

	CHRRAM = (uint8_t *)FCEU_gmalloc(8192);
	SetupCartCHRMapping(0x10, CHRRAM, 8192, 1);
	AddExState(CHRRAM, 8192, 0, "CRAM");
}
