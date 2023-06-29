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
#include "fdssound.h"

static uint8 submapper = 0;

/*------------------ Map 2 ---------------------------*/

static void M002Sync(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg16(0x8000, latch.data);
	setprg16(0xc000, ~0);
	setchr8(0);
}

void Mapper002_Init(CartInfo *info) {
	/* By default, do not emulate bus conflicts except when explicitly told by a NES 2.0 header to do so. */
	Latch_Init(info, M002Sync, NULL, 1, info->iNES2 && info->submapper == 2);
}

/*------------------ Map 3 ---------------------------*/

static void M003Sync(void) {
	setchr8(latch.data);
	setprg32(0x8000, 0);
	setprg8r(0x10, 0x6000, 0); /* Hayauchy IGO uses 2Kb or RAM */
}

void Mapper003_Init(CartInfo *info) {
	/* By default, do not emulate bus conflicts except when explicitly told by a NES 2.0 header to do so. */
	Latch_Init(info, M003Sync, NULL, 1, info->iNES2 && info->submapper == 2);
}

/*------------------ Map 7 ---------------------------*/

static void M007Sync(void) {
	setprg32(0x8000, latch.data & 0xF);
	setmirror(MI_0 + ((latch.data >> 4) & 1));
	setchr8(0);
}

void Mapper007_Init(CartInfo *info) {
	Latch_Init(info, M007Sync, NULL, 0, info->iNES2 && info->submapper == 2);
}

/*------------------ Map 8 ---------------------------*/

static void M008Sync(void) {
	setprg16(0x8000, latch.data >> 3);
	setprg16(0xc000, 1);
	setchr8(latch.data & 3);
}

void Mapper008_Init(CartInfo *info) {
	Latch_Init(info, M008Sync, NULL, 0, 0);
}

/*------------------ Map 11  ---------------------------*/

static void M011Sync(void) {
	setprg32(0x8000, latch.data & 0xF);
	setchr8(latch.data >> 4);
}

void Mapper011_Init(CartInfo *info) {
	Latch_Init(info, M011Sync, NULL, 0, 0);
}

/*------------------ Map 144 ---------------------------*/

static DECLFW(M144Write) {
    uint8 data = CartBR(A & (A | 1));
	LatchWrite(A, data);
}

static void M144Power() {
	LatchPower();
	SetWriteHandler(0x8000, 0xFFFF, M144Write);
}

void Mapper144_Init(CartInfo *info) {
	Latch_Init(info, M011Sync, NULL, 0, 1);
    info->Power = M144Power;
}

/*------------------ Map 13 ---------------------------*/

static void M013Sync(void) {
	setchr4(0x0000, 0);
	setchr4(0x1000, latch.data & 3);
	setprg32(0x8000, 0);
}

void Mapper013_Init(CartInfo *info) {
	Latch_Init(info, M013Sync, NULL, 0, 1);
}

/*------------------ Map 29 ---------------------------*/
/* added 2019-5-23
 * Mapper 28, used by homebrew game Glider
 * https://wiki.nesdev.com/w/index.php/INES_Mapper_029 */

static void M029Sync(void) {
	setprg16(0x8000, (latch.data >> 2) & 7);
	setprg16(0xc000, ~0);
	setchr8r(0, latch.data & 3);
	setprg8r(0x10, 0x6000, 0);
}

void Mapper029_Init(CartInfo *info) {
	Latch_Init(info, M029Sync, NULL, 1, 0);
}

/*------------------ Map 66 ---------------------------*/

static void MM066Sync(void) {
	setprg32(0x8000, latch.data >> 4);
	setchr8(latch.data & 0xF);
}

void Mapper066_Init(CartInfo *info) {
	Latch_Init(info, MM066Sync, NULL, 0, 1);
}

/*------------------ Map 70 ---------------------------*/

static void M070Sync(void) {
	setprg16(0x8000, latch.data >> 4);
	setprg16(0xc000, ~0);
	setchr8(latch.data & 0xf);
}

void Mapper070_Init(CartInfo *info) {
	Latch_Init(info, M070Sync, NULL, 0, 1);
}

/*------------------ Map 78 ---------------------------*/
/* Should be two separate emulation functions for this "mapper".  Sigh.  URGE TO KILL RISING. */
/* Submapper 1 - Uchuusen - Cosmo Carrier ( one-screen mirroring ) */
/* Submapper 3 - Holy Diver ( horizontal/vertical mirroring ) */
static void M078Sync(void) {
	setprg16(0x8000, (latch.data & 7));
	setprg16(0xc000, ~0);
	setchr8(latch.data >> 4);
	if (submapper == 3) {
		setmirror((latch.data >> 3) & 1);
	} else {
		setmirror(MI_0 + ((latch.data >> 3) & 1));
	}
}

