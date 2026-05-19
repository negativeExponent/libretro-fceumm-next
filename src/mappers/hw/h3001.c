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
 */

#include "mapinc.h"
#include "h3001.h"

H3001 h3001;

void (*H3001_pwrap)(uint16_t A, uint16_t V);
void (*H3001_cwrap)(uint16_t A, uint16_t V);

void (*H3001_SyncPRG)(void);
void (*H3001_SyncCHR)(void);
void (*H3001_SyncMirror)(void);

static SFORMAT StateRegs[] = {
	{ &h3001.cmd, 1, "CMD0" },
	{ h3001.prg, 2, "PREG" },
	{ h3001.chr, 8, "CREG" },
	{ &h3001.mirror, 1, "MIRR" },
	{ &h3001.IRQa, 1, "IRQA" },
	{ &h3001.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ &h3001.IRQLatch, 2 | FCEUSTATE_RLSB, "IRQL" },
	{ 0 }
};

void H3001_SetPRG_default(uint16_t A, uint16_t V) {
	setprg8(A, V);
}

void H3001_SetCHR_default(uint16_t A, uint16_t V) {
	setchr1(A, V);
}

void H3001_SyncPRG_default(void) {
	uint16_t pswap = (h3001.cmd & 0x80) ? 0x4000 : 0;

	H3001_pwrap(0x8000 ^ pswap, h3001.prg[0]);
	H3001_pwrap(0xA000,         h3001.prg[1]);
	H3001_pwrap(0xC000 ^ pswap, 0xFE);
	H3001_pwrap(0xE000,         0xFF);
}

void H3001_SyncCHR_default(void) {
	H3001_cwrap(0x0000, h3001.chr[0]);
	H3001_cwrap(0x0400, h3001.chr[1]);
	H3001_cwrap(0x0800, h3001.chr[2]);
	H3001_cwrap(0x0C00, h3001.chr[3]);
	H3001_cwrap(0x1000, h3001.chr[4]);
	H3001_cwrap(0x1400, h3001.chr[5]);
	H3001_cwrap(0x1800, h3001.chr[6]);
	H3001_cwrap(0x1C00, h3001.chr[7]);
}

void H3001_SyncMirror_default(void) {
	switch (h3001.mirror >> 6) {
	case 0: setmirror(MI_V); break;
	case 2: setmirror(MI_H); break;
	default: setmirror(MI_0); break;
	}
}

DECLFW(H3001_WritePRG) {
	h3001.prg[(A >> 13) & 0x01] = V;
	H3001_SyncPRG();
}

DECLFW(H3001_WriteMisc) {
	switch (A & 0x07) {
	case 0:
		h3001.cmd = V;
		H3001_SyncPRG();
		break;
	case 1:
		h3001.mirror = V;
		H3001_SyncMirror();
		break;
	case 3:
		h3001.IRQa = V & 0x80;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 4:
		h3001.IRQCount = h3001.IRQLatch;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 5:
		h3001.IRQLatch &= 0x00FF;
		h3001.IRQLatch |= V << 8;
		break;
	case 6:
		h3001.IRQLatch &= 0xFF00;
		h3001.IRQLatch |= V;
		break;
	}
}

DECLFW(H3001_WriteCHR) {
	h3001.chr[A & 0x07] = V;
	H3001_SyncCHR();
}

DECLFW(H3001_Write) {
	switch (A & 0xF000) {
	case 0x8000: H3001_WritePRG(A, V); break;
	case 0x9000: H3001_WriteMisc(A, V); break;
	case 0xA000: H3001_WritePRG(A, V); break;
	case 0xB000: H3001_WriteCHR(A, V); break;
	}
}

void H3001_CPUIRQHook(int a) {
	if (h3001.IRQa) {
		h3001.IRQCount -= a;
		if (h3001.IRQCount <= 0) {
			X6502_IRQBegin(FCEU_IQEXT);
			h3001.IRQa = 0;
		}
	}
}

void H3001_Reset(void) {
	H3001_SyncPRG();
	H3001_SyncCHR();
	H3001_SyncMirror();
}

void H3001_Power(void) {
	h3001.prg[0] = 0;
	h3001.prg[1] = 1;

	h3001.chr[0] = 0;
	h3001.chr[1] = 1;
	h3001.chr[2] = 2;
	h3001.chr[3] = 3;
	h3001.chr[4] = 4;
	h3001.chr[5] = 5;
	h3001.chr[6] = 6;
	h3001.chr[7] = 7;

	H3001_SyncPRG();
	H3001_SyncCHR();
	H3001_SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, H3001_WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, H3001_WriteMisc);
	SetWriteHandler(0xA000, 0xAFFF, H3001_WritePRG);
	SetWriteHandler(0xB000, 0xBFFF, H3001_WriteCHR);
}

void H3001_StateRestore(int version) {
	H3001_SyncPRG();
	H3001_SyncCHR();
	H3001_SyncMirror();
}

void H3001_Init(CartInfo *info) {
	H3001_pwrap = H3001_SetPRG_default;
	H3001_cwrap = H3001_SetCHR_default;

	H3001_SyncPRG = H3001_SyncPRG_default;
	H3001_SyncCHR = H3001_SyncCHR_default;
	H3001_SyncMirror = H3001_SyncMirror_default;

	info->Power = H3001_Power;
	info->Reset = H3001_Reset;
	MapIRQHook = H3001_CPUIRQHook;
	GameStateRestore = H3001_StateRestore;

	AddExState(StateRegs, ~0, 0, NULL);
}

void H3001_SetConfig(uint8_t clear) {
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x8FFF, H3001_WritePRG);
	SetWriteHandler(0x9000, 0x9FFF, H3001_WriteMisc);
	SetWriteHandler(0xA000, 0xAFFF, H3001_WritePRG);
	SetWriteHandler(0xB000, 0xBFFF, H3001_WriteCHR);
	if (clear) {
		h3001.prg[0] = 0;
		h3001.prg[1] = 1;

		h3001.chr[0] = 0;
		h3001.chr[1] = 1;
		h3001.chr[2] = 2;
		h3001.chr[3] = 3;
		h3001.chr[4] = 4;
		h3001.chr[5] = 5;
		h3001.chr[6] = 6;
		h3001.chr[7] = 7;

		h3001.cmd = 0;
		h3001.IRQa = 0;
		h3001.IRQCount = 0;
		h3001.IRQLatch = 0;
		h3001.mirror = 0;
	}
	H3001_SyncPRG();
	H3001_SyncCHR();
	H3001_SyncMirror();
}
