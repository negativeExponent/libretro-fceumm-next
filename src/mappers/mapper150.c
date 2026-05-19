/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

/* Mapper 150 - SA-015 / SA-630 / Unif UNL-Sachen-74LS374N */
/* Mapper 243 - SA-020A */

#include "mapinc.h"

static struct {
	uint8_t cmd;
	uint8_t reg[8];
} m150;

static uint8_t dipsw;
static uint8_t compat_mode;

static SFORMAT StateRegs[] = {
	{ m150.reg, 8, "REGS" },
	{ &m150.cmd, 1, "CMD0" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg32(0x8000, (m150.reg[2] & 0x01) | m150.reg[5]);
}

static void SyncCHR(void) {
	if (compat_mode) { /* old dumps of m150/m243 with wrong bank order */
		setchr8((m150.reg[2] << 3) | ((m150.reg[6] & 0x03) << 1) | (m150.reg[4] & 0x01));
	} else if (iNESCart.mapper == 243) {
		setchr8((m150.reg[2] & 0x01) | ((m150.reg[4] << 1) & 0x02) | (m150.reg[6] << 2));
	} else { /* standard Mapper 150 */
		setchr8((m150.reg[6] & 0x03) | ((m150.reg[4] << 2) & 0x04) | (m150.reg[2] << 3));
	}
}

static void SyncMirror(void) {
	switch ((m150.reg[7] >> 1) & 0x03) {
	case 0:
		setmirrorw(0, 1, 1, 1);
		break;
	case 1:
		setmirror(MI_H);
		break;
	case 2:
		setmirror(MI_V);
		break;
	case 3:
		setmirror(MI_0);
		break;
	}
}

static DECLFR(Read) {
	if ((A & 0x101) == 0x101) {
		if (dipsw & 1)
			return (m150.reg[m150.cmd] & 0x03) | (cpu.openbus & 0xFC);
		else
			return (m150.reg[m150.cmd] & 0x07) | (cpu.openbus & 0xF8);
	}
	return cpu.openbus;
}

static DECLFW(Write) {
	if (dipsw & 0x01)
		V |= 0x04;
	switch (A & 0x101) {
	case 0x100:
		m150.cmd = V & 0x07;
		break;
	case 0x101:
		m150.reg[m150.cmd] = V & 0x07;
		SyncPRG();
		SyncCHR();
		SyncMirror();
		break;
	}
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Reset(void) {
	memset(&m150, 0, sizeof(m150));
	dipsw ^= 0x01;
	SyncPRG();
	SyncCHR();
	SyncMirror();
}

static void Power(void) {
	memset(&m150, 0, sizeof(m150));
	dipsw = 0;

	SyncPRG();
	SyncCHR();
	SyncMirror();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetReadHandler(0x4100, 0x5FFF, Read);
	SetWriteHandler(0x4100, 0x5FFF, Write);
}

void Mapper150_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	compat_mode = FALSE;
	if ((info->CRC32 == 0x19C1ED51) || /* Poker III (Sachen) [!].nes */
		(info->CRC32 == 0xF56D6D46) || /* Poker III (Sachen) [a1].nes */
		(info->CRC32 == 0x695CC180)) {    /* Honey Peach [with wrong CHR bank order] */
		compat_mode = TRUE;
		FCEU_printf(" Running in compatibility mode\n");
	}
}
