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
 * NES 2.0 Mapper 302 - UNL-KS7057
 * FDS Conversion
 * Gyruss (KS-7057)
 */

#include "mapinc.h"
#include "vrc24.h"

static void SyncPRG(void) {
	setprg8( 0x8000, 0x00);
	setprg8( 0xA000, 0x0D);
	setprg16(0xC000, 0x07);
}

static void SyncCHR(void) {
	setprg2(0x6000, vrc24.chr[4]);
	setprg2(0x6800, vrc24.chr[5]);
	setprg2(0x7000, vrc24.chr[6]);
	setprg2(0x7800, vrc24.chr[7]);

	setprg2(0x8000, vrc24.chr[0]);
	setprg2(0x8800, vrc24.chr[1]);
	setprg2(0x9000, vrc24.chr[2]);
	setprg2(0x9800, vrc24.chr[3]);

	setchr8(0);
}

static void SyncMirror(void) {
	setmirror(vrc24.mirr & 0x01);
}

static void Power(void) {
	VRC24_Power();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
}

void Mapper302_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, FALSE, TRUE);
	info->Power = Power;
	VRC24_SyncPRG = SyncPRG;
	VRC24_SyncCHR = SyncCHR;
	VRC24_SyncMirror = SyncMirror;
}
