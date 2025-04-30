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

/* NES 2.0 Mapper 445
 * DG574B MMC3-compatible multicart circuit board.
 */

#include "mapinc.h"
#include "mmc3.h"
#include "vrc24.h"

#define MAPPER_MMC3 0x00
#define MAPPER_VRC4 0x10

static uint8 reg[4];

static uint8 GetPRGBase(void) {
	return reg[0];
}

static uint8 GetPRGMask(void) {
	return ((0x7F >> (reg[2] & 0x07)) & 0x1F);
}

static void M445MMC3PW(uint16 A, uint16 V) {
	uint16 mask = GetPRGMask();
	uint16 base = GetPRGBase();

	setprg8(A, (base & ~mask) | (V & mask));
}

static void M445VRC24PW(uint16 A, uint16 V) {
	uint16 mask = GetPRGMask();
	uint16 base = GetPRGBase();

	setprg8(A, (base & ~mask) | (V & mask));
}

static uint8 GetCHRBase(void) {
	return reg[1];
}

static uint8 GetCHRMask(void) {
	return ((0x3FF >> ((reg[2] >> 3) & 0x07)) & 0xFF);
}

static void M445MMC3CW(uint16 A, uint16 V) {
	uint16 mask = GetCHRMask();
	uint16 base = GetCHRBase() << 3;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void M445VRC24CW(uint16 A, uint16 V) {
	uint16 mask = GetCHRMask();
	uint16 base = GetCHRBase() << 3;

	setchr1(A, (base & ~mask) | (V & mask));
}

static void Sync(void) {
	switch (reg[3] & 0x10) {
	case MAPPER_VRC4:
		VRC24_FixPRG();
		VRC24_FixCHR();
		VRC24_FixMIR();
		break;
	case MAPPER_MMC3:
	default:
		MMC3_FixPRG();
		MMC3_FixCHR();
		MMC3_FixMIR();
		break;
	}
}

static DECLFW(M445WriteREG) {
	if (!(reg[3] & 0x20)) {
		reg[A & 0x03] = V;
		Sync();
	}
}

static DECLFW(M445WriteASIC) {
	switch (reg[3] & 0x10) {
	case MAPPER_VRC4:
		vrc24.A0 = (reg[3] & 0x01) ? 0x0A : 0x05;
		vrc24.A1 = (reg[3] & 0x01) ? 0x05 : 0x0A;
		VRC24_Write(A, V);
		break;
	case MAPPER_MMC3:
		MMC3_Write(A, V);
		break;
	}
}

static void M445Reset(void) {
	reg[0] = 0x00;
	reg[1] = 0x00;
	reg[2] = 0x00;
	reg[3] = 0x00;
	Sync();
}

static void M445Power(void) {
	reg[0] = 0x00;
	reg[1] = 0x00;
	reg[2] = 0x00;
	reg[3] = 0x00;
	VRC24_Power();
	MMC3_Power();
	SetWriteHandler(0x5000, 0x5FFF, M445WriteREG);
	SetWriteHandler(0x8000, 0xFFFF, M445WriteASIC);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

static void CPUIRQHook(int a) {
	switch (reg[3] & 0x10) {
	case MAPPER_VRC4:
		VRC24_IRQCPUHook(a);
		break;
	case MAPPER_MMC3:
		break;
	}
}

static void HBIRQHook(void) {
	switch (reg[3] & 0x10) {
	case MAPPER_VRC4:
		break;
	case MAPPER_MMC3:
		MMC3_IRQHBHook();
		break;
	}
}

void Mapper445_Init(CartInfo *info) {
	MMC3_Init(info, MMC3B, FALSE, FALSE);
	MMC3_pwrap = M445MMC3PW;
	MMC3_cwrap = M445MMC3CW;

	VRC24_Init(info, VRC24_VRC4, 0x01, 0x02, FALSE, TRUE);
	VRC24_pwrap = M445VRC24PW;
	VRC24_cwrap = M445VRC24CW;

	info->Power = M445Power;
	info->Reset = M445Reset;

	GameStateRestore = StateRestore;
	AddExState(reg, 4, 0, "EXPR");

	MapIRQHook = CPUIRQHook;
	GameHBIRQHook = HBIRQHook;
}
