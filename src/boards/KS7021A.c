/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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

/* -------------------- UNL-KS7021A -------------------- */
/* http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_525
 * NES 2.0 Mapper 525 is used for a bootleg version of versions of Contra and 月風魔伝 (Getsu Fūma Den).
 * Its similar to Mapper 23 Submapper 3) with non-nibblized CHR-ROM bank registers.
 */

#include "mapinc.h"
#include "vrc24.h"

static DECLFW(KS7021AWrite) {
	switch (A & 0xB000) {
	case 0xB000:
		vrc24.chrreg[A & 0x07] = V;
		FixVRC24CHR();
		break;
	}
}

static void KS7021APower(void) {
	GenVRC24Power();
	SetWriteHandler(0xB000, 0xBFFF, KS7021AWrite);
}

void UNLKS7021A_Init(CartInfo *info) {
	GenVRC24_Init(info, VRC2b, 1);
	info->Power = KS7021APower;
}
