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

/* iNES Mapper 141 - Sachen 8259A */

#include "mapinc.h"

static struct {
	uint8_t cmd;
	uint8_t reg[8];
} m141;

static SFORMAT StateRegs[] = {
	{ m141.reg, 8, "REGS" },
	{ &m141.cmd, 1, "CMD0" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg32(0x8000, m141.reg[5]);
}

static void SyncCHR(void) {
	if (!ROM.chr.size) {
		setchr8(0);
	} else {
		uint16_t base = m141.reg[4] << 3;

		setchr2(0x0000, ((base | (m141.reg[(m141.reg[7] & 0x01) ? 0 : 0] & 0x07)) << 1) | 0);
		setchr2(0x0800, ((base | (m141.reg[(m141.reg[7] & 0x01) ? 0 : 1] & 0x07)) << 1) | 1);
		setchr2(0x1000, ((base | (m141.reg[(m141.reg[7] & 0x01) ? 0 : 2] & 0x07)) << 1) | 0);
		setchr2(0x1800, ((base | (m141.reg[(m141.reg[7] & 0x01) ? 0 : 3] & 0x07)) << 1) | 1);
	}
}

static void SyncMirror(void) {
	switch (m141.reg[7] & 0x07) {
	case 0:
		setmirrorw(0, 0, 0, 1);
		break;
	case 2:
		setmirror(MI_H);
		break;
	default:
		setmirror(MI_V);
		break;
	case 6:
		setmirror(MI_0);
		break;
	}
}

static DECLFW(WriteReg) {
	if ((A & 0x4000) && (A & 0x100)) {
		if (A & 0x01) {
			m141.reg[m141.cmd & 0x07] = V;
			switch (m141.cmd & 0x07) {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 6:
				SyncCHR();
				break;
			case 7:
				SyncCHR();
				SyncMirror();
				break;
			case 5:
				SyncPRG();
				break;
			}
		} else {
			m141.cmd = V;
		}
	}
}

static void Reset(void) {
	memset(&m141, 0, sizeof(m141));

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper141_Init(CartInfo *info) {
	info->Power = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
