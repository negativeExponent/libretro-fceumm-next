/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2019 Libretro Team
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

/* Waixing FS304 PCB */

#include "mapinc.h"

static struct {
	uint8_t reg[4];
} m162;

static SFORMAT StateRegs[] = {
	{ m162.reg, 4, "REGS" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000,
		(m162.reg[2] << 4) | (m162.reg[0] & 0x0C)										/* PRG A17-A20 always normal from $5000 and $5200 */
			| ((m162.reg[3] & 0x04) ? 0x00 : 0x02)								/* PRG A16 is 1       if $5300.2=0                */
			| ((m162.reg[3] & 0x04) ? (m162.reg[0] & 0x02) : 0x00)					/* PRG A16 is $5000.1 if %5300.2=1                */
			| ((m162.reg[3] & 0x01) ? 0x00 : m162.reg[1] >> 1 & 0x01)					/* PRG A15 is $5100.1 if               $5300.0=0  */
			| ((~m162.reg[3] & 0x04) && (m162.reg[3] & 0x01) ? 0x01 : 0x00)			/* PRG A15 is 1       if $5300.2=0 and $5300.0=1  */
			| ((m162.reg[3] & 0x04) && (m162.reg[3] & 0x01) ? (m162.reg[0] & 0x01) : 0x00) /* PRG A15 is $5000.0 if $5300.2=1 and $5300.0=1  */
	);
	setprg8r(0x10, 0x6000, 0);
	if (~m162.reg[0] & 0x80) {
		setchr8(0);
	}
}

static void HBIRQHook(void) {
	if ((m162.reg[0] & 0x80) && scanline < 239) { /* Actual hardware cannot look at the current scanline number, but instead latches PA09 on
												PA13 rises. This does not seem possible with the current PPU emulation however. */
		setchr4(0x0000, (scanline >= 127) ? 1 : 0);
		setchr4(0x1000, (scanline >= 127) ? 1 : 0);
	} else {
		setchr8(0);
	}
}

static DECLFR(ReadReg) {
	return 0x00;
}

static DECLFW(WriteReg) {
	m162.reg[(A >> 8) & 0x03] = V;
	Sync();
}

static void Power(void) {
	memset(&m162, 0, sizeof(m162));
	Sync();
	SetReadHandler(0x5000, 0x57FF, ReadReg);
	SetWriteHandler(0x5000, 0x57FF, WriteReg);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
}

static void Reset(void) {
	memset(m162.reg, 0, sizeof(m162.reg));
	Sync();
}

static void Close(void) {
}

static void StateRestore(int version) {
	Sync();
}

void Mapper162_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	info->Close = Close;
	GameHBIRQHook = HBIRQHook;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : 8192;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);

	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}
}
