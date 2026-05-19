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

/* NES 2.0 Mapper 448
 * VRC4-based 830768C multicart circuit board used by a Super 6-in-1 multicart.
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t reg;
} m448;

static SFORMAT StateRegs[] = {
	{ &m448.reg, 1, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	if (m448.reg & 0x08) { /* AOROM */
		setprg32(0x8000, ((m448.reg << 2) & ~0x07) | (vrc24.prg[0] & 0x07));
	} else {
		if (m448.reg & 0x04) { /* UOROM */
			setprg16(0x8000, ((m448.reg << 3) & ~0x0F) | (vrc24.prg[0] & 0x0F));
			setprg16(0xC000, ((m448.reg << 3) & ~0x0F) | 0x0F);
		} else { /* UNROM */
			setprg16(0x8000, (m448.reg << 3) | (vrc24.prg[0] & 0x07));
			setprg16(0xC000, (m448.reg << 3) | 0x07);
		}
	}
}

static void SyncCHR(void) {
	setchr8(0);
}

static void SyncMirror(void) {
	if (m448.reg & 0x08) { /* AOROM */
		setmirror(MI_0 + ((vrc24.prg[0] >> 4) & 0x01));
	} else {
		VRC24_SyncMirror_default();
	}
}

static DECLFW(WriteReg) {
	if (vrc24.cmd & 0x01) {
		m448.reg = A & 0xFF;
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
	}
}

static DECLFW(WriteVRC4) {
	if (m448.reg & 0x08) {
		VRC24_Write(0x8000, V);
		SyncMirror();
	} else {
		VRC24_Write(A, V);
	}
}

static void Reset(void) {
	memset(&m448, 0, sizeof(m448));
	VRC24_Reset();
}

static void Power(void) {
	memset(&m448, 0, sizeof(m448));
	VRC24_Power();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteVRC4);
}

static void StateRestore(int version) {
	VRC24_SyncPRG();
	VRC24_SyncCHR();
	VRC24_SyncMirror();
}

void Mapper448_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, 0, 1);
	VRC24_SyncPRG = SyncPRG;
	VRC24_SyncCHR = SyncCHR;
	VRC24_SyncMirror = SyncMirror;
	info->Reset = Reset;
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
