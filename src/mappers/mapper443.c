/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2022
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

/* NC3000M PCB */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m443;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m443.reg, 1, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t mask = 0x0F;
	uint8_t base = ((m443.reg << 4) & 0x20) | (m443.reg & 0x10);

	if (m443.reg & 0x04) { /* NROM */
		uint16_t bank = (base & ~mask) | (mmc3.reg[6] & mask);
		if (m443.reg & 0x08) { /* NROM-128 */
			setprg16(0x8000, bank >> 1);
			setprg16(0xC000, bank >> 1);
		} else { /* NROM-256 */
			setprg32(0x8000, bank >> 2);
		}
	} else { /*  MMC3 */
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, ((m443.reg << 8) & ~0xFF) | (V & 0xFF));
}

static DECLFR(ReadDIP) {
	return (((m443.reg & 0x0C) == 0x08) ? dipsw : CartBR(A));
}

static DECLFW(WriteReg) {
	m443.reg = A & 0xFF;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	dipsw++;
	dipsw &= 15;
	m443.reg = 0;
	MMC3_Reset();
}

static void Power(void) {
	dipsw = 0;
	m443.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
}

void Mapper443_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
