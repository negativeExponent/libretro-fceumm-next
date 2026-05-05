/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2026 negativeExponent
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"

static struct {
	uint8_t reg[2];
} m598;

static SFORMAT StateRegs[] = {
	{ m598.reg, 2, "EXPR" },
	{ 0 }
 };

static void Sync(void) {
	setprg16(0x8000, (m598.reg[0] & 0x18) | (m598.reg[1] & 0x07));
	setprg16(0xC000, (m598.reg[0] & 0x18) | 0x07);
	setchr8(0);
	switch (m598.reg[0] & 0x03) {
	case 0x00:
		setmirror(MI_0);
		break;
	case 0x01:
		setmirror(MI_V);
		break;
	case 0x02:
		setmirror(MI_H);
		break;
	case 0x03:
		setmirror(MI_1);
		break;
	}
}

static DECLFW(WriteReg) {
	m598.reg[(A >> 14) & 1] = V;
	Sync();
}

static void Reset(void) {
	memset(&m598, 0, sizeof(m598));
	Sync();
}

static void Power(void) {
	memset(&m598, 0, sizeof(m598));
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper598_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
