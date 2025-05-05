/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024 negativeExponent
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

static uint8 reg;

static SFORMAT StateRegs[] = {
	{ &reg, 1, "REGS" },
	{ 0 },
};

static void M448FixPRG(void) {
	if (reg & 0x08) { /* AOROM */
		setprg32(0x8000, ((reg << 2) & ~0x07) | (vrc24.prg[0] & 0x07));
	} else {
		if (reg & 0x04) { /* UOROM */
			setprg16(0x8000, ((reg << 3) & ~0x0F) | (vrc24.prg[0] & 0x0F));
			setprg16(0xC000, ((reg << 3) & ~0x0F) | 0x0F);
		} else { /* UNROM */
			setprg16(0x8000, (reg << 3) | (vrc24.prg[0] & 0x07));
			setprg16(0xC000, (reg << 3) | 0x07);
		}
	}
}

static void M448FixCHR(void) {
	setchr8(0);
}

static void M448FixMIRR(void) {
	if (reg & 0x08) { /* AOROM */
		setmirror(MI_0 + ((vrc24.prg[0] >> 4) & 0x01));
	} else {
		VRC24_SyncMirror_default();
	}
}

static DECLFW(M448WriteReg) {
	if (vrc24.cmd & 0x01) {
		reg = A & 0xFF;
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
	}
}

static DECLFW(M448WriteASIC) {
	if (reg & 0x08) {
		VRC24_Write(0x8000, V);
		M448FixMIRR();
	} else {
		VRC24_Write(A, V);
	}
}

static void M448Reset(void) {
	reg = 0;
	VRC24_Reset();
}

static void M448Power(void) {
	reg = 0;
	VRC24_Power();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, M448WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, M448WriteASIC);
}

static void StateRestore(int version) {
	VRC24_SyncPRG();
	VRC24_SyncCHR();
	VRC24_SyncMirror();
}

void Mapper448_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, 0, 1);
	VRC24_SyncPRG = M448FixPRG;
	VRC24_SyncCHR = M448FixCHR;
	VRC24_SyncMirror = M448FixMIRR;
	info->Reset = M448Reset;
	info->Power = M448Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
