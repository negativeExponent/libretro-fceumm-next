/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright (C) 2019 Libretro Team
 * Copyright (C) 2023
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

static uint8 preg, creg, dipSwitch;

static SFORMAT StateRegs[] =
{
	{ &preg, 1, "PREG" },
	{ &creg, 1, "CREG" },
	{ &dipSwitch, 1, "DPSW" },
	{ 0 }
};

static void Sync(void) {
	uint32 prg = (preg & 7) | ((preg >> 3) & 0x08);			/* There is a high bit 3 of the PRG register that applies both to PRG and CHR */
	uint32 chr = (creg & 7) | ((preg >> 3) & 0x08);			/* There is a high bit 3 of the PRG register that applies both to PRG and CHR */
	uint32 mask = (creg & 0x10)? 0: (creg & 0x20)? 1: 3;	/* There is an CNROM mode that takes either two or four inner CHR banks from a CNROM-like latch.data register at $8000-$FFFF. */

	if (preg & 8) {
		setprg16(0x8000, prg);
		setprg16(0xc000, prg);
	} else
		setprg32(0x8000, prg >> 1);

	setchr8((chr & ~mask) | (latch.data & mask));			/* This "inner CHR bank" substitutes the respective bit(s) of the creg register. */
	setmirror(((preg >> 4) & 1) ^ 1);
}

static DECLFR(BMCWSRead) {
	if ((creg >> 6) & (dipSwitch & 3))
		return X.DB;
	return CartBR(A);
}

static DECLFW(BMCWSWrite) {
	if (preg & 0x20)
		return;

	switch (A & 1) {
		case 0:
			preg = V;
			Sync();
			break;
		case 1:
			creg = V;
			Sync();
			break;
	}
}

static void MBMCWSPower(void) {
	preg = 0;
	creg = 0;
	dipSwitch = 0;
	LatchPower();
	SetReadHandler(0x8000, 0xFFFF, BMCWSRead);
	SetWriteHandler(0x6000, 0x7FFF, BMCWSWrite);
}

static void BMCWSReset(void) {
	dipSwitch++; /* Soft-resetting cycles through solder pad or DIP switch settings */
	if (dipSwitch == 3)
		dipSwitch = 0; /* Only 00b, 01b and 10b settings are valid */

	/* Always reset to menu */
	preg = 0;
	creg = 0;
	LatchHardReset();
}

void BMCWS_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Reset = BMCWSReset;
	info->Power = MBMCWSPower;
	AddExState(&StateRegs, ~0, 0, 0);
}
