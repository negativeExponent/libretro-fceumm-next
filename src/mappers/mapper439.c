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

/* NES 2.0 Mapper 439 denotes the YS2309 multicart PCB.
 * Its UNIF MAPRs are BMC-DS-07 and BMC-K86B.
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[2];
} m439;

static SFORMAT StateRegs[] = {
	{ m439.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t mask = ((~m439.reg[1] >> 1) & 0x38) | 0x07;
	uint8_t base = m439.reg[0] >> 1;

	setprg16(0x8000, (base & ~mask) | (latch.data & mask));
	setprg16(0xC000, (base & ~mask) | (0x3F & mask));
	setchr8(0);
	setmirror(((latch.data >> 7) & 0x01) ^ 0x01);
}

static DECLFW(WriteReg) {
	m439.reg[A & 0x01] = V;
	Sync();
}

static DECLFW(WriteLatch) {
	/* mask to protect mirroring and A19-A17 on latch from being updated depending on $6001 m439.reg */
	uint8_t mask = (m439.reg[1] & 0x80) | ((m439.reg[1] >> 1) & 0x38);
	Latch_Write(A, (V & ~mask) | (latch.data & mask));
}

static void Reset(void) {
	memset(&m439, 0, sizeof(m439));
	Latch_RegReset();
}

static void Power(void) {
	memset(&m439, 0, sizeof(m439));
	Latch_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);
}

void Mapper439_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
