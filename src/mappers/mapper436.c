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

/* NES 2.0 Mapper 436 denotes the ZLX-08 plug-and-play VT02 console PCB, used by
 * the Entertainment System 620-in-1 plug-and-play console. It is uses normal
 * OneBus banking, with one exception: the VT02's PRG A23 output (register $4100
 * bit 6 for PRG and bit 2 for CHR accesses) is connected to PRG-ROM A24, and
 * PRG A23 comes from the VT02's I/O port at $410F, bit 5 instead. Since the I/O
 * port is high-impedance on reset, which is pulled-up to a logical "1", the
 * reset vectors are in the second 8 MiB part of ROM.
 *
 * 620-in-1 (Mini Games Anniversary Edition) (Unl)
 */

#include "mapinc.h"
#include "onebus.h"

static uint8_t reg;

static void Sync(void) {
	OneBus_SyncPRG(0xF3FF, ((onebus.cpu41xx[0x0F] << 5) & 0x0400) | ((onebus.cpu41xx[0x00] << 5) & 0x0800));
	OneBus_SyncCHR(0x9FFF, ((onebus.cpu41xx[0x0F] << 8) & 0x2000) | ((onebus.cpu41xx[0x00] << 12) & 0x4000));
	OneBus_SyncMirror();
}

void Mapper436_Init(CartInfo *info) {
	OneBus_Init(info, Sync, 0, 0);
}
