/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright (C) 2019 Libretro Team
 *  Copyright (C) 2023-2025-2026 negativeExponent
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

/* NES 2.0 mapper 339 is used for a 21-in-1 multicart.
 * Its UNIF board name is BMC-K-3006.
 * http://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_339
 *
 * NROM-256 selection
 * Every multicart using this mapper has its own method of selecting NROM-256 mode, in which PRG A14 comes directly from CPU A14 rather than the "a" bit, that is denoted by NES 2.0 submapper:
 *
 * Submapper 0: NROM-256 if (Address AND $06) == $06 (K-3006 PCB, UNIF MAPR BMC-K-3006)
 * Submapper 1: NROM-256 if (Address AND $04) != $00 (unmarked PCB)
 * Submapper 2: NROM-256 if (Address AND $11) != $00 (Realtec 8058 PCB)
 * Submapper 3: NROM-256 if (Address AND $18) != $00 (K-3091/GN-16 PCB)
 * Submapper 4: NROM-256 if (Address AND $14) != $00 (GR-002-31 PCB)
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint16_t reg;
	uint8_t dipsw;
} m339;

static SFORMAT StateRegs[] = {
	{ &m339.reg, 2 | FCEUSTATE_RLSB, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint16_t base = m339.reg << 1;
	uint16_t mask = 0x0F;

	if (!(m339.reg & 0x20)) { /* NROM */
		uint8_t nrom256 = FALSE;
		switch (iNESCart.submapper) {
		case 0: nrom256 = ((m339.reg & 0x06) == 0x06) ? TRUE : FALSE; break;
		case 1: nrom256 = ((m339.reg & 0x04) != 0x00) ? TRUE : FALSE; break;
		case 2: nrom256 = ((m339.reg & 0x11) != 0x00) ? TRUE : FALSE; break;
		case 3: nrom256 = ((m339.reg & 0x18) != 0x00) ? TRUE : FALSE; break;
		case 4: nrom256 = ((m339.reg & 0x14) != 0x00) ? TRUE : FALSE; break;
		}
		if (nrom256) { /* NROM-256 */
			mask = 0x03;
		} else { /* NROM-128 */
			mask = 0x01;
		}
		V = (A >> 13);
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint16_t base = m339.reg << 4;
	uint16_t mask = 0x7F; 

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadDIP) {
	if (m339.reg & 0x80) {
		A = (A & ~0x03) | (m339.dipsw & 0x03);
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	m339.reg = A;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
}

static void Reset(void) {
	memset(&m339.reg, 0, sizeof(m339.reg));
	m339.dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m339, 0, sizeof(m339));
	MMC3_Power();
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);

}

void Mapper339_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
