/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024-2025-2026 negativeExponent
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

/* NES 2.0 Mapper 373 - SFC-13 */
/* FIXME: Poewer Ranges 5 freeze/lock (irq?) */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
	uint8_t cmd;
} m373;

static SFORMAT StateRegs[] = {
	{ m373.reg, 4, "EXPR" },
	{ &m373.cmd, 1, "CMD0" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = ~m373.reg[3] & 0x3F;
	uint16_t base = m373.reg[1] | m373.reg[2] << 2 & 0x300;

	if (m373.reg[2] & 0x20) { /* GNROM-like */
		if (!(A & 0x4000)) {
			setprg8(A, (base & ~mask) | ((V & mask) & 0xFD));
			A += 0x4000;
			setprg8(A, (base & ~mask) | ((V & mask) | 0x02));
		}
	} else {
		setprg8(A, base | (V & mask));
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint32_t mask = 0xFF >> (~m373.reg[2] & 0xF);
	uint32_t base = m373.reg[0] | ((m373.reg[2] << 4) & 0xF00);

	/* CHR A10-A17, OR'd with MMC3's CHR A10-A17 masked according to CHR-AND */
	setchr1(A, base | (V & mask));
}

static DECLFW(WriteReg) {
	CartBW(A, V);
	if (!(m373.reg[3] & 0x40)) {
		m373.reg[m373.cmd] = V;
		m373.cmd = (m373.cmd + 1) & 0x03;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m373, 0, sizeof(m373));
	m373.reg[2] = 0x0F;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m373, 0, sizeof(m373));
	m373.reg[2] = 0x0F;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper373_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 8, info->battery);
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Reset = Reset;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
