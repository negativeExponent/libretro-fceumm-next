/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
	uint8_t cmd;
} m045;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m045.reg, 4, "EXPR" },
	{ &m045.cmd, 1, "CMD0" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	/* Some multicarts select between five different menus by connecting one of the higher address lines to PRG /CE.
	The menu code selects between menus by checking which of the higher address lines disables PRG-ROM when set. */
	if (dipsw &&
	    (dipsw == 1 && m045.reg[1] & 0x80 ||
	     dipsw == 2 && m045.reg[2] & 0x40 ||
	     dipsw == 3 && m045.reg[1] & 0x40 ||
	     dipsw == 4 && m045.reg[2] & 0x20)) {
		unsetcpu8(A);
	} else {
		uint32_t mask = ~m045.reg[3] & 0x3F;
		uint32_t base = ((m045.reg[2] << 2) & 0x300) | m045.reg[1];
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (ROM.chr.size || (iNESCart.CHRRamSize > 8192)) {
		uint32_t mask = 0xFF >> (~m045.reg[2] & 0x0F);
		uint32_t base = ((m045.reg[2] << 4) & 0xF00) | m045.reg[0];

		setchr1(A, (base & ~mask) | (V & mask));
	} else {
		/* assume chr-ram, 4-in-1 Yhc-Sxx-xx variants */
		setchr8(0);
	}
}

static DECLFR(ReadDIP) {
	/* Solder pad for 超强年度新卡 15-in-1 (New Years 15-in-1 cartridge )*/
	uint32_t addr = 1 << (dipsw + 4);

	if (A & (addr | (addr - 1))) {
		return 0x01;
	}
	return 0x00;
}

static DECLFW(WriteReg) {
	CartBW(A, V);
	if (!(m045.reg[3] & 0x40)) {
		m045.reg[m045.cmd] = V;
		m045.cmd = (m045.cmd + 1) & 0x03;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m045, 0, sizeof(m045));
	m045.reg[2] = 0x0F;
	dipsw++;
	dipsw &= 7;
	MMC3_Reset();
	FCEU_printf(" dipsw = %d\n", dipsw);
}

static void Power(void) {
	memset(&m045, 0, sizeof(m045));
	m045.reg[2] = 0x0F;
	dipsw = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetReadHandler(0x5000, 0x5FFF, ReadDIP);
}

void Mapper045_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
