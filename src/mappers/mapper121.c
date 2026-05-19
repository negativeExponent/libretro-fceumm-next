/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007-2008 Mad Dumper, CaH4e3
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
 * Panda prince pirate.
 * MK4, MK6, A9711/A9713 board
 * 6035052 seems to be the same too, but with prot array in reverse
 * A9746  seems to be the same too, check
 * 187 seems to be the same too, check (A98402 board)
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t prg[3];
	uint8_t reg;
	uint8_t readIndex;
	uint8_t protIndex;
	uint8_t protLatch;
} m121;

static SFORMAT StateRegs[] = {
	{ m121.prg, 3, "PREG" },
	{ &m121.reg, 1, "REGS" },
	{ &m121.readIndex, 1, "PRRD" },
	{ &m121.protIndex, 1, "PRID" },
	{ &m121.protLatch, 1, "PRLT" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t mask = 0x1F;
	uint8_t base = ((m121.reg & 0x80) >> 2);

	if (m121.protIndex & 0x20) {
		if ((A > 0x8000)) {
			mask = 0xFF;
			V = m121.prg[((A >> 13) & 0x03) - 1];
		}
	}

	setprg8(A, base | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	if (ROM.prg.size > SIZE_256K) {
		setchr1(A, ((m121.reg & 0x80) << 1) | V);
	} else {
		uint16_t base = ((A >> 4) & 0x100);
		setchr1(A, base | V);
	}
}

static const uint8_t prot_array[] = { 0x83, 0x83, 0x42, 0x00, 0x00, 0x02, 0x02, 0x03 };
static DECLFR(ReadProtection) {
	return prot_array[m121.readIndex];
}

static DECLFW(WriteReg) {
	m121.readIndex = ((A >> 6) & 0x04) | (V & 0x03);
	if (A & 0x0100) {
		m121.reg = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFW(WriteProtectionLatch) {
	switch (A & 0xE001) {
	case 0x8001:
		switch (A & 0x03) {
		case 0x01:
			m121.protLatch = ((V & 0x01) << 5) | ((V & 0x02) << 3) |
			            ((V & 0x04) << 1) | ((V & 0x08) >> 1) |
			            ((V & 0x10) >> 3) | ((V & 0x20) >> 5);
			if ((m121.protIndex == 0x26) || (m121.protIndex == 0x28) || (m121.protIndex == 0x2A)) {
				m121.prg[0x15 - (m121.protIndex >> 1)] = m121.protLatch;
			}
			break;
		case 0x03:
			m121.protIndex = V & 0x3F;
			if ((m121.protIndex & 0x20) && m121.protLatch) {
				m121.prg[2] = m121.protLatch;
			}
			break;
		}
		mmc3.reg[mmc3.cmd & 0x07] = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		break;
	default:
		MMC3_CMDWrite(A, V);
		break;
	}
}

static void Power(void) {
	memset(m121.prg, 0, sizeof(m121.prg));
	MMC3_Power();
	SetReadHandler(0x5000, 0x5FFF, ReadProtection);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0x9FFF, WriteProtectionLatch);
}

void Mapper121_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	AddExState(StateRegs, ~0, 0, NULL);
}
