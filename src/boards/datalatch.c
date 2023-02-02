/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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
#include "../fds_apu.h"

static uint8 submapper = 0;

/*------------------ Map 2 ---------------------------*/

static void UNROMSync(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg16(0x8000, latch.data);
	setprg16(0xc000, ~0);
	setchr8(0);
}

void UNROM_Init(CartInfo *info) {
	/* By default, do not emulate bus conflicts except when explicitly told by a NES 2.0 header to do so. */
	Latch_Init(info, UNROMSync, NULL, 1, info->iNES2 && info->submapper == 2);
}

/*------------------ Map 3 ---------------------------*/

static void CNROMSync(void) {
	setchr8(latch.data);
	setprg32(0x8000, 0);
	setprg8r(0x10, 0x6000, 0); /* Hayauchy IGO uses 2Kb or RAM */
}

void CNROM_Init(CartInfo *info) {
	/* By default, do not emulate bus conflicts except when explicitly told by a NES 2.0 header to do so. */
	Latch_Init(info, CNROMSync, NULL, 1, info->iNES2 && info->submapper == 2);
}

/*------------------ Map 7 ---------------------------*/

static void ANROMSync(void) {
	setprg32(0x8000, latch.data & 0xF);
	setmirror(MI_0 + ((latch.data >> 4) & 1));
	setchr8(0);
}

void ANROM_Init(CartInfo *info) {
	Latch_Init(info, ANROMSync, NULL, 0, info->iNES2 && info->submapper == 2);
}

/*------------------ Map 8 ---------------------------*/

static void M8Sync(void) {
	setprg16(0x8000, latch.data >> 3);
	setprg16(0xc000, 1);
	setchr8(latch.data & 3);
}

void Mapper8_Init(CartInfo *info) {
	Latch_Init(info, M8Sync, NULL, 0, 0);
}

/*------------------ Map 11 / 144 ---------------------------*/

static void M11Sync(void) {
	setprg32(0x8000, latch.data & 0xF);
	setchr8(latch.data >> 4);
}

void Mapper11_Init(CartInfo *info) {
	Latch_Init(info, M11Sync, NULL, 0, 0);
}

static DECLFW(M144Write) {
    uint8 data = CartBR(A & (A | 1));
	LatchWrite(A, data);
}

static void M144Power() {
	LatchPower();
	SetWriteHandler(0x8000, 0xFFFF, M144Write);
}

void Mapper144_Init(CartInfo *info) {
	Latch_Init(info, M11Sync, NULL, 0, 1);
    info->Power = M144Power;
}

/*------------------ Map 13 ---------------------------*/

static void CPROMSync(void) {
	setchr4(0x0000, 0);
	setchr4(0x1000, latch.data & 3);
	setprg32(0x8000, 0);
}

void CPROM_Init(CartInfo *info) {
	Latch_Init(info, CPROMSync, NULL, 0, 1);
}

/*------------------ Map 29 ---------------------------*/
/* added 2019-5-23
 * Mapper 28, used by homebrew game Glider
 * https://wiki.nesdev.com/w/index.php/INES_Mapper_029 */

static void M29Sync(void) {
	setprg16(0x8000, (latch.data >> 2) & 7);
	setprg16(0xc000, ~0);
	setchr8r(0, latch.data & 3);
	setprg8r(0x10, 0x6000, 0);
}

void Mapper29_Init(CartInfo *info) {
	Latch_Init(info, M29Sync, NULL, 1, 0);
}

/*------------------ Map 66 ---------------------------*/

static void MHROMSync(void) {
	setprg32(0x8000, latch.data >> 4);
	setchr8(latch.data & 0xF);
}

void MHROM_Init(CartInfo *info) {
	Latch_Init(info, MHROMSync, NULL, 0, 1);
}

/*------------------ Map 70 ---------------------------*/

static void M70Sync(void) {
	setprg16(0x8000, latch.data >> 4);
	setprg16(0xc000, ~0);
	setchr8(latch.data & 0xf);
}

void Mapper70_Init(CartInfo *info) {
	Latch_Init(info, M70Sync, NULL, 0, 1);
}

/*------------------ Map 78 ---------------------------*/
/* Should be two separate emulation functions for this "mapper".  Sigh.  URGE TO KILL RISING. */
/* Submapper 1 - Uchuusen - Cosmo Carrier ( one-screen mirroring ) */
/* Submapper 3 - Holy Diver ( horizontal/vertical mirroring ) */
static void M78Sync(void) {
	setprg16(0x8000, (latch.data & 7));
	setprg16(0xc000, ~0);
	setchr8(latch.data >> 4);
	if (submapper == 3) {
		setmirror((latch.data >> 3) & 1);
	} else {
		setmirror(MI_0 + ((latch.data >> 3) & 1));
	}
}

void Mapper78_Init(CartInfo *info) {
	Latch_Init(info, M78Sync, NULL, 0, 1);
	submapper = info->iNES2 ? info->submapper : 0;
}

