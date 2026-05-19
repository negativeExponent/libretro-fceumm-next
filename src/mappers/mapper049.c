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

/* iNES Mapper 49 */
/* BMC-STREETFIGTER-GAME4IN1 - Sic. $6000 set to $41 rather than $00 on power-up. */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m049;

static SFORMAT StateRegs[] = {
	{ &m049.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	if (m049.reg & 0x01) {
		setprg8(A, ((m049.reg >> 2) & ~0x0F) | (V & 0x0F));
	} else {
		uint8_t mask = 0x0F;
		if (iNESCart.submapper == 1) {
			/* Street Fighter 2 of the UNIF variant */
			mask = 0x03;
		}
		setprg32(0x8000, (m049.reg >> 4) & mask);
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, ((m049.reg << 1) & ~0x7F) | (V & 0x7F));
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m049.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	m049.reg = ((iNESCart.submapper == 1) || (iNESCart.PRGCRC32 == 0x408EA235)) ? 0x41 : 0x00; /* Street Fighter II Game 4-in-1 */
	MMC3_Reset();
}

static void Power(void) {
	m049.reg = ((iNESCart.submapper == 1) || (iNESCart.PRGCRC32 == 0x408EA235)) ? 0x41 : 0x00; /* Street Fighter II Game 4-in-1 */
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper049_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
