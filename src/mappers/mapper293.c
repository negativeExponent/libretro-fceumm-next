/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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
 * NewStar 12-in-1 and 76-in-1
 */

#include "mapinc.h"

static uint8 regs[2];

static SFORMAT StateRegs[] = {
	{ regs, 2, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8 mode = ((regs[0] >> 2) & 0x02) | ((regs[1] >> 6) & 0x01);
	uint8 base = ((regs[1] << 5) & 0x20) | ((regs[1] >> 1) & 0x18);
	uint8 bank = (regs[0] & 0x07);

	switch (mode) {
	case 0: /* UNROM */
		setprg16(0x8000, base | bank);
		setprg16(0xC000, base | 0x07);
		break;
	case 1:
		setprg16(0x8000, base | (bank & 0xFE));
		setprg16(0xC000, base | 0x07);
		break;
	case 2: /* NROM-128 */
		setprg16(0x8000, base | bank);
		setprg16(0xC000, base | bank);
		break;
	case 3: /* NROM-256 */
		setprg16(0x8000, (base | bank) & ~1);
		setprg16(0xC000, (base | bank) | 1);
		break;
	}
	setchr8(0);
	setmirror(((regs[1] >> 7) & 0x01) ^ 0x01);
}

static DECLFW(M293Write1) {
	regs[0] = V;
	regs[1] = V;
	Sync();
}

static DECLFW(M293Write2) {
	regs[1] = V;
	Sync();
}

static DECLFW(M293Write3) {
	regs[0] = V;
	Sync();
}

static void M293Power(void) {
	regs[0] = regs[1] = 0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0x9FFF, M293Write1);
	SetWriteHandler(0xA000, 0xBFFF, M293Write2);
	SetWriteHandler(0xC000, 0xDFFF, M293Write3);
}

static void StateRestore(int version) {
	Sync();
}

/* BMC 12-in-1/76-in-1 (NewStar) (Unl) */
void Mapper293_Init(CartInfo *info) {
	info->Power = M293Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
