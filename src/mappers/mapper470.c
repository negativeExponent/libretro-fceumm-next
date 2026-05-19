/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024-2025-2026 negativeExponent
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

/* NES 2.0 Mapper 470 denotes the INX_007T_V01 multicart circuit board,
 * used for the Retro-Bit re-release of Battletoads and Double Dragon.
 * It is basically AOROM with an additional outer bank register at $5000-$5FFF
 * whose data selects the 256 KiB outer bank.
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg;
} m470;

static SFORMAT StateRegs[] = {
	{ &m470.reg, 1, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, (m470.reg << 3) | (latch.data & 0x07));
	setchr8(0);
	setmirror((MI_0 + (latch.data >> 4) & 1));
}

static DECLFW(WriteReg) {
	m470.reg = V;
	Sync();
}

static void Reset(void) {
	memset(&m470, 0, sizeof(m470));
	Latch_RegReset();
}

static void Power(void) {
	memset(&m470, 0, sizeof(m470));
	Latch_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper470_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
