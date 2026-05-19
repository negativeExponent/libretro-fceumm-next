/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
 *  Copyright (C) 2023-2025-2026 negativeExponent
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/* NES 2.0 Mapper 403 denotes the 89433 circuit board with up to 1 MiB PRG-ROM and 32 KiB of CHR-RAM, bankable with 8
 * KiB granularity.
 *
 * Tetris Family - 玩家 19-in-1 智瑟實典 (NO-1683)
 * Sachen Superpack (versions A-C)
 */

#include "mapinc.h"

static struct {
	uint8_t reg[3];
} m403;

static SFORMAT StateRegs[] = {
	{ m403.reg, 3, "REGS" },
	{ 0 }
};

static void Sync(void) {
	uint8_t bank = m403.reg[0] >> 1;

	if (m403.reg[2] & 0x01) { /* NROM-128 */
		setprg16(0x8000, bank);
		setprg16(0xC000, bank);
	} else { /* NROM-256 */
		setprg32(0x8000, bank >> 1);
	}
	setchr8(m403.reg[1]);
	setmirror(((m403.reg[2] >> 4) & 0x01) ^ 0x01);
}

static DECLFR(Read2) {
	/* For TetrisA (Tetris Family 19-in-1 NO 1683) */
	/* expects something other than openbus */
	/* unmapped pages returns zero by default so just use that */
	return 0;
}

static DECLFW(WriteReg) {
	if (A & 0x100) {
		m403.reg[A & 0x03] = V;
		Sync();
	}
}

static DECLFW(Writelatch) {
	if (m403.reg[2] & 0x04) {
		m403.reg[1] = V;
		Sync();
	}
}

static void Reset(void) {
	memset(&m403, 0, sizeof(m403));
	Sync();
}

static void Power(void) {
	memset(&m403, 0, sizeof(m403));
	Sync();
	SetReadHandler(0x6004, 0x6004, Read2);
	SetReadHandler(0x6015, 0x6015, Read2);
	SetReadHandler(0x6026, 0x6026, Read2);
	SetReadHandler(0x6037, 0x6037, Read2);
	SetReadHandler(0x7061, 0x7061, Read2);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, Writelatch);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper403_Init(CartInfo *info) {
	info->Reset = Reset;
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
