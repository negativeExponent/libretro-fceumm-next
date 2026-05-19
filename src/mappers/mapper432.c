/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

static struct {
	uint8_t reg[2];
} m432;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m432.reg, 2, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m432.reg[1] & 0x02) ? 0x0F : 0x1F;
	uint16_t base = ((m432.reg[1] << 4) & 0x10) | ((m432.reg[1] << 1) & 0x60);
	uint8_t nrom256 = (iNESCart.submapper == 2) ? ((m432.reg[1] & 0x20) != 0) : ((m432.reg[1] & 0x80) != 0);

	if (m432.reg[1] & 0x40) { /* NROM */
		if (!(A & 0x4000)) { /* GNROM */
			uint8_t A14 = ((iNESCart.submapper == 2) ? ((m432.reg[1] & 0x20) != 0) : ((m432.reg[1] & 0x80) != 0)) ? 0x02 : 0;

			setprg8(A, (base & ~mask) | ((V & mask) & ~A14));
			A += 0x4000;
			setprg8(A, (base & ~mask) | ((V & mask) |  A14));
		}
	} else { /* MMC3 */
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m432.reg[1] & 0x04) ? 0x7F : 0xFF;
	uint16_t base = ((m432.reg[1] << 4) & 0x200) | ((m432.reg[1] << 5) & 0x100) | ((m432.reg[1] << 7) & 0x80);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadDIP) {
	if ((iNESCart.submapper == 1) ? ((m432.reg[1] & 0x20) != 0) : ((m432.reg[0] & 0x01) != 0)) {
		return dipsw;
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		m432.reg[A & 0x01] = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	m432.reg[0] = 0;
	m432.reg[1] = 0;
	dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	m432.reg[0] = 0;
	m432.reg[1] = 0;
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper432_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
