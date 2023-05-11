/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
 *  Copyright (C) 2023
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

#include "mapinc.h"
#include "latch.h"

static uint32 dipswitch = 0;
static uint32 submapper = 0;

/*------------------ BMCD1038 ---------------------------*/
/* iNES Mapper 59 */

static void BMCD1038Sync(void) {
	if (latch.addr & 0x80) {
		setprg16(0x8000, (latch.addr & 0x70) >> 4);
		setprg16(0xC000, (latch.addr & 0x70) >> 4);
	} else {
		setprg32(0x8000, (latch.addr & 0x60) >> 5);
	}
	setchr8(latch.addr & 7);
	setmirror(((latch.addr & 8) >> 3) ^ 1);
}

static DECLFR(BMCD1038Read) {
	if (latch.addr & 0x100) {
		return (X.DB & ~3) | (dipswitch & 3);
	}
	return CartBR(A);
}

static DECLFW(BMCD1038Write) {
	/* Only recognize the latch write if the lock bit has not been set. */
	/* Needed for NT-234 "Road Fighter" */
	if (~latch.addr & 0x200) {
		LatchWrite(A, V);
	}
}

static void BMCD1038Reset(void) {
	dipswitch++;
	/* Always reset to menu */
	latch.addr = 0;
	BMCD1038Sync();
}

static void BMCD1038Power(void) {
	LatchPower();

	/* Trap latch writes to enforce the "Lock" bit */
	SetWriteHandler(0x8000, 0xFFFF, BMCD1038Write);
}

void BMCD1038_Init(CartInfo *info) {
	Latch_Init(info, BMCD1038Sync, BMCD1038Read, 0, 0);
	info->Reset = BMCD1038Reset;
	info->Power = BMCD1038Power;
	AddExState(&dipswitch, 1, 0, "DIPSW");
}

/*------------------ Map 058 ---------------------------*/

static void M58Sync(void) {
	if (latch.addr & 0x40) {
		setprg16(0x8000, latch.addr & 7);
		setprg16(0xC000, latch.addr & 7);
	} else {
		setprg32(0x8000, (latch.addr >> 1) & 3);
	}
	setchr8((latch.addr >> 3) & 7);
	setmirror(((latch.addr & 0x80) >> 7) ^ 1);
}

