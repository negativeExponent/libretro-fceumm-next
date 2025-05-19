/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024 negativeExponent
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

/* NES 2.0 Mapper 450 - YY841157C
 */

#include "mapinc.h"
#include "vrc24.h"

static void M450PW(uint16 A, uint16 V) {
	setprg8(A, (vrc24.wire << 4) | (V & 0x0F));
}

static void M450CW(uint16 A, uint16 V) {
	setchr1(A, (vrc24.wire << 7) | (V & 0x7F));
}

static void M450SyncWire(void) {
	VRC24_SyncPRG();
	VRC24_SyncCHR();
}

static void M450Reset(void) {
	vrc24.wire = 0;
	VRC24_SyncPRG();
	VRC24_SyncCHR();
	VRC24_SyncMirror();
}

void Mapper450_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, 0, 1);
	VRC24_pwrap = M450PW;
	VRC24_cwrap = M450CW;
	VRC24_SyncWires = M450SyncWire;
	info->Reset = M450Reset;
}