/*------------------ Map 89 ---------------------------*/

static void M89Sync(void) {
	setprg16(0x8000, (latch.data >> 4) & 7);
	setprg16(0xc000, ~0);
	setchr8((latch.data & 7) | ((latch.data >> 4) & 8));
	setmirror(MI_0 + ((latch.data >> 3) & 1));
}

void Mapper89_Init(CartInfo *info) {
	Latch_Init(info, M89Sync, NULL, 0, 0);
}

/*------------------ Map 93 ---------------------------*/

static void SSUNROMSync(void) {
	setprg16(0x8000, latch.data >> 4);
	setprg16(0xc000, ~0);
	setchr8(0);
}

void SUNSOFT_UNROM_Init(CartInfo *info) {
	Latch_Init(info, SSUNROMSync, NULL, 0, 0);
}

/*------------------ Map 94 ---------------------------*/

static void M94Sync(void) {
	setprg16(0x8000, latch.data >> 2);
	setprg16(0xc000, ~0);
	setchr8(0);
}

void Mapper94_Init(CartInfo *info) {
	Latch_Init(info, M94Sync, NULL, 0, 0);
}

/*------------------ Map 97 ---------------------------*/

static void M97Sync(void) {
	setchr8(0);
	setprg16(0x8000, ~0);
	setprg16(0xc000, latch.data & 15);
	switch (latch.data >> 6) {
		case 0:
			break;
		case 1:
			setmirror(MI_H);
			break;
		case 2:
			setmirror(MI_V);
			break;
		case 3:
			break;
	}
	setchr8(((latch.data >> 1) & 1) | ((latch.data << 1) & 2));
}

void Mapper97_Init(CartInfo *info) {
	Latch_Init(info, M97Sync, NULL, 0, 0);
}

/*------------------ Map 107 ---------------------------*/

static void M107Sync(void) {
	setprg32(0x8000, latch.data >> 1);
	setchr8(latch.data);
}

void Mapper107_Init(CartInfo *info) {
	Latch_Init(info, M107Sync, NULL, 0, 0);
}

/*------------------ Map 152 ---------------------------*/

static void M152Sync(void) {
	setprg16(0x8000, (latch.data >> 4) & 7);
	setprg16(0xc000, ~0);
	setchr8(latch.data & 0xf);
	setmirror(MI_0 + ((latch.data >> 7) & 1)); /* Saint Seiya...hmm. */
}

void Mapper152_Init(CartInfo *info) {
	Latch_Init(info, M152Sync, NULL, 0, 0);
}

/*------------------ Map 180 ---------------------------*/

static void M180Sync(void) {
	setprg16(0x8000, 0);
	setprg16(0xc000, latch.data);
	setchr8(0);
}

void Mapper180_Init(CartInfo *info) {
	Latch_Init(info, M180Sync, NULL, 0, 0);
}

/*------------------ Map 203 ---------------------------*/

static void M203Sync(void) {
	setprg16(0x8000, latch.data >> 2);
	setprg16(0xC000, latch.data >> 2);
	setchr8(latch.data & 3);
}

void Mapper203_Init(CartInfo *info) {
	Latch_Init(info, M203Sync, NULL, 0, 0);
}

/*------------------ Map 241 ---------------------------*/
/* Mapper 7 mostly, but with SRAM or maybe prot circuit.
 * figure out, which games do need 5xxx area reading.
 */

static void M241Sync(void) {
	setchr8(0);
	setprg8r(0x10, 0x6000, 0);
	if (latch.data & 0x80)
		setprg32(0x8000, latch.data | 8); /* no 241 actually, but why not afterall? */
	else
		setprg32(0x8000, latch.data);
}

void Mapper241_Init(CartInfo *info) {
	Latch_Init(info, M241Sync, NULL, 1, 0);
}

/*------------------ Map 271 ---------------------------*/

static void M271Sync(void) {
	setchr8(latch.data & 0x0F);
	setprg32(0x8000, latch.data >> 4);
	setmirror((latch.data >> 5) & 1);
}

void Mapper271_Init(CartInfo *info) {
	Latch_Init(info, M271Sync, NULL, 0, 0);
}

/*------------------ Map 381 ---------------------------*/
/* 2-in-1 High Standard Game (BC-019), reset-based */
static uint8 reset = 0;
static void M381Sync(void) {
	setprg16(0x8000, ((latch.data & 0x10) >> 4) | ((latch.data & 7) << 1) | (reset << 4));
	setprg16(0xC000, 15 | (reset << 4));
	setchr8(0);
}

static void M381Reset(void) {
	reset ^= 1;
	M381Sync();
}

void Mapper381_Init(CartInfo *info) {
	info->Reset = M381Reset;
	Latch_Init(info, M381Sync, NULL, 0, 0);
	AddExState(&reset, 1, 0, "RST0");
}

/*------------------ Map 538 ---------------------------*/
/* NES 2.0 Mapper 538 denotes the 60-1064-16L PCB, used for a
 * bootleg cartridge conversion named Super Soccer Champion
 * of the Konami FDS game Exciting Soccer.
 */

