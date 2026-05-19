/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
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
 * VRC-6
 *
 */

#include "mapinc.h"
#include "vrc6.h"
#include "vrcirq.h"
#include "vrc6sound.h"

VRC6 vrc6;

void (*VRC6_pwrap)(uint16_t A, uint16_t V);
void (*VRC6_cwrap)(uint16_t A, uint16_t V);

static SFORMAT StateRegs[] = {
	{ vrc6.prg, 2, "PRG" },
	{ vrc6.chr, 8, "CHR" },
	{ &vrc6.mirr, 1, "MIRR" },
	{ &vrc6.A0, 2 | FCEUSTATE_RLSB, "V6A0" },
	{ &vrc6.A1, 2 | FCEUSTATE_RLSB, "V6A1" },

	{ 0 }
};

static void GENPWRAP(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x3F);
}

static void GENCWRAP(uint16_t A, uint16_t V) {
	setchr1(A, V & 0xFF);
}

void VRC6_SyncMirror(void) {
	switch (vrc6.mirr & 3) {
	case 0:
		setmirror(MI_V);
		break;
	case 1:
		setmirror(MI_H);
		break;
	case 2:
		setmirror(MI_0);
		break;
	case 3:
		setmirror(MI_1);
		break;
	}
}

void VRC6_SyncPRG(void) {
	VRC6_pwrap(0x8000, (vrc6.prg[0] << 1) | 0x00);
	VRC6_pwrap(0xa000, (vrc6.prg[0] << 1) | 0x01);
	VRC6_pwrap(0xc000, vrc6.prg[1]);
	VRC6_pwrap(0xe000, ~0);
}

void VRC6_SyncCHR(void) {
	int i;

	for (i = 0; i < 8; i++) {
		VRC6_cwrap(i << 10, vrc6.chr[i]);
	}
}

DECLFW(VRC6_Write) {
	int index;

	A = (A & 0xF000) | ((A & vrc6.A1) ? 0x02 : 0x00) | ((A & vrc6.A0) ? 0x01 : 0x00);
	switch (A & 0xF000) {
	case 0x8000:
		vrc6.prg[0] = V;
		VRC6_pwrap(0x8000, (V << 1) | 0x00);
		VRC6_pwrap(0xA000, (V << 1) | 0x01);
		break;
	case 0x9000:
		VRC6Sound_Write(A, V);
		break;
	case 0xA000:
		VRC6Sound_Write(A, V);
		break;
	case 0xB000:
		VRC6Sound_Write(A, V);
		if ((A & 0x03) == 0x03) {
			vrc6.mirr = (V >> 2) & 3;
			VRC6_SyncMirror();
		}
		break;
	case 0xC000:
		vrc6.prg[1] = V;
		VRC6_pwrap(0xC000, V);
		break;
	case 0xD000:
	case 0xE000:
		index = ((A - 0xD000) >> 10) | (A & 0x03);
		vrc6.chr[index] = V;
		VRC6_cwrap(index << 10, V);
		break;
	case 0xF000:
		index = A & 0x03;
		switch (index) {
		case 0x00:
			VRCIRQ_Latch(V);
			break;
		case 0x01:
			VRCIRQ_Control(V);
			break;
		case 0x02:
			VRCIRQ_Acknowledge();
			break;
		}
	}
}

void VRC6_IRQCPUHook(int a) {
	VRCIRQ_CPUHook(a);
}

void VRC6_Reset(void) {
	vrc6.prg[0] = 0;
	vrc6.prg[1] = 1;

	vrc6.chr[0] = 0;
	vrc6.chr[1] = 1;
	vrc6.chr[2] = 2;
	vrc6.chr[3] = 3;
	vrc6.chr[4] = 4;
	vrc6.chr[5] = 5;
	vrc6.chr[6] = 6;
	vrc6.chr[7] = 7;

	vrc6.mirr = 0;

	VRC6_SyncPRG();
	VRC6_SyncCHR();
	VRC6_SyncMirror();
}

void VRC6_Power(void) {
	VRC6_Reset();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, VRC6_Write);

	if (WRAMSIZE) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

void VRC6_Restore(int version) {
	VRC6_SyncPRG();
	VRC6_SyncCHR();
	VRC6_SyncMirror();
}

void VRC6_Init(CartInfo *info, uint32_t A0, uint32_t A1, int wram) {
	memset(&vrc6, 0, sizeof(vrc6));

	VRC6_pwrap = GENPWRAP;
	VRC6_cwrap = GENCWRAP;

	vrc6.A0 = A0;
	vrc6.A1 = A1;

	if (wram) {
		if (info->iNES2) {
			WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
		} else if (info->mapper == 26) {
			WRAMSIZE = 8192;
		}
		if (WRAMSIZE) {
			WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
			SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
			AddExState(WRAM, WRAMSIZE, 0, "WRAM");
			if (info->battery) {
				info->SaveGame[0] = WRAM;
				info->SaveGameLen[0] = WRAMSIZE;
			}
		}
	}
	AddExState(StateRegs, ~0, 0, NULL);

	info->Power = VRC6_Power;
	GameStateRestore = VRC6_Restore;

	VRCIRQ_Init(TRUE);
	MapIRQHook = VRC6_IRQCPUHook;
	AddExState(&VRCIRQ_StateRegs, ~0, 0, 0);

	VRC6Sound_ESI();
	VRC6Sound_AddStateInfo();
}

void VRC6_SetConfig(uint8_t clear, int A0, int A1) {
	vrc6.A0 = A0;
	vrc6.A1 = A1;
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, VRC6_Write);
	VRCIRQ_Init(TRUE);
	MapIRQHook = VRC6_IRQCPUHook;
	if (clear) {
		VRC6_Reset();
	} else {
		VRC6_SyncPRG();
		VRC6_SyncCHR();
		VRC6_SyncMirror();
	}
}
