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
 */

#include "mapinc.h"
#include "mmc3.h"

static void SyncMirror(void) {
	setmirrorw(
		MMC3_GetCHRBank(0) >> 7,
		MMC3_GetCHRBank(1) >> 7,
		MMC3_GetCHRBank(2) >> 7,
		MMC3_GetCHRBank(3) >> 7);
}

static DECLFW(WriteMMC3) {
	switch (A & 0xE001) {
	case 0x8001:
		switch (mmc3.cmd & 0x07) {
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			mmc3.reg[mmc3.cmd & 0x07] = V;
			MMC3_SyncCHR();
			MMC3_SyncMirror();
			break;
		default:
			MMC3_Write(A, V);
			break;
		}
		break;
	default:
		MMC3_Write(A, V);
		break;
	}
}

static void Power(void) {
	MMC3_Power();
	SetWriteHandler(0x8000, 0xBFFF, WriteMMC3);
}

void Mapper118_Init(CartInfo *info) {
	uint8_t ws = info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) / 1024 : 8;
	MMC3_Init(info, MMC3B, ws, info->battery);
	info->Power = Power;
	MMC3_SyncMirror = SyncMirror;
}
