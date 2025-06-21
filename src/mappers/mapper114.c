/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023-2024 negativeExponent
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
	uint8 reg[2];
} m114;

static uint8 dipsw;

static SFORMAT StateRegs[] = {
	{ m114.reg, 2, "EXPR" },
	{ 0 }
};

static const uint16 translateAddrLut[2][8] = {
	{ 0xA001, 0xA000, 0x8000, 0xC000, 0x8001, 0xC001, 0xE000, 0xE001 }, /* 0: Aladdin, The Lion King */
	{ 0xA001, 0x8001, 0x8000, 0xC001, 0xA000, 0xC000, 0xE000, 0xE001 } /* 1: Boogerman */
};

static const uint8 translateDataLut[2][8] = {
	{ 0, 3, 1, 5, 6, 7, 2, 4 }, /* 0: Aladdin, The Lion King */
	{ 0, 2, 5, 3, 6, 1, 7, 4 }, /* 1: Boogerman */
};

static void SetPRG(uint16 A, uint16 V) {
	if (m114.reg[0] & 0x80) {
		uint8 bank = m114.reg[0] & 0x0F;

		if (m114.reg[0] & 0x20) {
			setprg32(0x8000, bank >> 1);
		} else {
			setprg16(0x8000, bank);
			setprg16(0xC000, bank);
		}
	} else {
		setprg8(A, V & 0x3F);
	}
}

static void SetCHR(uint16 A, uint16 V) {
	setchr1(A, ((m114.reg[1] << 8) & 0x100) | (V & 0x0FF));
}

static DECLFR(ReadDIP) {
	if ((A & 0x03) == 0x02) {
		return (cpu.openbus & ~0x07) | (dipsw & 0x07);
	}
	return cpu.openbus;
}

static DECLFW(WriteReg) {
	/* The Registers responds even when the MMC3 clone's WRAM bit is clear. */
	if (!(m114.reg[1] & 0x01)) {
		m114.reg[A & 0x03] = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
	}
}

static DECLFW(WriteMMC3) {
	A = translateAddrLut[iNESCart.submapper & 0x01][((A >> 12) & 0x06) | (A & 0x01)];
	if (A == 0x8000) {
		V = (V & 0xF8) | translateDataLut[iNESCart.submapper & 0x01][V & 0x07];
	}
	MMC3_Write(A, V);
}

static void Reset(void) {
	memset(&m114, 0, sizeof(m114));
	dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m114, 0, sizeof(m114));
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadDIP);
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteMMC3);
}

void Mapper114_Init(CartInfo *info) {
	MMC3_Init(info, MMC3A, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}

/* INES Mapper 182 is a duplicate of Mapper 114. In fact, FCEUX explicitly
 * reassigns Hosenkan's Mapper 182 ROMs to Mapper 114. Because Hosenkan's games
 * make no use of the $6000-$6001 registers, other than writing zero to both of
 * them on startup, some implementations (such as the EverDrive N8's) do not
 * emulate the $600x registers. */
void Mapper182_Init(CartInfo *info) {
	MMC3_Init(info, MMC3A, 0, 0);
	MMC3_pwrap = SetPRG;
	MMC3_cwrap = SetCHR;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