void Mapper58_Init(CartInfo *info) {
	Latch_Init(info, M58Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/*------------------ Map 061 ---------------------------*/
static void M61Sync(void) {
	if (latch.addr & 0x10) {
		setprg16(0x8000, ((latch.addr & 0xF) << 1) | ((latch.addr & 0x20) >> 5));
		setprg16(0xC000, ((latch.addr & 0xF) << 1) | ((latch.addr & 0x20) >> 5));
	} else {
		setprg32(0x8000, latch.addr & 0xF);
	}
	setchr8(0);
	setmirror(((latch.addr >> 7) & 1) ^ 1);
}

void Mapper61_Init(CartInfo *info) {
	Latch_Init(info, M61Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/*------------------ Map 063 ---------------------------*/
/* added 2019-5-23
 * Mapper 63 NTDEC-Multicart
 * http://wiki.nesdev.com/w/index.php/INES_Mapper_063
 * - Powerful 250-in-1
 * - Hello Kitty 255-in-1 */

static uint16 openBus   = 0;
static uint8 hasBattery = 0;

extern uint32 ROM_size;

static DECLFR(M63Read) {
	if (openBus)
		return X.DB;
	return CartBR(A);
}

static void M63Sync(void) {
	uint8 prg_mask = (submapper == 0) ? 0xFF : 0x7F;
	uint8 prg_bank = (latch.addr >> 2) & prg_mask;
	uint8 chr_protect = (latch.addr & ((submapper == 0) ? 0x400 : 0x200)) == 0;
	if (latch.addr & 2) {
		setprg32(0x8000, prg_bank >> 1);
	} else {
		setprg16(0x8000, prg_bank);
		setprg16(0xC000, prg_bank);
	}
	setchr8(0);
	setmirror((latch.addr & 1) ^ 1);
	/* return openbus for unpopulated rom banks */
	openBus = prg_bank >= ROM_size;
	/* chr-ram protect */
	SetupCartCHRMapping(0, CHRptr[0], 0x2000, chr_protect);
}

void Mapper63_Init(CartInfo *info) {
	Latch_Init(info, M63Sync, M63Read, 0, 0);
	info->Reset = LatchHardReset;
	submapper = info->submapper;
}

/*------------------ Map 092 ---------------------------*/
/* Another two-in-one mapper, two Jaleco carts uses similar
 * hardware, but with different wiring.
 * Original code provided by LULU
 * Additionally, PCB contains DSP extra sound chip, used for voice samples (unemulated)
 */

static void M92Sync(void) {
	uint8 reg = latch.addr & 0xF0;
	setprg16(0x8000, 0);
	if (latch.addr >= 0x9000) {
		switch (reg) {
			case 0xD0:
				setprg16(0xc000, latch.addr & 15);
				break;
			case 0xE0:
				setchr8(latch.addr & 15);
				break;
		}
	} else {
		switch (reg) {
			case 0xB0:
				setprg16(0xc000, latch.addr & 15);
				break;
			case 0x70:
				setchr8(latch.addr & 15);
				break;
		}
	}
}

static void M92Power() {
	LatchPower();
	latch.addr = 0x80B0;
	M92Sync();
}

void Mapper92_Init(CartInfo *info) {
	Latch_Init(info, M92Sync, NULL, 0, 0);
	info->Power = M92Power;
}

/*------------------ Map 200 ---------------------------*/

static void M200Sync(void) {
	setprg16(0x8000, latch.addr & 7);
	setprg16(0xC000, latch.addr & 7);
	setchr8(latch.addr & 7);
	setmirror(((latch.addr >> ((submapper == 0) ? 3 : 2)) & 1) ^ 1);
}

void Mapper200_Init(CartInfo *info) {
	/* TODO: init was ~0 */
	Latch_Init(info, M200Sync, NULL, 0, 0);
	submapper = info->submapper;
}

/*------------------ Map 201 ---------------------------*/

static void M201Sync(void) {
	setprg32(0x8000, latch.addr);
	setchr8(latch.addr);
}

void Mapper201_Init(CartInfo *info) {
	Latch_Init(info, M201Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/*------------------ Map 202 ---------------------------*/

static void M202Sync(void) {
	if ((latch.addr & 0x9) == 0x09) {
		setprg32(0x8000, latch.addr >> 2);
	} else {
		setprg16(0x8000, latch.addr >> 1);
		setprg16(0xC000, latch.addr >> 1);
	}
	setchr8(latch.addr >> 1);
	setmirror((latch.addr & 1) ^ 1);
}

void Mapper202_Init(CartInfo *info) {
	Latch_Init(info, M202Sync, NULL, 0, 0);
}

/*------------------ Map 204 ---------------------------*/

static void M204Sync(void) {
	int32 tmp2 = latch.addr & 0x6;
	int32 tmp1 = tmp2 + ((tmp2 == 0x6) ? 0 : (latch.addr & 1));
	setprg16(0x8000, tmp1);
	setprg16(0xc000, tmp2 + ((tmp2 == 0x6) ? 1 : (latch.addr & 1)));
	setchr8(tmp1);
	setmirror(((latch.addr >> 4) & 1) ^ 1);
}

void Mapper204_Init(CartInfo *info) {
	Latch_Init(info, M204Sync, NULL, 0, 0);
}

/*------------------ Map 212 ---------------------------*/

static DECLFR(M212Read) {
	return X.DB | ((A & 0x10) ? 0 : 0x80);
}

static void M212Sync(void) {
	if (latch.addr & 0x4000) {
		setprg32(0x8000, (latch.addr >> 1) & 3);
	} else {
		setprg16(0x8000, latch.addr & 7);
		setprg16(0xC000, latch.addr & 7);
	}
	setchr8(latch.addr & 7);
	setmirror(((latch.addr >> 3) & 1) ^ 1);
}

static void M212Power() {
	LatchPower();
	SetReadHandler(0x6000, 0x7FFF, M212Read);
}

void Mapper212_Init(CartInfo *info) {
	Latch_Init(info, M212Sync, NULL, 0, 0);
	info->Power = M212Power;
}

/*------------------ Map 214 ---------------------------*/

static void M214Sync(void) {
	setprg16(0x8000, (latch.addr >> 2) & 3);
	setprg16(0xC000, (latch.addr >> 2) & 3);
	setchr8(latch.addr & 3);
}

void Mapper214_Init(CartInfo *info) {
	Latch_Init(info, M214Sync, NULL, 0, 0);
}

/*------------------ Map 217 ---------------------------*/

static void M217Sync(void) {
	setprg32(0x8000, latch.addr >> 2);
	setchr8(latch.addr);
}

void Mapper217_Init(CartInfo *info) {
	Latch_Init(info, M217Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/*------------------ Map 227 ---------------------------*/

static void M227Sync(void) {
	uint32 S = latch.addr & 1;
	uint32 p = ((latch.addr >> 2) & 0x1F) + ((latch.addr & 0x100) >> 3);
	uint32 L = (latch.addr >> 9) & 1;

	if ((latch.addr >> 7) & 1) {
		if (S) {
			setprg32(0x8000, p >> 1);
		} else {
			setprg16(0x8000, p);
			setprg16(0xC000, p);
		}
	} else {
		if (S) {
			if (L) {
				setprg16(0x8000, p & 0x3E);
				setprg16(0xC000, p | 7);
			} else {
				setprg16(0x8000, p & 0x3E);
				setprg16(0xC000, p & 0x38);
			}
		} else {
			if (L) {
				setprg16(0x8000, p);
				setprg16(0xC000, p | 7);
			} else {
				setprg16(0x8000, p);
				setprg16(0xC000, p & 0x38);
			}
		}
	}

	if (!hasBattery && (latch.addr & 0x80) == 0x80)
		/* CHR-RAM write protect hack, needed for some multicarts */
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 0);
	else
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 1);

	setmirror(((latch.addr >> 1) & 1) ^ 1);
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
}

static DECLFR(M227Read) {
	if ((latch.addr & 0x400) && dipswitch) {
		A |= dipswitch;
	}
	return CartBROB(A);
}

static void M227Power() {
	dipswitch = 0;
	LatchPower();
	SetReadHandler(0x8000, 0xFFFF, M227Read);
}

static void M227Reset() {
	LatchHardReset();

	dipswitch = (dipswitch + 1) & 0x0F;
	M227Sync();
}

void Mapper227_Init(CartInfo *info) {
	Latch_Init(info, M227Sync, NULL, 1, 0);
	info->Power = M227Power;
	info->Reset = M227Reset;
	hasBattery = info->battery;
	AddExState(&dipswitch, 1, 0, "PADS");
}

/*------------------ Map 229 ---------------------------*/

static void M229Sync(void) {
	setchr8(latch.addr);
	if (!(latch.addr & 0x1e))
		setprg32(0x8000, 0);
	else {
		setprg16(0x8000, latch.addr & 0x1F);
		setprg16(0xC000, latch.addr & 0x1F);
	}
	setmirror(((latch.addr >> 5) & 1) ^ 1);
}

void Mapper229_Init(CartInfo *info) {
	Latch_Init(info, M229Sync, NULL, 0, 0);
}

/*------------------ Map 231 ---------------------------*/

static void M231Sync(void) {
	setchr8(0);
	if (latch.addr & 0x20)
		setprg32(0x8000, (latch.addr >> 1) & 0x0F);
	else {
		setprg16(0x8000, latch.addr & 0x1E);
		setprg16(0xC000, latch.addr & 0x1E);
	}
	setmirror(((latch.addr >> 7) & 1) ^ 1);
}

void Mapper231_Init(CartInfo *info) {
	Latch_Init(info, M231Sync, NULL, 0, 0);
}

/*------------------ Map 242 ---------------------------*/

static uint8 M242TwoChips;
static void M242Sync(void) {
	uint32 S = latch.addr & 1;
	uint32 p = (latch.addr >> 2) & 0x1F;
	uint32 L = (latch.addr >> 9) & 1;

	if (M242TwoChips) {
		if (latch.addr & 0x600) { /* First chip */
			p &= ((ROM_size & ~8) - 1);
		} else { /* Second chip */
			p &= 0x07;
			p += (ROM_size & ~8);
		}
	}

	if ((latch.addr >> 7) & 1) {
		if (S) {
			setprg32(0x8000, p >> 1);
		} else {
			setprg16(0x8000, p);
			setprg16(0xC000, p);
		}
	} else {
		if (S) {
			if (L) {
				setprg16(0x8000, p & 0x3E);
				setprg16(0xC000, p | 7);
			} else {
				setprg16(0x8000, p & 0x3E);
				setprg16(0xC000, p & 0x38);
			}
		} else {
			if (L) {
				setprg16(0x8000, p);
				setprg16(0xC000, p | 7);
			} else {
				setprg16(0x8000, p);
				setprg16(0xC000, p & 0x38);
			}
		}
	}

	if (!hasBattery && (latch.addr & 0x80) == 0x80 && (ROM_size * 16) > 256)
		/* CHR-RAM write protect hack, needed for some multicarts */
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 0);
	else
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 1);

	setmirror(((latch.addr >> 1) & 1) ^ 1);
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
}

static DECLFR(M242Read) {
	if (latch.addr & 0x100) {
		A |= dipswitch;
	}
	return CartBROB(A);
}

static void M242Power() {
	LatchPower();

	dipswitch = 0;
	SetReadHandler(0x8000, 0xFFFF, M242Read);
}

static void M242Reset() {
	dipswitch = (dipswitch + 1) & 0x1F;
	LatchHardReset();
}

void Mapper242_Init(CartInfo *info) {
	Latch_Init(info, M242Sync, NULL, 1, 0);
	info->Power  = M242Power;
	info->Reset  = M242Reset;
	M242TwoChips = info->PRGRomSize & 0x20000 && info->PRGRomSize > 0x20000;
	hasBattery   = info->battery;
	AddExState(&dipswitch, 1, 0, "PADS");
}

/*------------------ Map 288 ---------------------------*/
/* NES 2.0 Mapper 288 is used for two GKCX1 21-in-1 multicarts
 * - 21-in-1 (GA-003)
 * - 64-in-1 (CF-015)
 */
static void M288Sync(void) {
	setchr8(latch.addr & 7);
	setprg32(0x8000, (latch.addr >> 3) & 3);
}

static DECLFR(M288Read) {
	uint8 ret = CartBR(A);
	if (latch.addr & 0x20)
		ret |= (dipswitch << 2);
	return ret;
}

static void M288Reset(void) {
	dipswitch++;
	dipswitch &= 3;
	M288Sync();
}

void Mapper288_Init(CartInfo *info) {
	dipswitch = 0;
	Latch_Init(info, M288Sync, M288Read, 0, 0);
	info->Reset = M288Reset;
	AddExState(&dipswitch, 1, 0, "DIPSW");
}

/*------------------ Map 385 ---------------------------*/

static void M385Sync(void) {
	setprg16(0x8000, latch.addr >> 1);
	setprg16(0xc000, latch.addr >> 1);
	setmirror((latch.addr & 1) ^ 1);
	setchr8(0);
}

void Mapper385_Init(CartInfo *info) {
	Latch_Init(info, M385Sync, NULL, 0, 1);
	info->Reset = LatchHardReset;
}

/*------------------ Map 541 ---------------------------*/
/* LittleCom 160-in-1 multicart */
static void M541Sync(void) {
	if (latch.addr & 2) {
		/* NROM-128 */
		setprg16(0x8000, latch.addr >> 2);
		setprg16(0xC000, latch.addr >> 2);
	} else {
		/* NROM=256 */
		setprg32(0x8000, latch.addr >> 3);
	}
	setchr8(0);
	setmirror(latch.addr & 1);
}

static void M541Write(uint32 A, uint8 V) {
	if (A >= 0xC000) {
		latch.addr = A;
		M541Sync();
	}
}

static void M541Power() {
	LatchPower();
	SetWriteHandler(0x8000, 0xFFFF, M541Write);
}

void Mapper541_Init(CartInfo *info) {
	Latch_Init(info, M541Sync, NULL, 0, 0);
	info->Power = M541Power;
	info->Reset = LatchHardReset;
}

/*------------------ BMC-190in1 ---------------------------*/
/* NES 2.0 Mapper 300 */

static void BMC190in1Sync(void) {
	setprg16(0x8000, (latch.addr >> 2) & 7);
	setprg16(0xC000, (latch.addr >> 2) & 7);
	setchr8((latch.addr >> 2) & 7);
	setmirror((latch.addr & 1) ^ 1);
}

void BMC190in1_Init(CartInfo *info) {
	Latch_Init(info, BMC190in1Sync, NULL, 0, 0);
}

/*-------------- BMC810544-C-A1 ------------------------*/
/* NES 2.0 Mapper 261 */

static void BMC810544CA1Sync(void) {
	uint32 bank = latch.addr >> 7;
	if (latch.addr & 0x40)
		setprg32(0x8000, bank);
	else {
		setprg16(0x8000, (bank << 1) | ((latch.addr >> 5) & 1));
		setprg16(0xC000, (bank << 1) | ((latch.addr >> 5) & 1));
	}
	setchr8(latch.addr & 0x0f);
	setmirror(((latch.addr >> 4) & 1) ^ 1);
}

void BMC810544CA1_Init(CartInfo *info) {
	Latch_Init(info, BMC810544CA1Sync, NULL, 0, 0);
}

/*-------------- BMCNTD-03 ------------------------*/
/* NES 2.0 Mapper 290 */

static void BMCNTD03Sync(void) {
	/* 1PPP Pmcc spxx xccc
	 * 1000 0000 0000 0000 v
	 * 1001 1100 0000 0100 h
	 * 1011 1010 1100 0100
	 */
	uint32 prg = ((latch.addr >> 10) & 0x1e);
	uint32 chr = ((latch.addr & 0x0300) >> 5) | (latch.addr & 7);
	if (latch.addr & 0x80) {
		setprg16(0x8000, prg | ((latch.addr >> 6) & 1));
		setprg16(0xC000, prg | ((latch.addr >> 6) & 1));
	} else
		setprg32(0x8000, prg >> 1);
	setchr8(chr);
	setmirror(((latch.addr >> 10) & 1) ^ 1);
}

void BMCNTD03_Init(CartInfo *info) {
	Latch_Init(info, BMCNTD03Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/*-------------- BMCG-146 ------------------------*/
/* NES 2.0 Mapper 349 */

static void BMCG146Sync(void) {
	setchr8(0);
	if (latch.addr & 0x800) { /* UNROM mode */
		setprg16(0x8000, (latch.addr & 0x1F) | (latch.addr & ((latch.addr & 0x40) >> 6)));
		setprg16(0xC000, (latch.addr & 0x18) | 7);
	} else {
		if (latch.addr & 0x40) { /* 16K mode */
			setprg16(0x8000, latch.addr & 0x1F);
			setprg16(0xC000, latch.addr & 0x1F);
		} else {
			setprg32(0x8000, (latch.addr >> 1) & 0x0F);
		}
	}
	setmirror(((latch.addr & 0x80) >> 7) ^ 1);
}

void BMCG146_Init(CartInfo *info) {
	Latch_Init(info, BMCG146Sync, NULL, 0, 0);
}

/*-------------- BMC-TJ-03 ------------------------*/
/* NES 2.0 mapper 341 is used for a simple 4-in-1 multicart */

static void BMCTJ03Sync(void) {
	uint8 mirr = (latch.addr & ((PRGsize[0] & 0x40000) ? 0x800 : 0x200)) ? MI_H : MI_V;
	setprg32(0x8000, latch.addr >> 8);
	setchr8(latch.addr >> 8);
	setmirror(mirr);
}

void BMCTJ03_Init(CartInfo *info) {
	Latch_Init(info, BMCTJ03Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/*-------------- BMC-SA005-A ------------------------*/
/* NES 2.0 mapper 338 is used for a 16-in-1 and a 200/300/600/1000-in-1 multicart.
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_338 */

static void BMCSA005ASync(void) {
	setprg16(0x8000, latch.addr & 0x0F);
	setprg16(0xC000, latch.addr & 0x0F);
	setchr8(latch.addr & 0x0F);
	setmirror((latch.addr >> 3) & 1);
}

void BMCSA005A_Init(CartInfo *info) {
	Latch_Init(info, BMCSA005ASync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/* -------------- 831019C J-2282 ------------------------ */
/* NES 2.0 Mapper 402 */

static void J2282Sync(void) {
	if (latch.addr & 0x800) {
		setprg8(0x6000, ((latch.addr & 0x1F) << 1) | 3);
	}
	if ((latch.addr & 0x40)) {
		setprg16(0x8000, latch.addr & 0x1F);
		setprg16(0xC000, latch.addr & 0x1F);
	} else {
		setprg32(0x8000, (latch.addr & 0x1F) >> 1);
	}
	if ((latch.addr & 0x400) == 0) {
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 0);
	} else {
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 1);
	}
	setchr8(0);
	setmirror(((latch.addr >> 7) & 1) ^ 1);
}

void J2282_Init(CartInfo *info) {
	Latch_Init(info, J2282Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/* -------------- Mapper 409 ------------------------ */
static void M409Sync(void) {
	setprg16(0x8000, latch.addr);
	setprg16(0xC000, ~0);
	setchr8(0);
}

void Mapper409_Init(CartInfo *info) {
	Latch_Init(info, M409Sync, NULL, 0, 0);
}

/*------------------ Map 435 ---------------------------*/
static void M435Sync(void) {
	int p = ((latch.addr >> 2) & 0x1F) | ((latch.addr >> 3) & 0x20) | ((latch.addr >> 4) & 0x40);
	if (latch.addr & 0x200) {
		if (latch.addr & 0x001) {
			setprg16(0x8000, p);
			setprg16(0xC000, p);
		} else {
			setprg32(0x8000, p >> 1);
		}
	} else {
		setprg16(0x8000, p);
		setprg16(0xC000, p | 7);
	}

	if (latch.addr & 0x800)
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 0);
	else
		SetupCartCHRMapping(0, CHRptr[0], 0x2000, 1);

	setmirror(latch.addr & 0x002 ? MI_H : MI_V);
	setchr8(0);
}

void Mapper435_Init(CartInfo *info) {
	Latch_Init(info, M435Sync, NULL, 1, 0);
}
