/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2011 CaH4e3
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
 * SL12 Protected 3-in-1 mapper hardware (VRC2, MMC3, MMC1)
 * the same as 603-5052 board (TODO: add reading registers, merge)
 * SL1632 2-in-1 protected board, similar to SL12 (TODO: find difference)
 *
 * Known PCB:
 *
 * Garou Densetsu Special (G0904.PCB, Huang-1, GAL dip: W conf.)
 * Kart Fighter (008, Huang-1, GAL dip: W conf.)
 * Somari (008, C5052-13, GAL dip: P conf., GK2-P/GK2-V maskroms)
 * Somari (008, Huang-1, GAL dip: W conf., GK1-P/GK1-V maskroms)
 * AV Mei Shao Nv Zhan Shi (aka AV Pretty Girl Fighting) (SL-12 PCB, Hunag-1, GAL dip: unk conf. SL-11A/SL-11B maskroms)
 * Samurai Spirits (Full version) (Huang-1, GAL dip: unk conf. GS-2A/GS-4A maskroms)
 * Contra Fighter (603-5052 PCB, C5052-3, GAL dip: unk conf. SC603-A/SCB603-B maskroms)
 *
 */

#include "mapinc.h"
#include "vrc24.h"
#include "mmc3.h"
#include "mmc1.h"

#define MAPPER_VRC2 0x00
#define MAPPER_MMC3 0x01
#define MAPPER_MMC1 0x02

static struct {
	uint8_t mapper;
	uint8_t mode;
	uint8_t game;
} m116;

static SFORMAT StateRegs[] = {
	{ &m116.mapper, 1, "MAPR" },
	{ &m116.mode, 1, "MODE" },
	{ &m116.game, 1, "GAME" },
	{ 0 }
};

static uint32_t GetPRGMask(void) {
	if (iNESCart.submapper != 3) {
		return 0x3F;
	}
	return (m116.game ? 0x0F : 0x1F);
}

static uint32_t GetPRGBase(void) {
	if (m116.game) {
		return (m116.game + 1) * 0x10;
	}
	return 0;
}

static uint32_t GetCHRMask(void) {
	return (m116.game ? 0x7F : 0xFF);
}

static uint32_t GetCHRBase(void) {
	return (m116.game ? (m116.game + 1) * 0x80 : 0);
}

static void SetPRG_vrc2(uint16_t A, uint16_t V) {
	setprg8(A, GetPRGBase() | (V & GetPRGMask()));
}

static void SetCHR_vrc2(uint16_t A, uint16_t V) {
	setchr1(A, ((m116.mode << 6) & 0x100) | GetCHRBase() | (V & GetCHRMask()));
}

static void SetPRG_mmc3(uint16_t A, uint16_t V) {
	setprg8(A, GetPRGBase() | (V & GetPRGMask()));
}

static void SetCHR_mmc3(uint16_t A, uint16_t V) {
	setchr1(A, ((m116.mode << 6) & 0x100) | GetCHRBase() | (V & GetCHRMask()));
}

static void SetPRG_mmc1(uint16_t A, uint16_t V) {
	if (iNESCart.submapper == 2) {
		setprg16(A, V >> 1);
	} else {
		setprg16(A, (GetPRGBase() >> 1) | (V & (GetPRGMask() >> 1)));
	}
}

static void SetCHR_mmc1(uint16_t A, uint16_t V) {
	setchr4(A, (GetCHRBase() >> 2) | (V & (GetCHRMask() >> 2)));
}

static void Sync(void) {
	if (m116.mapper == MAPPER_MMC1) {
		MMC1_SyncPRG();
		MMC1_SyncCHR();
		MMC1_SyncMirror();
	} else if (m116.mapper == MAPPER_MMC3) {
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	} else if (m116.mapper == MAPPER_VRC2) {
		VRC24_SyncPRG();
		VRC24_SyncCHR();
		VRC24_SyncMirror();
	}
}

static void applyMode(void) {
	switch (m116.mode & 0x03) {
	case 1:
		m116.mapper = MAPPER_MMC3;
		SetWriteHandler(0x8000, 0xFFFF, MMC3_Write);
		break;
	case 2:
	case 3:
		m116.mapper = MAPPER_MMC1;
		SetWriteHandler(0x8000, 0xFFFF, MMC1_Write);
		break;
	case 0:
		m116.mapper = MAPPER_VRC2;
		SetWriteHandler(0x8000, 0xFFFF, VRC24_Write);
		break;
	}
}

static DECLFW(WriteMode) {
	if (A & 0x100) {
		m116.mode = V;
		applyMode();
		Sync();
	}
}

static void HBIRQHook(void) {
	if (m116.mapper == MAPPER_MMC3){
		MMC3_IRQHBHook();
	}
}

static void Reset(void) {
	if (iNESCart.submapper == 3) {
		m116.game = m116.game + 1;
		if (m116.game > 4) {
			m116.game = 0;
		}
	}
	applyMode();
	Sync();
}

static void Power(void) {
	m116.game = (iNESCart.submapper == 3) ? 4 : 0;
	m116.mode = 1;

	MMC3_Power();
	MMC1_Reset();
	VRC24_Power();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4100, 0x5FFF, WriteMode);

	vrc24.chr[0] = ~0;
	vrc24.chr[1] = ~0;
	vrc24.chr[2] = ~0;
	vrc24.chr[3] = ~0;

	applyMode();
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper116_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, FALSE, TRUE);
	VRC24_pwrap = SetPRG_vrc2;
	VRC24_cwrap = SetCHR_vrc2;

	MMC3_Init(info, MMC3B, FALSE, FALSE);
	MMC3_pwrap = SetPRG_mmc3;
	MMC3_cwrap = SetCHR_mmc3;

	MMC1_Init(info, MMC1A, FALSE, FALSE);
	MMC1_pwrap = SetPRG_mmc1;
	MMC1_cwrap = SetCHR_mmc1;

	info->Power = Power;
	info->Reset = Reset;

	GameHBIRQHook = HBIRQHook;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	/* PRG 128K and CHR 128K is Huang-2 (iNESCart.submapper 2) */
	if ((info->submapper != 2) && (ROM.prg.size == (128 * 1024)) && (ROM.chr.size == (128 * 1024))) {
		info->submapper = 2;
	}
}
