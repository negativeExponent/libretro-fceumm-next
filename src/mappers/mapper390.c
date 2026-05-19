/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
 *  Copyright (C) 2023-2024-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 390 - Realtec 8031 */
/* NOTE: Duplicate of Mapper 236 (CHR-ROM variant */

#include "mapinc.h"

static struct {
	uint8_t reg[2];
} m390;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m390.reg, 2, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	switch (m390.reg[1] & 0x30) {
	case 0x00:
	case 0x10: /* UNROM */
		setprg16(0x8000, m390.reg[1]);
		setprg16(0xC000, m390.reg[1] | 0x07);
		break;
	case 0x20: /* Maybe unused, NROM-256? */
		setprg32(0x8000, m390.reg[1] >> 1);
		break;
	case 0x30: /* NROM-128 */
		setprg16(0x8000, m390.reg[1]);
		setprg16(0xC000, m390.reg[1]);
		break;
	}
}

static void SyncCHR(void) {
	setchr8(m390.reg[0]);
}

static void SyncMirror(void){
	setmirror(((m390.reg[0] & 0x20) >> 5) ^ 1);
}

static DECLFR(ReadDIP) {
	uint8_t ret = CartBR(A);
	if ((m390.reg[1] & 0x30) == 0x10)
		ret |= dipsw;
	return ret;
}

static DECLFW(WriteCHRMirror) {
	m390.reg[0] = A & 0x3F;
	SyncCHR();
	SyncMirror();
}

static DECLFW(WritePRG) {
	m390.reg[1] = A & 0x3F;
	SyncPRG();
}

static void Reset(void) {
	dipsw = 11; /* hard-coded 150-in-1 menu */
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Power(void) {
	memset(&m390, 0, sizeof(m390));
	dipsw = 11; /* hard-coded 150-in-1 menu */
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x8000, 0xBFFF, WriteCHRMirror);
	SetWriteHandler(0xC000, 0xFFFF, WritePRG);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

void Mapper390_Init(CartInfo *info) {
	info->Reset = Reset;
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
