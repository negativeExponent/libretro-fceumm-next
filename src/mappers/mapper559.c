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

/* NES 2.0 Mapper 559 - VRC4 clone
 * Subor 0102
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t nt[4];
	uint8_t prg;
} m559;

static SFORMAT StateRegs[] = {
	{ &m559.prg, 1, "PRGC" },
	{ m559.nt, 4, "NTBL" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (A == 0xC000) {
		setprg8(A, m559.prg & 0x1F);
	} else {
		setprg8(A, V & 0x1F);
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, V & 0x1FF);
}

static void SyncMirror(void) {
	setmirrorw(m559.nt[0] & 0x01, m559.nt[1] & 0x01, m559.nt[2] & 0x01, m559.nt[3] & 0x01);
}

static DECLFW(WriteMisc) {
	if (A & 0x04) {
		m559.nt[A & 0x03] = V;
		VRC24_SyncMirror();
	} else {
		m559.prg = V;
		VRC24_SyncPRG();
	}
}

static DECLFW(WriteVRC4) {
	/* nibblize address */
	if (A & 0x400) {
		V >>= 4;
	}
	VRC24_Write(A, V);
}

static void Power(void) {
	m559.nt[0] = 0;
	m559.nt[1] = 0;
	m559.nt[2] = 1;
	m559.nt[3] = 1;
	m559.prg = ~1;
	VRC24_Power();
	SetWriteHandler(0xB000, 0xFFFF, WriteVRC4);
}

void Mapper559_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x400, 0x800, 1, 1);
	info->Power = Power;
	VRC24_SyncMirror = SyncMirror;
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	VRC24_WriteExtSelect = WriteMisc;
	AddExState(StateRegs, ~0, 0, NULL);
}
