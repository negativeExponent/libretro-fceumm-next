/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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
	uint8_t extMode;
} m219;

static SFORMAT StateRegs[] = {
	{ &m219.reg, 2, "EXPR" },
	{ &m219.extMode, 1, "MODE" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t mask = (m219.reg[0] & 0x40) ? 0x0F : 0x1F;
	uint16_t base = (m219.reg[1] & 0x20) | ((m219.reg[0] << 4) & 0x10);

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t mask = (m219.reg[0] & 0x80) ? 0x7F : 0xFF;
	uint16_t base = ((m219.reg[1] << 3) & 0x100) | ((m219.reg[0] << 4) & 0x80);

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFW(WriteReg) {
	m219.reg[A & 0x01] = V;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static DECLFW(WriteASIC) {
	switch (A & 0xE001) {
	case 0x8000:
		MMC3_Write(A, V);
		if (A & 0x02) {
			m219.extMode = (V & 0x20) != 0;
		}
		break;
	case 0x8001:
		if (!m219.extMode) { /* Scrambled mode inactive */
			MMC3_Write(A, V);
			break;
		}
		if ((mmc3.cmd >= 0x08) && (mmc3.cmd <= 0x1F)) { /* Scrambled CHR register */
			uint8_t index = (mmc3.cmd - 8) >> 2;

			if (mmc3.cmd & 0x01) { /* LSB nibble */
				mmc3.reg[index] &= ~0x0F;
				mmc3.reg[index] |= ((V >> 1) & 0x0F);
			} else { /* MSB nibble */
				mmc3.reg[index] &= ~0xF0;
				mmc3.reg[index] |= ((V << 4) & 0xF0);
			}
		} else if ((mmc3.cmd >= 0x25) && (mmc3.cmd <= 0x26)) { /* Scrambled PRG register */
			V = ((V << 1) & 0x08) | ((V >> 1) & 0x04) |
				((V >> 3) & 0x02) | ((V >> 5) & 0x01);
			mmc3.reg[0x06 | (mmc3.cmd & 0x01)] = V;
		}
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		break;
	}
}

static void Power(void) {
	memset(&m219, 0, sizeof(m219));
	m219.reg[1] = 0x20;
	MMC3_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteASIC);
}

static void Reset(void) {
	memset(&m219, 0, sizeof(m219));
	m219.reg[1] = 0x20;
	MMC3_Reset();
}

void Mapper219_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
