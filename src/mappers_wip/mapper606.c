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
#include "latch.h"

static struct {
	uint8_t reg;
} m606;

static SFORMAT StateRegs[] = {
	{ &m606.reg, 4, "EXPR" },
	{ 0 }
 };

static void Sync(void) {
	if (m606.reg & 0x80) { /* ANROM mode */
		setprg32(0x8000, ((m606.reg >> 2) & 0x08) | ((m606.reg >> 4) & 0x04) | (latch.data & 0x03));
		setmirror(MI_0 + ((latch.data >> 4) & 0x01));
	} else { /* UNROM mode */
		setprg16(0x8000, ((m606.reg >> 1) & 0x10) | ((m606.reg >> 3) & 0x08) | (latch.data & 0x07));
		setprg16(0xC000, ((m606.reg >> 1) & 0x10) | ((m606.reg >> 3) & 0x08) | 0x07);
		setmirror((latch.data & 0x01) ^ 0x01);
	}
	setchr8(0);
}

static DECLFW(WriteReg) {
	if (~m606.reg & 0x10) {
		m606.reg = V;
		Sync();
	}
}

static void Reset(void) {
	m606.reg = 0;
	Latch_RegReset();
}

static void Power(void) {
	memset(&m606, 0, sizeof(m606));
	Latch_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper606_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
