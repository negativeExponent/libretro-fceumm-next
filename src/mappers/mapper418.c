/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 */

/* NES 2.0 Mapper 418 denotes the 820106-C/821007C circuit boards for the LH42 bootleg cartridge versions of Highway Star. */

#include "mapinc.h"
#include "n118.h"

static void SetCHR(void) {
	setchr8(0);
	setmirror((n118.reg[5] & 0x01) ^ 0x01);
}

void Mapper418_Init(CartInfo *info) {
	N118_Init(info, 0, 0);
	N118_SyncCHR = SetCHR;
}
