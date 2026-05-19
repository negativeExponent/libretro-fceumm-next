/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024-2026 negativeExponent
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

#include "mapinc.h"
#include "txc.h"

static void Sync(void) {
	setprg32(0x8000, (txc.output >> 2) & 0x01);
	setchr8(txc.output & 0x03);
}

static DECLFR(ReadTXC) {
	return ((cpu.openbus & 0xF0) | (TXC_Read(A) & 0x0F));
}

static DECLFW(WriteTXC) {
	TXC_Write(A, V & 0x0F);
}

static void Power(void) {
	TXC_Power();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetReadHandler(0x4100, 0x5FFF, ReadTXC);
	SetWriteHandler(0x4100, 0xFFFF, WriteTXC);
}

void Mapper132_Init(CartInfo *info) {
	if (info->CRC32 == 0x2A5F4C5A) {
		/* Jin Gwok Sei Chuen Saang (Ch) [U][!] */
		FCEU_printf(" WARNING: Using alternate mapper implementation.\n");
		UNL22211_Init(info);
	} else {
		TXC_Init(info, Sync);
		info->Power = Power;
	}
}
