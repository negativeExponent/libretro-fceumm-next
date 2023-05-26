/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 527 is used for a bootleg version of
 * Taito's 不動明王伝 (Fudō Myōō Den).
 * Its UNIF board name is UNL-AX-40G. The original INES Mapper 207 is
 * replaced with a VRC2 clone (A0/A1, i.e. VRC2b) while retaining
 * Mapper 207's extended mirroring.
 */

#include "mapinc.h"
#include "vrc24.h"

static void M527CW(uint32 A, uint32 V) {
	setchr1(A, V);
	setmirrorw((vrc24.chrhi[0] >> 3) & 1, (vrc24.chrhi[0] >> 3) & 1, (vrc24.chrhi[1] >> 3) & 1, (vrc24.chrhi[1] >> 3) & 1);
}

void Mapper527_Init(CartInfo *info) {
	GenVRC24_Init(info, VRC2b, 0);
	vrc24.cwrap = M527CW;
}
