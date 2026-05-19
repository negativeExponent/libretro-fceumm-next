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

/*
 * NES 2.0 Mapper 352
 * BMC-KS106C
 * - Kaiser 4-in-1(Unl,KS106C)[p1] - B-Wings, Kung Fu, 1942, SMB1
 */

#include "mapinc.h"

static struct {
	uint8_t reg;
} m352;

static SFORMAT StateRegs[] = {
	{ &m352.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, m352.reg);
	setchr8(m352.reg);
	setmirror(m352.reg & 0x01);
}

static void Power(void) {
	m352.reg = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
}

static void Reset(void) {
	m352.reg++;
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper352_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
