/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2008 CaH4e3
 *  Copyright (C) 2019 Libretro Team
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

/* NES 2.0 Mapper 287
 * BMC-411120-C, actually cart ID is 811120-C, sorry ;) K-3094 - another ID
 * - 4-in-1 (411120-C)
 * - 4-in-1 (811120-C,411120-C) [p4][U]
 *
 * BMC-K-3088, similar to BMC-411120-C but without jumper or dipswitch
 * - 19-in-1(K-3088)(810849-C)(Unl)
 */

/* 2023-03-02
 - use PRG size to determine variant
 - remove forced mask for outer-bank (rely on internal mask set during PRG/CHR mapping)
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg;
} m287;

static uint8_t dipsw;

static SFORMAT StateRegs[] = {
	{ &m287.reg, 1, "REGS" },
	{ 0 }
};

static void SetCHRBank(uint16_t A, uint16_t V) {
	uint16_t base = m287.reg << 7;
	uint16_t mask = 0x7F;

	setchr1(A, ((base & ~mask) | (V & mask)));
}

static void SetPRGBank(uint16_t A, uint16_t V) {
	if ((m287.reg & 0x04) && dipsw && ROM.prg.size < (1024 * 1024)) {
		unsetcpu32(0x8000);
	} else {
		if (m287.reg & 0x08) {
			/* 32K Mode */
			setprg32(0x8000, ((m287.reg << 2) & ~0x03) | ((m287.reg >> 4) & 0x03));
			/* FCEU_printf("32K mode: bank:%02x\n", ((m287.reg >> 4) & 3) | ((m287.reg & 7) << 2)); */
		} else {
			/* MMC3 Mode */
			uint8_t base = m287.reg << 4;
			uint8_t mask = 0x0F;

			setprg8(A, ((base & ~mask) | (V & mask)));
			/* FCEU_printf("MMC3: %04x:%02x\n", A, (V & 0x0F) | ((m287.reg & 7) << 4)); */
		}
	}
}

static DECLFW(WriteReg) {
	/*	printf("Wr: A:%04x V:%02x\n", A, V); */
	if (MMC3_WramIsWritable()) {
		m287.reg = A;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static void Reset(void) {
	m287.reg = 0;
	dipsw ^= 4;
	MMC3_Reset();
}

static void Power(void) {
	m287.reg = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper287_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRGBank;
	MMC3_cwrap = SetCHRBank;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
