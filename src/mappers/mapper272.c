/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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
 * NES 2.0 Mapper 272 is used for a bootleg implementation of
 * 悪魔城 Special: ぼくDracula君 (Akumajō Special: Boku Dracula-kun).
 *
 * as implemented from
 * https://forums.nesdev.org/viewtopic.php?f=9&t=15302&start=60#p205862
 *
 */

#include "mapinc.h"

static struct {
	uint8_t prg[2];
	uint8_t chr[8];
	uint8_t mirrorHV;
	uint8_t mirrorOneScreen;
	uint8_t IRQCount;
	uint8_t IRQa;
} m272;

static uint16_t lastAddr;

static SFORMAT StateRegs[] = {
	{ m272.prg, 2, "PREG" },
	{ m272.chr, 8, "CREG" },
	{ &m272.mirrorHV, 1, "MIRR" },
	{ &m272.mirrorOneScreen, 1, "PALM" },
	{ &m272.IRQCount, 1, "IRQC" },
	{ &m272.IRQa, 1, "IRQa" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg8(0x8000, m272.prg[0]);
	setprg8(0xA000, m272.prg[1]);
	setprg16(0xC000, ~0);
}

static void SyncCHR(void) {
	setchr1(0x0000, m272.chr[0]);
	setchr1(0x0400, m272.chr[1]);
	setchr1(0x0800, m272.chr[2]);
	setchr1(0x0C00, m272.chr[3]);
	setchr1(0x1000, m272.chr[4]);
	setchr1(0x1400, m272.chr[5]);
	setchr1(0x1800, m272.chr[6]);
	setchr1(0x1C00, m272.chr[7]);
}

static void SyncMirror(void) {
	if (m272.mirrorOneScreen & 0x02) {
		setmirror(MI_0 + (m272.mirrorOneScreen & 0x01));
	} else {
		setmirror((m272.mirrorHV & 0x01) ^ 0x01);
	}
}

static DECLFW(Write) {
	/* writes to VRC chip */
	switch (A & 0xF000) {
	case 0x8000:
		m272.prg[0] = V;
		SyncPRG();
		break;
	case 0x9000:
		m272.mirrorHV = V;
		SyncMirror();
		break;
	case 0xA000:
		m272.prg[1] = V;
		SyncPRG();
		break;
	case 0xB000:
	case 0xC000:
	case 0xD000:
	case 0xE000: {
		int bank = (((A - 0xB000) >> 11) & 0x06) | ((A >> 1) & 0x01);
		if (A & 0x01) {
			m272.chr[bank] = (m272.chr[bank] & ~0xF0) | (V << 4);
		} else {
			m272.chr[bank] = (m272.chr[bank] & ~0x0F) | (V & 0x0F);
		}
		SyncCHR();
		break;
	}
	}

	/* writes to PAL chip */
	switch (A & 0xC00C) {
	case 0x8004:
		m272.mirrorOneScreen = V;
		SyncMirror();
		break;
	case 0x800C:
		X6502_IRQBegin(FCEU_IQEXT);
		break;
	case 0xC004:
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0xC008:
		m272.IRQa = 1;
		break;
	case 0xC00C:
		m272.IRQa = 0;
		m272.IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	}
}

static void PPUHook(uint32_t A) {
	if ((lastAddr & 0x2000) && !(A & 0x2000)) {
		if (m272.IRQa) {
			m272.IRQCount++;
			if (m272.IRQCount == 84) {
				m272.IRQCount = 0;
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
	lastAddr = A;
}

static void Reset(void) {
	memset(&m272, 0, sizeof(m272));
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Power(void) {
	memset(&m272, 0, sizeof(m272));
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, Write);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper272_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	PPU_hook = PPUHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
