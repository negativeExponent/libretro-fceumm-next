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
 * Konami VRC-3
 *
 */

#include "mapinc.h"
#include "vrc3.h"

VRC3 vrc3;

void (*VRC3_SyncPRG)(void);
void (*VRC3_SyncCHR)(void);

void (*VRC3_pwrap)(uint16_t A, uint16_t V);
void (*VRC3_cwrap)(uint16_t V);

static SFORMAT StateRegs[] = {
	{ &vrc3.prg, 1, "PREG" },
	{ &vrc3.IRQa, 1, "IRQA" },
	{ &vrc3.IRQx, 1, "IRQX" },
	{ &vrc3.IRQm, 1, "IRQM" },
	{ &vrc3.IRQLatch, 2 | FCEUSTATE_RLSB, "IRQL" },
	{ &vrc3.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ 0 }
};

void VRC3_SetPRG_default(uint16_t A, uint16_t V) {
	setprg16(A, V);
}

void VRC3_SetCHR_default(uint16_t V) {
	setchr8(V);
}

void VRC3_SyncPRG_default(void) {
	setprg8r(0x10, 0x6000, 0);
	VRC3_pwrap(0x8000, vrc3.prg);
	VRC3_pwrap(0xC000, ~0);
}

void VRC3_SyncCHR_default(void) {
	VRC3_cwrap(0);
}

DECLFW(VRC3_WriteReg) {
	switch (A & 0xF000) {
	case 0x8000:
		vrc3.IRQLatch &= 0xFFF0;
		vrc3.IRQLatch |= (V & 0x0F) << 0;
		break;
	case 0x9000:
		vrc3.IRQLatch &= 0xFF0F;
		vrc3.IRQLatch |= (V & 0x0F) << 4;
		break;
	case 0xA000:
		vrc3.IRQLatch &= 0xF0FF;
		vrc3.IRQLatch |= (V & 0x0F) << 8;
		break;
	case 0xB000:
		vrc3.IRQLatch &= 0x0FFF;
		vrc3.IRQLatch |= (V & 0x0F) << 12;
		break;
	case 0xC000:
		vrc3.IRQm = V & 0x04;
		vrc3.IRQx = V & 0x01;
		vrc3.IRQa = V & 0x02;
		if (vrc3.IRQa) {
			vrc3.IRQCount = vrc3.IRQLatch;
		}
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xD000:
		X6502_IRQEnd(FCEU_IQEXT);
		vrc3.IRQa = vrc3.IRQx;
		break;
	case 0xF000:
		vrc3.prg = V;
		VRC3_SyncPRG();
		VRC3_SyncCHR();
		break;
	}
}

void VRC3_CPUIRQHook(int a) {
	int32_t i;

	if (vrc3.IRQa) {
		for (i = 0; i < a; i++) {
			uint32_t IRQCountMask = vrc3.IRQm ? 0xFF : 0xFFFF;
			if ((vrc3.IRQCount & IRQCountMask) == IRQCountMask) {
				vrc3.IRQCount = vrc3.IRQLatch;
				X6502_IRQBegin(FCEU_IQEXT);
			} else {
				vrc3.IRQCount++;
			}
		}
	}
}

void VRC3_Reset(void) {
	VRC3_SyncPRG();
	VRC3_SyncCHR();
}

void VRC3_Power(void) {
	VRC3_SyncPRG();
	VRC3_SyncCHR();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0xFFFF, VRC3_WriteReg);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
}

void VRC3_StateRestore(int version) {
	VRC3_SyncPRG();
	VRC3_SyncCHR();
}

void VRC3_Init(CartInfo *info) {
	VRC3_pwrap = VRC3_SetPRG_default;
	VRC3_cwrap = VRC3_SetCHR_default;

	VRC3_SyncPRG = VRC3_SyncPRG_default;
	VRC3_SyncCHR = VRC3_SyncCHR_default;

	info->Power = VRC3_Power;
	MapIRQHook = VRC3_CPUIRQHook;

	AddExState(StateRegs, ~0, 0, NULL);
	GameStateRestore = VRC3_StateRestore;
}

void VRC3_SetConfig(uint8_t clear) {
	SetReadHandler (0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0xFFFF, VRC3_WriteReg);
	MapIRQHook = VRC3_CPUIRQHook;
	if (clear) {
		vrc3.prg = 0;
		vrc3.IRQa = 0;
		vrc3.IRQCount = 0;
		vrc3.IRQLatch = 0;
		vrc3.IRQm = 0;
		vrc3.IRQx = 0;
		X6502_IRQEnd(FCEU_IQEXT);
	}
	VRC3_SyncPRG();
	VRC3_SyncCHR();
}
