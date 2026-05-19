/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 * Konami VRC-1
 *
 */

#include "mapinc.h"
#include "vrc1.h"

VRC1 vrc1;

void (*VRC1_SyncPRG)(void);
void (*VRC1_SyncCHR)(void);
void (*VRC1_SyncMirror)(void);

void (*VRC1_pwrap)(uint16_t A, uint16_t V);
void (*VRC1_cwrap)(uint16_t A, uint16_t V);

static SFORMAT StateRegs[] = {
	{ &vrc1.mode, 1, "MODE" },
	{ vrc1.chr, 2, "CREG" },
	{ vrc1.prg, 3, "PREG" },
	{ 0 }
};

void VRC1_SetPRG_default(uint16_t A, uint16_t V) {
    setprg8(A, V);
}

void VRC1_SetCHR_default(uint16_t A, uint16_t V) {
    setchr4(A, V);
}

void VRC1_SyncPRG_default(void) {
	VRC1_pwrap(0x8000, vrc1.prg[0]);
	VRC1_pwrap(0xA000, vrc1.prg[1]);
	VRC1_pwrap(0xC000, vrc1.prg[2]);
	VRC1_pwrap(0xE000, ~0);
}

void VRC1_SyncCHR_default(void) {
	VRC1_cwrap(0x0000, ((vrc1.mode << 3) & 0x10) | (vrc1.chr[0] & 0x0F));
	VRC1_cwrap(0x1000, ((vrc1.mode << 2) & 0x10) | (vrc1.chr[1] & 0x0F));
}

void VRC1_SyncMirror_default(void) {
	if (iNESCart.mirror == MI_4) {
		setmirror(MI_4);
	} else {
		setmirror((vrc1.mode & 1) ^ 1);
	}
}

DECLFW(VRC1_WritePRG) {
	vrc1.prg[(A >> 13) & 0x03] = V;
	VRC1_SyncPRG();
}

DECLFW(VRC1_WriteMode) {
	vrc1.mode = V;
	VRC1_SyncCHR();
	VRC1_SyncMirror();
}

DECLFW(VRC1_WriteCHR) {
	vrc1.chr[(A >> 12) & 0x01] = V;
	VRC1_SyncCHR();
}

DECLFW(VRC1_Write) {
	switch (A & 0xF000) {
	case 0x8000: VRC1_WritePRG(A, V); break;
	case 0x9000: VRC1_WriteMode(A, V); break;
	case 0xA000:
	case 0xB000:
	case 0xC000:
	case 0xD000: VRC1_WritePRG(A, V); break;
	case 0xE000:
	case 0xF000: VRC1_WriteCHR(A, V); break;
	}
}


void VRC1_Reset(void) {
    VRC1_SyncPRG();
    VRC1_SyncCHR();
    VRC1_SyncMirror();
}

void VRC1_Power(void) {
	memset(&vrc1, 0, sizeof(vrc1));

	VRC1_SyncPRG();
	VRC1_SyncCHR();
	VRC1_SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, VRC1_WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, VRC1_WriteMode);
	SetWriteHandler(0xA000, 0xDFFF, VRC1_WritePRG);
	SetWriteHandler(0xE000, 0xFFFF, VRC1_WriteCHR);
}

void VRC1_StateRestore(int version) {
	VRC1_SyncPRG();
	VRC1_SyncCHR();
	VRC1_SyncMirror();
}

void VRC1_Init(CartInfo *info) {
    VRC1_pwrap = VRC1_SetPRG_default;
	VRC1_cwrap = VRC1_SetCHR_default;

	VRC1_SyncPRG = VRC1_SyncPRG_default;
	VRC1_SyncCHR = VRC1_SyncCHR_default;
	VRC1_SyncMirror = VRC1_SyncMirror_default;

    info->Power = VRC1_Power;
    info->Reset = VRC1_Reset;
	GameStateRestore = VRC1_StateRestore;

	AddExState(StateRegs, ~0, 0, NULL);
}

void VRC1_SetConfig(uint8_t clear) {
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, VRC1_WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, VRC1_WriteMode);
	SetWriteHandler(0xA000, 0xDFFF, VRC1_WritePRG);
	SetWriteHandler(0xE000, 0xFFFF, VRC1_WriteCHR);
	if (clear) {
		vrc1.chr[0] = 0;
		vrc1.chr[1] = 0;
		vrc1.prg[0] = 0;
		vrc1.prg[1] = 0;
		vrc1.prg[2] = 0;
		vrc1.mode = 0;
	}
	VRC1_SyncPRG();
	VRC1_SyncCHR();
	VRC1_SyncMirror();
}

