/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

/*
 * NES 2.0 Mapper 401 denotes the KC885 multicart circuit board, used for the Super 19-in-1 (VIP19) multicart. It is basically mapper 45 with the higher address lines connected weirdly.
 * 
 * PRG A13-A17: from $6000 #1 bits 0-4, same as mapper 45
 * PRG A18: either from $6000 #2 bit 5 or from $6000 #1 bit 6, depending on solder pad setting
 * PRG A19: either from $6000 #2 bit 6 or from $6000 #1 bit 5, depending on solder pad setting
 * PRG /CE: $6000 #1 bit 7, if solder pad connected
 * The menu code tries the two ways of selecting PRG A18 and A19 and whether $6000 #1 bit 7 disables PRG-ROM, and selects one of eight different menus based on what it finds. Other multicarts using mapper 45's chipset do the same thing; KC885 is unique in that the standard mapper 45 way of connecting PRG-ROM will not work at all.
 * 
 * Super 19-in-1 (VIP19) (crc 0x2F497313)
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
	uint8_t cmd;
} m401;

static uint8_t dipsw = 0;

static SFORMAT StateRegs[] = {
	{ m401.reg, 4, "EXPR" },
	{ &m401.cmd, 1, "CMD0" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if ((dipsw & 0x01)  && (m401.reg[1] & 0x80)) {
		unsetcpu8(A);
	} else {
		uint16_t mask = (~m401.reg[3] & 0x1F);
		uint16_t base = (m401.reg[1] & 0x1F) | (m401.reg[2] & 0x80) |
		    ((dipsw & 0x02) ? (m401.reg[2] & 0x20) : ((m401.reg[1] >> 1) & 0x20)) |
		    ((dipsw & 0x04) ? (m401.reg[2] & 0x40) : ((m401.reg[1] << 1) & 0x40));

		setprg8(A, base | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (0xFF >> (~m401.reg[2] & 0xF));
	uint16_t base = (m401.reg[0] | ((m401.reg[2] << 4) & 0xF00));

	setchr1(A, base | (V & mask));
}

static DECLFW(WriteReg) {
	/* FCEU_printf("Wr A:%04x V:%02x index:%d\n", A, V, m401.cmd); */
	if (!(m401.reg[3] & 0x40)) {
		m401.reg[m401.cmd] = V;
		m401.cmd = (m401.cmd + 1) & 0x03;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
	CartBW(A, V);
}

static void Reset(void) {
	memset(&m401, 0, sizeof(m401));
	m401.reg[2] = 0x0F;
	dipsw = (dipsw + 1) & 0x07;
	FCEU_printf("dipsw = %d\n", dipsw);
	MMC3_Reset();
}

static void Power(void) {
	memset(&m401, 0, sizeof(m401));
	m401.reg[2] = 0x0F;
	dipsw = 7;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper401_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
