/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2025-2026 negativeExponent
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
#include "n118.h"

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, V & 0x0F);
}

static DECLFW(M486Write) {
	n118.cmd = A & 0x07;
	n118.reg[n118.cmd] = V;
    N118_SyncPRG();
    N118_SyncCHR();
}

static void Power(void) {
	N118_Power();
	SetWriteHandler(0x8000, 0x9FFF, M486Write);
}

void Mapper486_Init(CartInfo *info) {
	N118_Init(info, 0, 0);
	N118_pwrap = SetPRG;
	info->Power = Power;
}