/* this code emulates rom dump with wrong bank order */
/*static uint8 M538Banks[16] = { 0, 1, 2, 1, 3, 1, 4, 1, 5, 5, 1, 1, 6, 6, 7, 7 };
static void M538Sync(void) {
	setprg8(0x6000, (latch.data >> 1) | 8);
	setprg8(0x8000, M538Banks[latch.data & 15]);
	setprg8(0xA000, 14);
	setprg8(0xC000, 7);
	setprg8(0xE000, 15);
	setchr8(0);
	setmirror(1);
}

static void M538Power(void) {
	FDSSoundPower();
	LatchPower();
}

void Mapper538_Init(CartInfo *info) {
	Latch_Init(info, M538Sync, 0, 0xC000, 0xCFFF, 1, 0);
	info->Power = M538Power;
}*/

static void M538Sync(void) {
	setprg8(0x6000, latch.data | 8);
	setprg8(0x8000, (latch.data & 1) && ((~latch.data & 8) ? 10 : (latch.data & ~1)));
	setprg8(0xA000, 13);
	setprg8(0xC000, 14);
	setprg8(0xE000, 15);
	setchr8(0);
	setmirror(1);
}

static void M538Power(void) {
	FDSSoundPower();
	LatchPower();
}

void Mapper538_Init(CartInfo *info) {
	Latch_Init(info, M538Sync, NULL, 1, 0);
	info->Power = M538Power;
}

/* ------------------ A65AS (m285) --------------------------- */

/* actually, there is two cart in one... First have extra mirroring
 * mode (one screen) and 32K bankswitching, second one have only
 * 16 bankswitching mode and normal mirroring... But there is no any
 * correlations between modes and they can be used in one mapper code.
 *
 * Submapper 0 - 3-in-1 (N068)
 * Submapper 0 - 3-in-1 (N080)
 * Submapper 1 - 4-in-1 (JY-066)
 */

static int A65ASsubmapper;
static void BMCA65ASSync(void) {
	if (latch.data & 0x40)
		setprg32(0x8000, (latch.data >> 1) & 0x0F);
	else {
		if (A65ASsubmapper == 1) {
			setprg16(0x8000, ((latch.data & 0x30) >> 1) | (latch.data & 7));
			setprg16(0xC000, ((latch.data & 0x30) >> 1) | 7);
		} else {
			setprg16(0x8000, latch.data & 0x0F);
			setprg16(0xC000, (latch.data & 0x0F) | 7);
		}
	}
	setchr8(0);
	if (latch.data & 0x80)
		setmirror(MI_0 + (((latch.data >> 5) & 1)));
	else
		setmirror(((latch.data >> 3) & 1) ^ 1);
}

void BMCA65AS_Init(CartInfo *info) {
	A65ASsubmapper = info->submapper;
	Latch_Init(info, BMCA65ASSync, NULL, 0, 0);
}

/*------------------ BMC-11160 (m299) ---------------------------*/
/* Simple BMC discrete mapper by TXC */

static void BMC11160Sync(void) {
	uint32 bank = (latch.data >> 4) & 7;
	setprg32(0x8000, bank);
	setchr8((bank << 2) | (latch.data & 3));
	setmirror((latch.data >> 7) & 1);
}

void BMC11160_Init(CartInfo *info) {
	Latch_Init(info, BMC11160Sync, NULL, 0, 0);
}

/*------------------ BMC-K-3046 ---------------------------*/
/* NES 2.0 mapper 336 is used for an 11-in-1 multicart
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_336 */

static void BMCK3046Sync(void) {
	setprg16(0x8000, latch.data);
	setprg16(0xC000, latch.data | 0x07);
	setchr8(0);
}

void BMCK3046_Init(CartInfo *info) {
	Latch_Init(info, BMCK3046Sync, NULL, 0, 0);
}

/*------ Mapper 429: LIKO BBG-235-8-1B/Milowork FCFC1 ----*/

static void Mapper429_Sync(void) {
	setprg32(0x8000, latch.data >> 2);
	setchr8(latch.data);
}

static void Mapper429_Reset(void) {
	latch.data = 4; /* Initial CHR bank 0, initial PRG bank 1 */
	Mapper429_Sync();
}

void Mapper429_Init(CartInfo *info) {
	Latch_Init(info, Mapper429_Sync, NULL, 0, 0);
	info->Reset = Mapper429_Reset;
}

/*------------------ Mapper 415 ---------------------------*/

static void Mapper415_Sync(void) {
	setprg8(0x6000, latch.data & 0x0F);
	setprg32(0x8000, ~0);
	setchr8(0);
	setmirror(((latch.data >> 4) & 1) ^ 1);
}

static void M415Power(void) {
	LatchPower();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
}

void Mapper415_Init(CartInfo *info) {
	Latch_Init(info, Mapper415_Sync, NULL, 0, 0);
	info->Power = M415Power;
}
