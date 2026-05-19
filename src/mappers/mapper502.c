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
 *
 */

/* NES 2.0 Mapper 502 */
/* Super Game 10-in-1 (Yhc002) */
/* BMC-Yhc-A/B/Uxrom-Cart */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[2];
} m502;

static SFORMAT StateRegs[] = {
	{ m502.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t mask = (8 << ((m502.reg[1] >> 4) & 0x03)) - 1;

	setprg4(0x7000, 0);
	if (m502.reg[1] & 0x06) {
		setprg32(0x8000, (m502.reg[0] << 2) + (latch.data & (mask >> 1)));
	} else {
		setprg16(0x8000, (m502.reg[0] << 3) + (latch.data & mask));
		setprg16(0xC000, (m502.reg[0] << 3) + mask);
	}
	setchr8(0);
	if (m502.reg[1] & 0x02) {
		setmirror(MI_0 + ((latch.data >> 4) & 0x01));
	} else {
		setmirror(m502.reg[1] & 0x01);
	}
}

static DECLFW(WriteReg) {
	if (!(m502.reg[1] & 0x80)) {
		m502.reg[A & 0x01] = V;
		Sync();
	}
}

static void Power(void) {
	memset(&m502, 0, sizeof(m502));
	Latch_Power();
	SetReadHandler(0x7000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x6FFF, WriteReg);
}

static void Reset(void) {
	memset(&m502, 0, sizeof(m502));
	Latch_RegReset();
}

void Mapper502_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
