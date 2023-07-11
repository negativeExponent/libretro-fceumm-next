/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2008 CaH4e3
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 287
 * BMC-411120-C, actually cart ID is 811120-C, sorry ;) K-3094 - another ID
 * - 4-in-1 (411120-C)
 * - 4-in-1 (811120-C,411120-C) [p4][U]
 *
 * BMC-K-3088, similar to BMC-411120-C but without jumper or dipswitch
 * - 19-in-1(K-3088)(810849-C)(Unl)
 */

/* 2023-03-02
 - use PRG size to determine variant
 - remove forced mask for outer-bank (rely on internal mask set during PRG/CHR mapping)
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 reg;
static uint8 dipsw = 0;

static void M287CW(uint32 A, uint8 V) {
	setchr1(A, (V & 0x7F) | ((reg & 0x07) << 7));
}

static void M287PW(uint32 A, uint8 V) {
	if (reg & (8 | (dipsw && ((ROM.prg.size * 16) <= 512) ? 4 : 0))) {
		/* 32K Mode */
		setprg32(0x8000, ((reg << 2) & 0x1C) | ((reg >> 4) & 0x03));
		/* FCEU_printf("32K mode: bank:%02x\n", ((reg >> 4) & 3) | ((reg & 7) << 2)); */
	} else {
		/* MMC3 Mode */
		setprg8(A, ((reg & 7) << 4) | (V & 0x0F));
		/* FCEU_printf("MMC3: %04x:%02x\n", A, (V & 0x0F) | ((reg & 7) << 4)); */
	}
}

static DECLFW(M287LoWrite) {
	/*	printf("Wr: A:%04x V:%02x\n", A, V); */
	if (MMC3_WramIsWritable()) {
		reg = A;
		MMC3_FixPRG();
		MMC3_FixCHR();
	}
}

static void M287Reset(void) {
	reg = 0;
	dipsw ^= 4;
	MMC3_Reset();
}

static void M287Power(void) {
	reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, M287LoWrite);
}

void Mapper287_Init(CartInfo *info) {
	MMC3_Init(info, 0, 0);
	MMC3_pwrap = M287PW;
	MMC3_cwrap = M287CW;
	info->Power = M287Power;
	info->Reset = M287Reset;
	AddExState(&reg, 1, 0, "EXPR");
}
