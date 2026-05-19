/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 *
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[2];
} m319;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m319.reg, 2, "REG" },
	{ 0 }
};

static void Sync(void) {
	uint16_t bank, mask;

	if (iNESCart.CRC32 == 0xE5B9AB1F || iNESCart.PRGCRC32 == 0xC25FD362) {
		/* The publicly-available UNIF (UNL-HP898F) ROM file of Prima Soft 9999999-in-1 has
		 * the order of the 16 KiB PRG-ROM banks slightly mixed up, so that the
		 * PRG A14 mode bit operates on A16 instead of A14. To obtain the
		 * correct bank order, use UNIF 16 KiB PRG banks 0, 4, 1, 5, 2, 6, 3, 7.
		 */
		bank = (m319.reg[1] >> 3) & 7;
		mask = (m319.reg[1] >> 4) & 4;

		setprg16(0x8000, bank & ~mask);
		setprg16(0xC000, bank | mask);
	} else {
		bank = ((m319.reg[1] >> 2) & 0x06) | ((m319.reg[1] >> 5) & 0x01);
		mask = (m319.reg[1] >> 6) & 0x01;

		setprg16(0x8000, (bank & ~mask));
		setprg16(0xC000, (bank | mask));
	}

	bank = m319.reg[0] >> 4;
	mask = (m319.reg[0] << 2) & 0x04;

	setchr8((bank & ~mask) | ((latch.data << 2) & mask));
	setmirror(m319.reg[1] >> 7);
}

static DECLFR(ReadDIP) {
	return dipsw;
}

static DECLFW(WriteReg) {
	m319.reg[(A >> 2) & 0x01] = V;
	Sync();
}

static void Reset(void) {
	memset(&m319, 0, sizeof(m319));
	dipsw ^= 0x40;
	Sync();
}

static void Power(void) {
	memset(&m319, 0, sizeof(m319));
	Latch_Power();
	SetReadHandler(0x5000, 0x5FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper319_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
