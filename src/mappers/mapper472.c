/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* NES 2.0 Mapper 472 denotes the 恒格 FK-206 JG MMC3-compatible PCB. It is basically mapper 52 with the bits reshuffled. */
/* 11-in-1 (JY008) (Unl) */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m472;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m472.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = 0x0F;
	uint16_t base = m472.reg & 0xF0;

	/* FCEU_printf("PRG: A:%04x V:%02x R0:%02x\n", A, V, m472.reg); */
	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m472.reg & 0x20) ? 0x7F : 0xFF;
	uint16_t base = m472.reg << 3;

	/* FCEU_printf("CHR: A:%04x V:%02x R0:%02x\n", A, V, m472.reg); */
	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	/* FCEU_printf("Wr: A:%04x V:%02x R0:%02x\n", A, V, m472.reg); */
	if (MMC3_WramIsWritable()) {
		m472.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFR(ReadDIP) {
	/* FCEU_printf("Rd: A:%04x DIP:%02x\n", A, dipsw); */
	return dipsw;
}

static void Reset(void) {
	m472.reg = 0;
	dipsw ^= 0x80; /* any other variants? */
	MMC3_Reset();
}

static void Power(void) {
	m472.reg = 0;
	dipsw = 0x80; /* start with 4-in-1 menu */
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper472_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(&m472.reg, 1, 0, "EXPR");
}
