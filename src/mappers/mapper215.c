/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2011 CaH4e3
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
 * Submapper 0, UNIF board name UNL-8237:
 * Earthworm Jim 2
 * Mortal Kombat 3 (SuperGame, not Extra 60, not to be confused by similarly-named games from other developers)
 * Mortal Kombat 3 Extra 60 (both existing ROM images are just extracts of the 2-in-1 multicart containing this game)
 * Pocahontas Part 2
 * 2-in-1: Aladdin, EarthWorm Jim 2 (Super 808)
 * 2-in-1: The Lion King, Bomber Boy (GD-103)
 * 2-in-1: Super Golden Card: EarthWorm Jim 2, Boogerman (king002)
 * 2-in-1: Mortal Kombat 3 Extra 60, The Super Shinobi (king005)
 * 3-in-1: Boogerman, Adventure Island 3, Double Dragon 3 (Super 308)
 * 5-in-1: Golden Card: Aladdin, EarthWorm Jim 2, Garo Densetsu Special, Silkworm, Contra Force (SPC005)
 * 6-in-1: Golden Card: EarthWorm Jim 2, Mortal Kombat 3, Double Dragon 3, Contra 3, The Jungle Book, Turtles Tournament Fighters (SPC009)
 *
 * Submapper 1, UNIF board name UNL-8237A:
 * 9-in-1 High Standard Card: The Lion King, EarthWorm Jim 2, Aladdin, Boogerman, Somari, Turtles Tournament Fighters, Mortal Kombat 3, Captain Tsubasa 2, Taito Basketball (king001)
 */

/* TODO: Enable dipswitch when proper menu or ddipswitch database is established */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[8];
} m215;

static SFORMAT StateRegs[] = {
	{ m215.reg, 8, "EXPR" },
	{ 0 },
};

static void SetPRGBank_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = (m215.reg[0] & 0x40) ? 0x0F : 0x1F;
	uint16_t base = ((m215.reg[1] << 4) & 0x80) | ((m215.reg[1] << 5) & 0x60) | (m215.reg[1] & 0x10);

	/* if (dipsw) {
		if (dipsw & 0x01) {
			base &= dipsw;
		} else {
			base |= dipsw;
		}
	} */

	if (m215.reg[0] & 0x80) { /* NROM */
		uint16_t A14 = (m215.reg[0] >> 4) & 0x02;
		uint16_t tmpmask = (A14 | 0x01);
		V = (((m215.reg[0] & 0x0F) << 1) & ~tmpmask) | ((A >> 13) & tmpmask);
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHRBank_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = (m215.reg[0] & 0x40) ? 0x7F : 0xFF;
	uint16_t base = ((m215.reg[1] << 2) & 0x80) |
	    (m215.reg[1] << ((iNESCart.submapper == 1) ? 7 : 6) & 0x700);

	/* if (dipsw) {
		if (dipsw & 0x01) {
			base &= ((dipsw << 3) | 0x07);
		}
	} */

	setchr1(A, (base & ~mask) | (V & mask));
}

static const uint8_t protarray[8][8] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 0 Super Hang-On               */
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00 }, /* 1 Monkey King                 */
	{ 0x00, 0x00, 0x00, 0x00, 0x03, 0x04, 0x00, 0x00 }, /* 2 Super Hang-On/Monkey King   */
	{ 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x05, 0x00 }, /* 3 Super Hang-On/Monkey King   */
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 4                             */
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 5                             */
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, /* 6                             */
	{ 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x0F, 0x00 } /* 7 (default) Blood of Jurassic */
};

static DECLFR(ReadProtection) {
	return (cpu.openbus & ~0x0F) | (protarray[m215.reg[2] & 0x07][A & 0x07] & 0x0F);
}

static DECLFW(WriteReg) {
	m215.reg[A & 0x07] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static const uint8_t regperm[8][8] = {
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0, 2, 6, 1, 7, 3, 4, 5 },
	{ 0, 5, 4, 1, 7, 2, 6, 3 }, /* unused */
	{ 0, 6, 3, 7, 5, 2, 4, 1 },
	{ 0, 2, 5, 3, 6, 1, 7, 4 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* empty */
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* empty */
	{ 0, 1, 2, 3, 4, 5, 6, 7 }, /* empty */
};

static const uint16_t adrperm[8][8] = {
	{ 0x8000, 0x8001, 0xA000, 0xA001, 0xC000, 0xC001, 0xE000, 0xE001 },
	{ 0xA001, 0xA000, 0x8000, 0xC000, 0x8001, 0xC001, 0xE000, 0xE001 },
	{ 0x8000, 0x8001, 0xA000, 0xA001, 0xC000, 0xC001, 0xE000, 0xE001 }, /* unused */
	{ 0xC001, 0x8000, 0x8001, 0xA000, 0xA001, 0xE001, 0xE000, 0xC000 },
	{ 0xA001, 0x8001, 0x8000, 0xC000, 0xA000, 0xC001, 0xE000, 0xE001 },
	{ 0x8000, 0x8001, 0xA000, 0xA001, 0xC000, 0xC001, 0xE000, 0xE001 }, /* empty */
	{ 0x8000, 0x8001, 0xA000, 0xA001, 0xC000, 0xC001, 0xE000, 0xE001 }, /* empty */
	{ 0x8000, 0x8001, 0xA000, 0xA001, 0xC000, 0xC001, 0xE000, 0xE001 }, /* empty */
};

static DECLFW(WriteMMC3Reg) {
	A = adrperm[m215.reg[7] & 0x07][((A >> 12) & 0x06) | (A & 0x01)];
	switch (A & 0xE001) {
	case 0x8000:
		MMC3_Write(A, (V & 0xC0) | regperm[m215.reg[7] & 0x07][V & 0x07]);
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Power(void) {
	memset(&m215, 0, sizeof(m215));
	m215.reg[1] = 0xFF;
	m215.reg[2] = 0x07;
	m215.reg[7] = 0x04;
	/* dipsw = 0; */
	MMC3_Power();
	SetReadHandler(0x5000, 0x5FFF, ReadProtection);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteMMC3Reg);
}

static void Reset(void) {
	memset(&m215, 0, sizeof(m215));
	m215.reg[1] = 0xFF;
	m215.reg[7] = 0x04;
	m215.reg[2] = 0x07;
	/* dipsw = (dipsw + 1) & 0x3F; */
	MMC3_Reset();
}

void Mapper215_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHRBank_mmc3;
	MMC3_pwrap = SetPRGBank_mmc3;

	info->Power = Power;
	info->Reset = Reset;

	AddExState(StateRegs, ~0, 0, 0);

	if ((!info->iNES2) && (ROM.prg.size >= (2048 * 1024))) { /* UNL-8237A */
		info->submapper = 1;
	}
}
