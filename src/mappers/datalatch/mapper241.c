/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 *
 */

/* Map 241 */
/* Mapper 7 mostly, but with SRAM or maybe prot circuit.
 * figure out, which games do need 5xxx area reading.
 */

#include "mapinc.h"
#include "latch.h"

static void Sync(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg32(0x8000, latch.data);
    setchr8(0);
}

void Mapper241_Init(CartInfo *info) {
	Latch_Init(info, Sync, NULL, 1, 0);
	info->Reset = Latch_RegReset;
}