void Mapper078_Init(CartInfo *info) {
	Latch_Init(info, M078Sync, NULL, 0, 1);
	submapper = info->iNES2 ? info->submapper : 0;
}

/*------------------ Map 89 ---------------------------*/

static void M089Sync(void) {
	setprg16(0x8000, (latch.data >> 4) & 7);
	setprg16(0xc000, ~0);
	setchr8((latch.data & 7) | ((latch.data >> 4) & 8));
	setmirror(MI_0 + ((latch.data >> 3) & 1));
}

void Mapper089_Init(CartInfo *info) {
	Latch_Init(info, M089Sync, NULL, 0, 1);
}

/*------------------ Map 93 ---------------------------*/

static void M093Sync(void) {
	setprg16(0x8000, latch.data >> 4);
	setprg16(0xc000, ~0);
	setchr8(0);
}

void Mapper093_Init(CartInfo *info) {
	Latch_Init(info, M093Sync, NULL, 0, 1);
}

/*------------------ Map 94 ---------------------------*/

static void M094Sync(void) {
	setprg16(0x8000, latch.data >> 2);
	setprg16(0xc000, ~0);
	setchr8(0);
}

void Mapper094_Init(CartInfo *info) {
	Latch_Init(info, M094Sync, NULL, 0, 1);
}

/*------------------ Map 97 ---------------------------*/

static void M097Sync(void) {
	setchr8(0);
	setprg16(0x8000, ~0);
	setprg16(0xc000, latch.data);
	setmirror((latch.data >> 7) & 1);
}

void Mapper097_Init(CartInfo *info) {
	Latch_Init(info, M097Sync, NULL, 0, 0);
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
	Latch_Init(info, M152Sync, NULL, 0, 1);
}

/*------------------ Map 180 ---------------------------*/

static void M180Sync(void) {
	setprg16(0x8000, 0);
	setprg16(0xc000, latch.data);
	setchr8(0);
}

void Mapper180_Init(CartInfo *info) {
	Latch_Init(info, M180Sync, NULL, 0, 1);
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
	setprg32(0x8000, latch.data);
}

void Mapper241_Init(CartInfo *info) {
	Latch_Init(info, M241Sync, NULL, 1, 0);
	info->Reset = LatchHardReset;
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
static uint8 M538Banks[16] = { 0, 1, 2, 1, 3, 1, 4, 1, 5, 5, 1, 1, 6, 6, 7, 7 };
static void M538Sync_wrong_prg_order(void) {
	setprg8(0x6000, (latch.data >> 1) | 8);
	setprg8(0x8000, M538Banks[latch.data & 15]);
	setprg8(0xA000, 14);
	setprg8(0xC000, 7);
	setprg8(0xE000, 15);
	setchr8(0);
	setmirror(1);
}

static void M538Sync(void) {
	setprg8(0x6000, latch.data | 1);
	setprg8(0x8000, (latch.data & 1) && (~latch.data & 8) ? 10 : (latch.data & ~1));
	setprg8(0xA000, 13);
	setprg8(0xC000, 14);
	setprg8(0xE000, 15);
	setchr8(0);
	setmirror(MI_V);
}

static DECLFW(M538Write) {
	LatchWrite(A, V);
}

static void M538Power(void) {
	FDSSoundPower();
	LatchHardReset();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0xC000, 0xDFFF, M538Write);
}

void Mapper538_Init(CartInfo *info) {
	if (info->CRC32 == 0xA8C6D77D) {
		Latch_Init(info, M538Sync_wrong_prg_order, NULL, 0, 0);
	}
	Latch_Init(info, M538Sync, NULL, 0, 0);
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
static void M285Sync(void) {
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

void Mapper285_Init(CartInfo *info) {
	A65ASsubmapper = info->submapper;
	Latch_Init(info, M285Sync, NULL, 0, 0);
}

/*------------------ BMC-11160 (m299) ---------------------------*/
/* Simple BMC discrete mapper by TXC */

static void M299Sync(void) {
	uint32 bank = (latch.data >> 4) & 7;
	setprg32(0x8000, bank);
	setchr8((bank << 2) | (latch.data & 3));
	setmirror((latch.data >> 7) & 1);
}

void Mapper299_Init(CartInfo *info) {
	Latch_Init(info, M299Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
}

/*------------------ BMC-K-3046 ---------------------------*/
/* NES 2.0 mapper 336 is used for an 11-in-1 multicart
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_336 */

static void M336Sync(void) {
	setprg16(0x8000, latch.data);
	setprg16(0xC000, latch.data | 0x07);
	setchr8(0);
}

void Mapper336_Init(CartInfo *info) {
	Latch_Init(info, M336Sync, NULL, 0, 0);
	info->Reset = LatchHardReset;
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
