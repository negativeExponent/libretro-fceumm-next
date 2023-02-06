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

static uint8 reg[2];
static uint8 solderpad;

static SFORMAT StateRegs[] =
{
	{ reg, 2, "REGS" },
	{ &solderpad, 1, "PADS" },
	{ 0 }
};

static void Sync(void) {
	uint32 bank = reg[1] & 0x7F;
	switch (reg[0] & 3) {
	case 0: /* NROM-128 */
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
		break;
	case 1: /* NROM-256 */
		setprg32(0x8000, bank >> 1);
		break;
	case 2: /* UNROM */
		setprg16(0x8000, bank | (latch.data & 7));
		setprg16(0xC000, bank | 7);
		break;
	case 3: /* A14-A16=1 regardless of CPU A14 */
		setprg16(0x8000, bank | 7);
		setprg16(0xC000, bank | 7);
		break;
	}
	/* CHR-RAM write-protect */
	SetupCartCHRMapping(0, CHRptr[0], 0x2000, ((reg[0] >> 2) & 1) ^ 1);
	setchr8(0);
	setmirror(((reg[0] >> 3) & 1) ^ 1);
}

static DECLFR(ReadPad) {
	return (X.DB & ~3) | (solderpad & 3);
}

static DECLFW(WriteReg) {
	reg[A & 1] = V;
	Sync();
}

static void BMC60311CPower(void) {
	reg[0] = reg[1] = 0;
	solderpad = 0;
	LatchPower();
	SetReadHandler(0x6000, 0x7FFF, ReadPad);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

static void BMC60311CReset(void) {
	reg[0] = reg[1] = 0;
	solderpad++;
	LatchHardReset();
}

void BMC60311C_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 0, 0);
	info->Power = BMC60311CPower;
	info->Reset = BMC60311CReset;
	AddExState(&StateRegs, ~0, 0, 0);
}
