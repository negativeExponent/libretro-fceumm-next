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

/* NES 2.0 Mapper 357 is used for a 4-in-1 multicart (cartridge ID 4602) from Bit Corp.
 * The first game is Bit Corp's hack of the YUNG-08 conversion of Super Mario Brothers 2 (J) named Mr. Mary 2,
 * the other three games are UNROM games.
 *
 * Implementation is modified so reset actually sets the correct dipswitch (or outer banks) for each of the 4 games
 */

#include "mapinc.h"

static struct {
	uint8_t reg[4];
	uint8_t IRQa;
	uint16_t IRQCount;
} m357;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m357.reg, 4, "REG" },
	{ &m357.IRQCount, 2 | FCEUSTATE_RLSB, "IRQC" },
	{ &m357.IRQa, 1, "IRQA" },
	{ 0 }
};

static void Sync(void) {
	if (dipsw == 0) {
		static const uint8_t banks[2][8] = {
			{ 4, 3, 5, 3, 6, 3, 7, 3 },
			{ 1, 1, 5, 1, 4, 1, 5, 1 }
		};
		/* SMB2J Mode */
		setprg8(0x6000, m357.reg[1] ? 0 : 2);
		setprg8(0x8000, m357.reg[1] ? 0 : 1);
		setprg8(0xA000, 0);
		setprg8(0xC000, banks[m357.reg[1]][m357.reg[0]]);
		setprg8(0xE000, m357.reg[1] ? 8 : 10);
	} else {
		/* UNROM Mode */
		setprg16(0x8000, (dipsw << 3) | m357.reg[2]);
		setprg16(0xC000, (dipsw << 3) | 0x07);
	}
}

static DECLFW(WriteReg) {
	if (A & 0x8000) {
		m357.reg[2] = V & 0x07;
		Sync();
	}
	if ((A & 0x71FF) == 0x4022) {
		m357.reg[0] = V & 0x07;
		Sync();
	}
	if ((A & 0x71FF) == 0x4120) {
		m357.reg[1] = V & 0x01;
		Sync();
	}
	if ((A & 0xF1FF) == 0x4122) {
		m357.IRQa = V & 0x01;
		m357.IRQCount = 0;
		X6502_IRQEnd(FCEU_IQEXT);
	}
}

static void Power(void) {
	memset(&m357, 0, sizeof(m357));
	m357.reg[0] = 0x03;
	dipsw = 0;
	setchr8(0);
	setmirror(MI_V);
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0xFFFF, WriteReg);
}

static void Reset(void) {
	memset(&m357, 0, sizeof(m357));
	m357.reg[0] = 0x03;
	dipsw++;
	dipsw &= 3;
	setmirror((dipsw == 3) ? MI_H : MI_V);
	Sync();
}

static void CPUIRQHook(int a) {
	if (m357.IRQa) {
		m357.IRQCount += a;
		if (m357.IRQCount & 0x1000) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper357_Init(CartInfo *info) {
	info->Reset = Reset;
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
