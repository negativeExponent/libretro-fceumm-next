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

/* NES 2.0 Mapper 445
 * DG574B MMC3-compatible multicart circuit board.
 */

#include "mapinc.h"
#include "mmc3.h"
#include "vrc24.h"

#define MAPPER_MMC3 0x00
#define MAPPER_VRC4 0x10

static struct {
	uint8_t reg[4];
} m445;

static SFORMAT StateRegs[] = {
	{ m445.reg, 4, "EXPR" },
	{ 0 }
};

static uint8_t GetPRGBase(void) {
	return m445.reg[0];
}

static uint8_t GetPRGMask(void) {
	return ((0x7F >> (m445.reg[2] & 0x07)) & 0x1F);
}

static void SetPRG_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = GetPRGMask();
	uint16_t base = GetPRGBase();

	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetPRG_vrc4(uint16_t A, uint16_t V) {
	uint16_t mask = GetPRGMask();
	uint16_t base = GetPRGBase();

	setprg8(A, (base & ~mask) | (V & mask));
}

static uint8_t GetCHRBase(void) {
	return m445.reg[1];
}

static uint8_t GetCHRMask(void) {
	return ((0x3FF >> ((m445.reg[2] >> 3) & 0x07)) & 0xFF);
}

static void SetCHR_mmc3(uint16_t A, uint16_t V) {
	uint16_t mask = GetCHRMask();
	uint16_t base = GetCHRBase() << 3;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void SetCHR_vrc4(uint16_t A, uint16_t V) {
	uint16_t mask = GetCHRMask();
	uint16_t base = GetCHRBase() << 3;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void Sync(void) {
	switch (m445.reg[3] & 0x10) {
	case MAPPER_VRC4:
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
		break;
	case MAPPER_MMC3:
	default:
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
		break;
	}
}

static DECLFW(WriteReg) {
	if (!(m445.reg[3] & 0x20)) {
		m445.reg[A & 0x03] = V;
		Sync();
	}
}

static DECLFW(WriteASIC) {
	switch (m445.reg[3] & 0x10) {
	case MAPPER_VRC4:
		vrc24.A0 = (m445.reg[3] & 0x01) ? 0x0A : 0x05;
		vrc24.A1 = (m445.reg[3] & 0x01) ? 0x05 : 0x0A;
		VRC24_Write(A, V);
		break;
	case MAPPER_MMC3:
		MMC3_Write(A, V);
		break;
	}
}

static void CPUIRQHook(int a) {
	switch (m445.reg[3] & 0x10) {
	case MAPPER_VRC4:
		VRC24_IRQCPUHook(a);
		break;
	case MAPPER_MMC3:
		break;
	}
}

static void HBIRQHook(void) {
	switch (m445.reg[3] & 0x10) {
	case MAPPER_VRC4:
		break;
	case MAPPER_MMC3:
		MMC3_IRQHBHook();
		break;
	}
}

static void Reset(void) {
	memset(&m445, 0, sizeof(m445));
	Sync();
}

static void Power(void) {
	memset(&m445, 0, sizeof(m445));
	VRC24_Power();
	MMC3_Power();
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteASIC);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper445_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, FALSE, FALSE);
	MMC3_pwrap = SetPRG_mmc3;
	MMC3_cwrap = SetCHR_mmc3;

	VRC24_Init(info, VRC24_VRC4, 0x01, 0x02, FALSE, TRUE);
	VRC24_pwrap = SetPRG_vrc4;
	VRC24_cwrap = SetCHR_vrc4;

	info->Power = Power;
	info->Reset = Reset;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	MapIRQHook = CPUIRQHook;
	GameHBIRQHook = HBIRQHook;
}
