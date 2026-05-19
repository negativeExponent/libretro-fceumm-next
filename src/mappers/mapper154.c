/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

/* NES 2.0 Mapper 154 It is identical to Mapper 88, but with the addition of a single bit allowing for mapper-controlled one-screen regoring: */

#include "mapinc.h"
#include "n118.h"

static struct {
	uint8_t mirror;
} m154;

static SFORMAT StateRegs[] = {
	{ &m154.mirror, 1, "MIRR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x0F);
}

static void SyncCHR(void) {
	setchr2(0x0000, (n118.reg[0] & 0x3F) >> 1);
	setchr2(0x0800, (n118.reg[1] & 0x3F) >> 1);
	setchr1(0x1000, 0x40 | (n118.reg[2] & 0x3F));
	setchr1(0x1400, 0x40 | (n118.reg[3] & 0x3F));
	setchr1(0x1800, 0x40 | (n118.reg[4] & 0x3F));
	setchr1(0x1C00, 0x40 | (n118.reg[5] & 0x3F));
}

static void SyncMirror(void) {
	setmirror(MI_0 + ((m154.mirror >> 6) & 0x01));
}

static DECLFW(WriteReg) {
	if (A <= 0x9FFF) {
		N118_Write(A, V);
	}
	if ((m154.mirror & 0x40) != (V & 0x40)) {
		/* mirroring bit is present over the entire 32KB reange */
		m154.mirror = V;
		SyncMirror();
	}
}

static void Power(void) {
	memset(&m154, 0, sizeof(m154));
	N118_Power();
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	N118_SyncPRG();
	N118_SyncCHR();
	SyncMirror();
}

void Mapper154_Init(CartInfo *info) {
	N118_Init(info, 0, 0);
	info->Power = Power;
	N118_pwrap = SetPRG;
	N118_SyncCHR = SyncCHR;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
