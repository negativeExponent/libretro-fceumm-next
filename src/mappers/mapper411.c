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
 *
 */

/* Mapper 411 - A88S-1
 * 1997 Super 7-in-1 (JY-201)
 * 1997 Super 6-in-1 (JY-202)
 * 1997 Super 7-in-1 (JY-203)
 * 1997 龍珠武鬥會 7-in-1 (JY-204)
 * 1997 Super 7-in-1 (JY-205)
 * 1997 Super 7-in-1 (JY-206)
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[2];
} m411;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ m411.reg, 4, "EXPR" },
	{ 0 }
};


static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base, mask;

	switch (iNESCart.submapper) {
	default:
		base = ((m411.reg[1] << 1) & 0x10) | ((m411.reg[1] >> 1) & 0x60);
		mask = (m411.reg[1] & 0x02) ? 0x1F : 0x0F;
		break;
	case 1:
		base = ((m411.reg[1] << 1) & 0x10) | ((m411.reg[1] >> 1) & 0x60);
		mask = (m411.reg[1] & 0x02) ? 0x1F : 0x0F;
		break;
	case 2:
		base = ((m411.reg[1] << 1) & 0x10) | ((m411.reg[1] >> 1) & 0x60);
		mask = (m411.reg[1] & 0x01) ? 0x1F : 0x0F;
		break;
	}

	/* NROM Mode */
	if ((m411.reg[0] & 0x40) && !(m411.reg[0] & 0x20)) { /* NOTE: $5xx0 bit 5 check required for JY-212 */
		uint16_t bank = (base >> 1) | (m411.reg[0] & 0x05) | ((m411.reg[0] >> 2) & 0x02);
		if (m411.reg[0] & 0x02) { /* NROM-256 */
			setprg32(0x8000, bank >> 1);
		} else { /* NROM-128 */
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else { /* MMC3 */
		setprg8(A, (base & ~mask) | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base, mask;

	switch (iNESCart.submapper) {
	default:
		base = ((m411.reg[1] << 5) & 0x080) | ((m411.reg[0] << 4) & 0x100) | ((m411.reg[1] << 2) & 0x200);
		mask = (m411.reg[1] & 0x02) ? 0xFF : 0x7F;
		break;
	case 1:
		base = ((m411.reg[1] << 5) & 0x080) | ((m411.reg[1] << 2) & 0x100);
		mask = (m411.reg[1] & 0x02) ? 0xFF : 0x7F;
		break;
	case 2:
		base = ((m411.reg[1] << 5) & 0x080) | ((m411.reg[0] << 4) & 0x100) | ((m411.reg[1] << 2) & 0x200);
		mask = (m411.reg[1] & 0x02) ? 0xFF : 0x7F;
		break;
	}

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadDIP) {
	return dipsw;
}

static DECLFW(WriteReg) {
	if ((iNESCart.submapper == 2) || (A & 0x800)) {
		m411.reg[A & 0x01] = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m411, 0, sizeof(m411));
	m411.reg[1] = 0x03;
	dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m411, 0, sizeof(m411));
	m411.reg[1] = 0x03;
	dipsw = 0;
	MMC3_Power();
	SetReadHandler(0x5000, 0x5FFF, ReadDIP);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
}

void Mapper411_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
