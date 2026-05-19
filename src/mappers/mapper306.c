/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2016 CaH4e3
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
 * NES 2.0 Mapper 306 - Kaiser 7016
 * UNIF UNL-KS7016
 * FDS Conversion (Exciting Basket), weird banking addressing, seems because
 * of used addressing scheme, made to disable the lower system banks from 6000
 * but the kaiser mapper chip and PCB are the same as usual
 * probably need a hard eprom dump to verify actual banks layout
 *
 */

#include "mapinc.h"
#include "fdssound.h"

static struct {
	uint8_t reg;
} m306;

static SFORMAT StateRegs[] = {
	{ &m306.reg, 1, "REGS" },
	{ 0 }
};

static void SyncPRG(void) {
	setprg32(0x8000, 3);
}

static void SyncCHR(void) {
	setchr8(0);
}

static void SyncWRAM(void) {
	setprg8(0x6000, m306.reg);
}

static DECLFW(WriteReg) {
	if ((A & 0xD903) == 0xD903) {
		if (A & 0x40) {
			m306.reg = (A >> 2) & 0x0F;
		} else {
			m306.reg = 0x08 | ((A >> 2) & 0x03);
		}
		SyncWRAM();
	}
}

static void Power(void) {
	memset(&m306, 0, sizeof(m306));
	FDSSound_Power();
	SyncPRG();
	SyncCHR();
	SyncWRAM();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteReg);
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncWRAM();
}

void Mapper306_Init(CartInfo *info) {
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
