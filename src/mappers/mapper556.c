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

/* NES 2.0 Mapper 556
 * Used for the for the 超强小新2+瑪莉家族 7-in-1 (JY-215) multicart.
 */

#include "mapinc.h"
#include "mmc3.h"
#include "vrc24.h"

static struct {
	uint8_t cmd;
	uint8_t reg[4];
} m556;

static SFORMAT StateRegs[] = {
	{ m556.reg, 5, "REGS" },
	{ &m556.cmd, 1, "CMD0" },
	{ 0 }
};

static uint32_t GetPRGMask(void) {
	return (~m556.reg[3] & 0x3F);
}

static uint32_t GetPRGBase(void) {
	return (((m556.reg[3] & 0x40) << 2) | m556.reg[1]);
}

static uint32_t GetCHRMask(void) {
	return (0xFF >> (~m556.reg[2] & 0x0F));
}

static uint32_t GetCHRBase(void) {
	return (((m556.reg[3] & 0x40) << 6) | ((m556.reg[2] & 0xF0) << 4) | m556.reg[0]);
}

static void SetPRG_mmc3(uint16_t A, uint16_t V) {
	uint32_t mask = GetPRGMask();
	uint32_t base = GetPRGBase();

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR_mmc3(uint16_t A, uint16_t V) {
	uint32_t mask = GetCHRMask();
	uint32_t base = GetCHRBase();

	setchr1(A, (base & ~mask) | (V & mask));
}

static void SetPRG_vrc4(uint16_t A, uint16_t V) {
	uint32_t mask = GetPRGMask();
	uint32_t base = GetPRGBase();

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR_vrc4(uint16_t A, uint16_t V) {
	uint32_t mask = GetCHRMask();
	uint32_t base = GetCHRBase();

	setchr1(A, (base & ~mask) | (V & mask));
}

static void Sync(void) {
	if (m556.reg[2] & 0x80) {
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
	} else {
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	}
}

static DECLFW(WriteReg) {
	if (!(m556.reg[3] & 0x80)) {
		m556.reg[m556.cmd] = V;
		m556.cmd = (m556.cmd + 1) & 0x03;
		Sync();
	}
}

static DECLFW(WriteASIC) {
	if (m556.reg[2] & 0x80) {
		VRC24_Write(A, V);
	} else {
		MMC3_Write(A, V);
	}
}

static void CPUIRQHook(int a) {
	if (m556.reg[2] & 0x80) {
		VRC24_IRQCPUHook(a);
	}
}

static void HBIRQHook(void) {
	if (!(m556.reg[2] & 0x80)) {
		MMC3_IRQHBHook();
	}
}

static void Reset(void) {
	memset(&m556, 0, sizeof(m556));
	m556.reg[2] = 0x0F;
	Sync();
}

static void Power(void) {
	memset(&m556, 0, sizeof(m556));
	m556.reg[2] = 0x0F;

	MMC3_Reset();
	VRC24_Reset();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteASIC);

	if (WRAM) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper556_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x05, 0x0A, 0, TRUE);
	VRC24_pwrap = SetPRG_vrc4;
	VRC24_cwrap = SetCHR_vrc4;

	MMC3_Init(info, MMC3B, 0, 0);
	MMC3_pwrap = SetPRG_mmc3;
	MMC3_cwrap = SetCHR_mmc3;

	info->Reset = Reset;
	info->Power = Power;
	MapIRQHook = CPUIRQHook;
	GameHBIRQHook = HBIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (info->battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}
}
