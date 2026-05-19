/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
 *
 * City Fighter IV sith Sound VRC4 hacked
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t reg;
} m266;

static writefunc writepcm4011;

static SFORMAT StateRegs[] = {
	{ &m266.reg, 1, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg32(0x8000, m266.reg >> 2);
}

static DECLFW(WriteVRC24) {
	/* FCEU_printf("%04x %02x",A,V); */
	A = (A & 0x9FFF) | ((A << 1) & 0x4000) | ((A >> 1) & 0x2000);
	VRC24_Write(A, V);
}

static DECLFW(WriteMisc) {
	if (A & 0x800) {
		writepcm4011(0x4011, (V & 0x0F) << 3);
	} else {
		m266.reg = V & 0x0C;
		VRC24_SyncPRG();
	}
}

static void Power(void) {
	m266.reg = 0;
	VRC24_Power();
	writepcm4011 = GetWriteHandler(0x4011);
	SetWriteHandler(0x8000, 0xFFFF, WriteVRC24);
}

void Mapper266_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, FALSE, TRUE);
	info->Power = Power;
	VRC24_SyncPRG = SyncPRG;
	VRC24_WriteExtSelect = WriteMisc;
	AddExState(StateRegs, ~0, 0, NULL);
}
