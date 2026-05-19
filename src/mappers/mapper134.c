/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 */

/* Chipset used on various PCBs named WX-KB4K, T4A54A, BS-5652... */
/* "Rockman 3" on YH-322 and "King of Fighters 97" on "Super 6-in-1" enable interrupts without initializing the frame IRQ register and therefore freeze on real hardware.
   They can run if another game is selected that does initialize the frame IRQ register, then soft-resetting to the menu and selecting the previously-freezing games. */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
	uint8_t dipsw;
} m134;

static SFORMAT StateRegs[] = {
	{ m134.reg, 4, "REGS" },
	{ 0 }
};

static void SetPRGBank_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = (m134.reg[1] & 0x04) ? 0x0F : 0x1F;
	uint16_t base = ((m134.reg[1] << 4) & 0x30) | ((m134.reg[0] << 2) & 0x40);

	if (m134.reg[1] & 0x80) { /* NROM mode */
		uint8_t nrom_mask = (m134.reg[1] & 0x08) ? 0x01 : 0x03;
		V = MMC3_GetPRGBank(0) & ~nrom_mask;
		V |= (A >> 13) & nrom_mask;
	}

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHRBank_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = (m134.reg[1] & 0x40) ? 0x7F : 0xFF;
	uint16_t base = ((m134.reg[1] << 3) & 0x180) | ((m134.reg[0] << 4) & 0x200);

	if (m134.reg[0] & 0x08) { /* In CNROM mode, outer bank register 2 replaces the MMC3's CHR registers, and CHR A10-A12 are PPU A10-A12. */
		V = ((m134.reg[2] & mask) << 3) | ((A >> 10) & 0x07);
	}

	setchr1(A, (base & ~mask) | (V & mask));
}

static DECLFR(ReadDIP) {
	if (m134.reg[0] & 0x40) {
		return m134.dipsw;
	}

	return CartBR(A);
}

static DECLFW(WriteReg) {
	if (MMC3_WramIsWritable()) {
		CartBW(A, V);
		if (!(m134.reg[0] & 0x80)) {
			m134.reg[A & 0x03] = V;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
		} else if ((A & 0x03) == 2) {
			m134.reg[2] = (m134.reg[2] & ~0x03) | (V & 0x03);
			MMC3_SyncCHR();
		}
	}
}

static DECLFW(WriteMMC3) {
	MMC3_Write(A, V);
	switch (A & 0xE001) {
	case 0x8001:
		switch (mmc3.cmd & 0x07) {
		case 0x06:
			if (m134.reg[1] & 0x80) {
				MMC3_SyncPRG();
			}
			break;
		}
		break;
	}
}

static void Reset(void) {
	memset(m134.reg, 0, sizeof(m134.reg));
	m134.dipsw++;
	m134.dipsw &= 15;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m134, 0, sizeof(m134));
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	SetWriteHandler(0x8000, 0x9FFF, WriteMMC3);
}

void Mapper134_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) / 1024 : 8, info->battery);
	MMC3_cwrap = SetCHRBank_mmc3;
	MMC3_pwrap = SetPRGBank_mmc3;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
