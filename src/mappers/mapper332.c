/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright (C) 2019 Libretro Team
 * Copyright (C) 2023-2025-2026 negativeExponent
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

/* added 2019-5-23
 * NES 2.0 Mapper 332
 * BMC-WS Used for  Super 40-in-1 multicart
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_332 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[2];
} m332;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m332.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint32_t prg = ((m332.reg[0] >> 3) & 0x08) | (m332.reg[0] & 0x07);
	uint32_t chr = ((m332.reg[0] >> 3) & 0x08) | (m332.reg[1] & 0x07);
	uint32_t mask = (m332.reg[1] & 0x10) ? 0 : (m332.reg[1] & 0x20) ? 1 : 3;

	if (m332.reg[0] & 0x08) {
		setprg16(0x8000, prg);
		setprg16(0xc000, prg);
	} else {
		setprg32(0x8000, prg >> 1);
	}
	setchr8((chr & ~mask) | (latch.data & mask));
	setmirror(((m332.reg[0] >> 4) & 0x01) ^ 0x01);
}

static DECLFR(ReadDIP) {
	if ((m332.reg[1] >> 6) & (dipsw & 0x03)) {
		return cpu.openbus;
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	if (!(m332.reg[0] & 0x20)) {
		m332.reg[A & 0x01] = V;
		Sync();
	}
}

static void Reset(void) {
	memset(&m332, 0, sizeof(m332));
	dipsw++; /* Soft-resetting cycles through solder pad or DIP switch settings */
	if (dipsw == 3) {
		dipsw = 0; /* Only 00b, 01b and 10b settings are valid */
	}
	Latch_RegReset();
}

static void Power(void) {
	memset(&m332, 0, sizeof(m332));
	dipsw = 0;
	Latch_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper332_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
