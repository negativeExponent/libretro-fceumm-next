/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright (C) 2019 Libretro Team
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
 * UNIF: BMC-60311C:
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_289
 */

#include "mapinc.h"
#include "latch.h"

static struct {
	uint8_t reg[2];
} m289;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m289.reg, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint16_t base = m289.reg[1] & ~0x07;

	if (m289.reg[0] & 0x02) {
		setprg16(0x8000, base | (latch.data & 0x07));
		setprg16(0xC000, base | 0x07);
	} else {
		uint16_t bank = base | (m289.reg[1] & 0x07);

		if (m289.reg[0] & 0x01) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	}
	/* CHR-RAM write-protect */
	SetupCartCHRMapping(0, CHRptr[0], 0x2000, ((m289.reg[0] >> 2) & 0x01) ^ 0x01);
	setchr8(0);
	setmirror(((m289.reg[0] >> 3) & 0x01) ^ 0x01);
}

static DECLFR(ReadDIP) {
	return (cpu.openbus & ~0x03) | (dipsw & 0x03);
}

static DECLFW(WriteReg) {
	m289.reg[A & 0x01] = V;
	Sync();
}

static void Power(void) {
	memset(&m289, 0, sizeof(m289));
	dipsw = 0;
	Latch_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

static void Reset(void) {
	memset(&m289, 0, sizeof(m289));
	dipsw++;
	Latch_RegReset();
}

void Mapper289_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
