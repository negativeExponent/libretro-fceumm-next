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

#include "mapinc.h"
#include "latch.h"
#include "mmc1.h"
#include "mmc2.h"
#include "mmc3.h"
#include "mmc3.h"
#include "vrc1.h"
#include "vrc3.h"
#include "vrc24.h"
#include "vrc6.h"
#include "vrc7.h"
#include "h3001.h"
#include "flashrom.h"

/* TODO: Only PNROM was tested */

static struct {
	uint8_t reg[8];
} m446;

static SFORMAT StateRegs[] = {
	{ m446.reg, 8, "EXPR" },
	{ 0 }
};

static void (*mapperSync_cb)(void) = NULL;
static void SetMode(uint8_t);
static DECLFW(WriteReg);
static DECLFR(ReadFlash);
static DECLFW(WriteFlash);

#define CHIP_WRAM  0x10
#define CHIP_ROM   0x00
#define CHIP_FLASH 0x11

static INLINE int Mapper_GetPRGMask(void) {
	return (m446.reg[3] ^ ((iNESCart.submapper == 2) ? 0x00 : 0xFF));
}

static INLINE int Mapper_GetPRGBase(void) {
	return (m446.reg[1] | (m446.reg[2] << 8));
}

static INLINE int Mapper_GetCHRMask(void) {
	return (((m446.reg[4] << 2) & 0xE0) ^ 0xFF);
}

static INLINE int Mapper_GetCHRBase(void) {
	return m446.reg[6];
}

static void Sync(void) {
	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], !(m446.reg[5] & 0x04));
	if (mapperSync_cb) {
		mapperSync_cb();
	}
}

static void Sync_152(void) {
	uint16_t prgMask = Mapper_GetPRGMask() >> 1;
	uint16_t chrMask = Mapper_GetCHRMask() >> 3;
	uint16_t prgBase = Mapper_GetPRGBase() >> 1;
	uint16_t chrBase = Mapper_GetCHRBase() >> 3;
	setprg16r(CHIP_FLASH, 0x8000, ((latch.data >> 4) & prgMask) | (prgBase & ~prgMask));
	setprg16r(CHIP_FLASH, 0xC000, prgBase | prgMask);
	setchr8((latch.data & chrMask) | (chrBase & ~chrMask));
	setmirror((latch.data & 0x80) ? MI_1 : MI_0);
}

static void Sync_AxROM(void) {
	uint16_t prgMask = Mapper_GetPRGMask() >> 2;
	uint16_t prgBase = Mapper_GetPRGBase() >> 2;
	uint16_t chrBase = Mapper_GetCHRBase();
	setprg32r(CHIP_FLASH, 0x8000, (latch.data & prgMask) | (prgBase & ~prgMask));
	setchr8(chrBase);
	setmirror((latch.data & 0x10) ? MI_1 : MI_0);
}

static void Sync_BNROM(void) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	uint16_t chrBase = Mapper_GetCHRBase();
	setprg8r(CHIP_FLASH, 0x8000, (((latch.data << 2) | 0) & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xA000, (((latch.data << 2) | 1) & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xC000, (((latch.data << 2) | 2) & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xE000, (((latch.data << 2) | 3) & prgMask) | prgBase);
	setchr8(chrBase);
	setmirror((m446.reg[4] & 0x01) ? MI_V : MI_H);
}

static void Sync_CNROM(void) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t chrMask = Mapper_GetCHRMask() >> 3;
	uint16_t prgBase = Mapper_GetPRGBase();
	uint16_t chrBase = Mapper_GetCHRBase() >> 3;
	setprg8r(CHIP_FLASH, 0x8000, (0 & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xA000, (1 & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xC000, (2 & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xE000, (3 & prgMask) | prgBase);
	setchr8(latch.data & ((m446.reg[4] & 1) ? 0x07 : 0x03));
	setmirror((m446.reg[4] & 0x01) ? MI_V : MI_H);
}

static void Sync_CNROM_Konami(void) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t chrMask = Mapper_GetCHRMask() >> 3;
	uint16_t prgBase = Mapper_GetPRGBase();
	uint16_t chrBase = Mapper_GetCHRBase() >> 3;
	setprg8r(CHIP_FLASH, 0x8000, (0 & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xA000, (1 & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xC000, (2 & prgMask) | prgBase);
	setprg8r(CHIP_FLASH, 0xE000, (3 & prgMask) | prgBase);
	setchr8(((latch.data << 1) & 0x02) | ((latch.data >> 1) & 0x01));
	setmirror((m446.reg[4] & 0x01) ? MI_V : MI_H);
}

static void Sync_GNROM(void) {
	uint16_t prgMask = Mapper_GetPRGMask() >> 2;
	uint16_t chrMask = Mapper_GetCHRMask() >> 3;
	uint16_t prgBase = Mapper_GetPRGBase() >> 2;
	uint16_t chrBase = Mapper_GetCHRBase() >> 3;
	setprg32r(CHIP_FLASH, 0x8000, ((latch.data >> 4) & prgMask) | (prgBase & ~prgMask));
	setchr8(latch.data & 0x03);
	setmirror((m446.reg[4] & 0x01) ? MI_V : MI_H);
}

static void SetPRG_H3001(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_H3001(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr1(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_H3001(void) {
	H3001_pwrap = SetPRG_H3001;
	H3001_cwrap = SetCHR_H3001;
	H3001_SyncPRG();
	H3001_SyncCHR();
	H3001_SyncMirror();
}

static void SetPRG_PNROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_PNROM(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr4(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_PNROM(void) {
	MMC2_pwrap = SetPRG_PNROM;
	MMC2_cwrap = SetCHR_PNROM;
	MMC2_SyncPRG();
	MMC2_SyncCHR();
	MMC2_SyncMirror();
}

static void SetPRG_SKROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask() >> 1;
	uint16_t prgBase = Mapper_GetPRGBase() >> 1;
	setprg16r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_SKROM(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask() >> 2;
	uint16_t chrBase = Mapper_GetCHRBase() >> 2;
	setchr4(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_SKROM(void) {
	MMC1_pwrap = SetPRG_SKROM;
	MMC1_cwrap = SetCHR_SKROM;
	MMC1_SyncPRG();
	MMC1_SyncCHR();
	MMC1_SyncMirror();
}

static void SetPRG_SNROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask() >> 1;
	uint16_t prgBase = Mapper_GetPRGBase() >> 1;
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_SNROM(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask() >> 2;
	uint16_t chrBase = Mapper_GetCHRBase() >> 2;
	setchr4(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_SNROM(void) {
	MMC1_pwrap = SetPRG_SNROM;
	MMC1_cwrap = SetCHR_SNROM;
	MMC1_SyncPRG();
	MMC1_SyncCHR();
	MMC1_SyncMirror();
}

static void SetPRG_SUROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask() >> 1;
	uint16_t prgBase = Mapper_GetPRGBase() >> 1;
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | ((MMC1_GetCHRBank(0) & 0x10 | V) & prgMask));
}

static void SetCHR_SUROM(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask() >> 2;
	uint16_t chrBase = Mapper_GetCHRBase() >> 2;
	setchr4(A, (chrBase & ~chrMask) | (V & chrMask & 0x0F));
}

static void Sync_SUROM(void) {
	MMC1_pwrap = SetPRG_SUROM;
	MMC1_cwrap = SetCHR_SUROM;
	MMC1_SyncPRG();
	MMC1_SyncCHR();
	MMC1_SyncMirror();
}

/*
static void Sync_PT8154(void) {
	PT8154_syncPRG(prgMask, prgBase & ~prgMask);
	PT8154_syncCHR(chrMask, chrBase & ~chrMask);
	PT8154_syncMirror();
}
*/

/*
static void Sync_QJ(void) {
	QJ_syncPRG(prgMask, prgBase & ~prgMask);
	QJ_syncCHR(chrMask, chrBase & ~chrMask);
	QJ_syncMirror();
}
*/

/*
static void Sync_TC3294(void) {
	TC3294_syncWRAM(m446.reg[5]);
	TC3294_syncPRG(prgMask, prgBase & ~prgMask);
	setchr8(0);
	TC3294_syncMirror();
}
*/

static void SetPRG_TxROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_TxROM(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr1(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_TxROM(void) {
	MMC3_pwrap = SetPRG_TxROM;
	MMC3_cwrap = SetCHR_TxROM;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
	MMC3_SyncMirror();
}

static void SetPRG_TxSROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_TxSROM(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr1(A, (chrBase & ~chrMask) | (V & chrMask & 0x7F));
	setmirror(MMC3_GetCHRBank(0) & 0x80 ? MI_1 : MI_0);
}

static void Sync_TxSROM(void) {
	MMC3_pwrap = SetPRG_TxROM;
	MMC3_cwrap = SetCHR_TxROM;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
	MMC3_SyncMirror();
}

static void Sync_UxROM(void) {
	uint16_t prgMask = Mapper_GetPRGMask() >> 1;
	uint16_t prgBase = Mapper_GetCHRBase() >> 1;
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setprg16r(CHIP_FLASH, 0x8000, (latch.data & prgMask) | (prgBase & ~prgMask));
	setprg16r(CHIP_FLASH, 0xC000, prgBase | prgMask);
	setchr8(chrBase);
	setmirror((m446.reg[4] & 0x01) ? MI_V : MI_H);
}

static void SetPRG_VRC1(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC1(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr1(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_VRC1(void) {
	VRC1_pwrap = SetPRG_VRC1;
	VRC1_cwrap = SetPRG_VRC1;
	VRC1_SyncPRG();
	VRC1_SyncCHR();
	VRC1_SyncMirror();
}

static void SetPRG_VRC3(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg16r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC3(uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr8(chrBase);
}

static void Sync_VRC3(void) {
	VRC3_pwrap = SetPRG_VRC3;
	VRC3_cwrap = SetCHR_VRC3;
	VRC3_SyncPRG();
	VRC3_SyncCHR();
	setmirror((m446.reg[4] & 0x01) ? MI_V : MI_H);
}

static void SetPRG_VRC4(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC4(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr1(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void SetCHR_VRC4_22(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	V >>= 1;
	setchr1(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_VRC4(void) {
	VRC24_pwrap = SetPRG_VRC4;
	VRC24_cwrap = SetCHR_VRC4;
	VRC24_SyncPRG();
	VRC24_SyncCHR();
	VRC24_SyncMirror();
}

static void Sync_VRC4_22(void) {
	VRC24_pwrap = SetPRG_VRC4;
	VRC24_cwrap = SetCHR_VRC4_22;
	VRC24_SyncPRG();
	VRC24_SyncCHR();
	VRC24_SyncMirror();
}

static void SetPRG_VRC6(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC6(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr1(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_VRC6(void) {
	VRC6_pwrap = SetPRG_VRC6;
	VRC6_cwrap = SetCHR_VRC6;
	VRC6_SyncPRG();
	VRC6_SyncCHR();
	VRC6_SyncMirror();
}

static void SetPRG_VRC7(uint16_t A, uint16_t V) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8r(CHIP_FLASH, A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC7(uint16_t A, uint16_t V) {
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setchr1(A, (chrBase & ~chrMask) | (V & chrMask));
}

static void Sync_VRC7(void) {
	VRC7_pwrap = SetPRG_VRC7;
	VRC7_cwrap = SetCHR_VRC7;
	VRC7_SyncPRG();
	VRC7_SyncCHR();
	VRC7_mwrap(vrc7.mirr);
}

static void Sync_supervisor(void) {
	uint16_t prgMask = Mapper_GetPRGMask();
	uint16_t prgBase = Mapper_GetPRGBase();
	uint16_t chrMask = Mapper_GetCHRMask();
	uint16_t chrBase = Mapper_GetCHRBase();
	setprg8r(CHIP_FLASH, 0x8000, prgBase);
	setprg8r(CHIP_FLASH, 0xA000, prgBase + 1);
	setprg8r(CHIP_FLASH, 0xC000, (iNESCart.submapper == 3) ? 0x1E : 0x3E);
	setprg8r(CHIP_FLASH, 0xE000, (iNESCart.submapper == 3) ? 0x1F : 0x3F);
	setchr8(chrBase);
	setmirror((m446.reg[4] & 0x01) ? MI_V : MI_H);
}

static void Mapper_SyncWRAM(uint8_t bank) {
	if (PRGsize[CHIP_WRAM]) {
		setprg8r(0x10, 0x6000, bank);
	}
}

static void SetMode(uint8_t clear) {
	if (m446.reg[0] & 0x80) {
		int code = (iNESCart.submapper << 8) | (m446.reg[0] & 0x1F);
		MapIRQHook = NULL;
		GameHBIRQHook = NULL;
		PPU_hook = NULL;
		SetWriteHandler(0x5000, 0x5FFF, CartBW);
		switch (code) {
		case 0x000:
		case 0x100:
		case 0x200:
			mapperSync_cb = Sync_UxROM;
			Latch_SetConfig(clear, Sync);
			break;
		case 0x001:
		case 0x105:
		case 0x205:
			mapperSync_cb = Sync_SKROM;
			MMC1_pwrap = SetPRG_SKROM;
			MMC1_cwrap = SetCHR_SKROM;
			MMC1_SetConfig(clear, MMC1B);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x002:
		case 0x102:
		case 0x202: /* NROM or BNROM */
			mapperSync_cb = Sync_BNROM;
			Latch_SetConfig(clear, Sync);
			break;
		case 0x003:
		case 0x103:
		case 0x203:
			mapperSync_cb = Sync_CNROM;
			Latch_SetConfig(clear, Sync);
			break;
		case 0x004:
		case 0x101:
		case 0x201:
		case 0x209: /* MMC3 or Namco 118 */
			mapperSync_cb = Sync_TxROM;
			MMC3_pwrap = SetPRG_TxROM;
			MMC3_cwrap = SetCHR_TxROM;
			MMC3_SetConfig(clear, MMC3B);
			if (clear) MMC3_Write(0xA000, (m446.reg[4] & 0x04) ? 0 : 1);
			break;
		case 0x10E:
		case 0x20E: /* MMC3 with single-screen mirroring. 239-in-1's Goal! Two has a screen where MMC3 scanline counter emulation fails. */
			mapperSync_cb = Sync_TxSROM;
			MMC3_pwrap = SetPRG_TxSROM;
			MMC3_cwrap = SetCHR_TxSROM;
			MMC3_SetConfig(clear, MMC3B);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x006:
			mapperSync_cb = Sync_VRC4;
			VRC24_pwrap = SetPRG_VRC4;
			VRC24_cwrap = SetCHR_VRC4;
			VRC4_SetConfig(clear, 0x42, 0x84, TRUE);
			break;
		case 0x007:
		case 0x112:
		case 0x212:
			mapperSync_cb = Sync_VRC4_22;
			VRC24_pwrap = SetPRG_VRC4;
			VRC24_cwrap = SetCHR_VRC4_22;
			VRC2_SetConfig(clear, 0x02, 0x01);
			break;
		case 0x008:
		case 0x118:
		case 0x218:
			mapperSync_cb = Sync_VRC4;
			VRC24_pwrap = SetPRG_VRC4;
			VRC24_cwrap = SetCHR_VRC4;
			VRC4_SetConfig(clear, 0x05, 0x0A, TRUE);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x009:
		case 0x110:
			mapperSync_cb = Sync_VRC6;
			VRC6_pwrap = SetPRG_VRC6;
			VRC6_cwrap = SetCHR_VRC6;
			VRC6_SetConfig(clear, 0x01, 0x02);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x00A:
		case 0x115:
		case 0x215:
			mapperSync_cb = Sync_VRC4;
			VRC24_pwrap = SetPRG_VRC4;
			VRC24_cwrap = SetCHR_VRC4;
			VRC4_SetConfig(clear, 0x0A, 0x05, TRUE);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x00B:
			mapperSync_cb = Sync_VRC6;
			VRC6_pwrap = SetPRG_VRC6;
			VRC6_cwrap = SetCHR_VRC6;
			VRC6_SetConfig(clear, 0x02, 0x01);
			break;
		case 0x00C:
			mapperSync_cb = Sync_VRC3;
			VRC3_pwrap = SetPRG_VRC3;
			VRC3_cwrap = SetCHR_VRC3;
			VRC3_SetConfig(clear);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x00D:
			mapperSync_cb = Sync_VRC7;
			VRC7_pwrap = SetPRG_VRC7;
			VRC7_cwrap = SetCHR_VRC7;
			VRC7_SetConfig(clear, 0x18);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x00E:
			mapperSync_cb = Sync_CNROM_Konami;
			Latch_SetConfig(clear, Sync);
			SetWriteHandler(0x6000, 0x7FFF, Latch_Write);
			break;
		case 0x104:
		case 0x204:
			mapperSync_cb = Sync_AxROM;
			Latch_SetConfig(clear, Sync);
			break;
		case 0x106:
		case 0x206:
			mapperSync_cb = Sync_SNROM;
			MMC1_pwrap = SetPRG_SNROM;
			MMC1_cwrap = SetPRG_SNROM;
			MMC1_SetConfig(clear, MMC1B);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x107:
		case 0x208:
			mapperSync_cb = Sync_SUROM;
			MMC1_pwrap = SetPRG_SUROM;
			MMC1_cwrap = SetCHR_SUROM;
			MMC1_SetConfig(clear, MMC1B);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x108:
			mapperSync_cb = Sync_GNROM;
			Latch_SetConfig(clear, Sync);
			break;
		case 0x109:
			mapperSync_cb = Sync_PNROM;
			MMC2_pwrap = SetPRG_PNROM;
			MMC2_cwrap = SetCHR_PNROM;
			MMC2_SetConfig(clear);
			break;
		case 0x10A:
		case 0x20A:
			mapperSync_cb = Sync_TxROM;
			MMC3_pwrap = SetPRG_TxROM;
			MMC3_cwrap = SetCHR_TxROM;
			MMC3_SetConfig(clear, MMC6B);
			Mapper_SyncWRAM(m446.reg[5]);
			break;
		case 0x10B:
		case 0x20B:
			mapperSync_cb = Sync_152;
			Latch_SetConfig(clear, Sync);
			break;
		/* case 0x10F:
			mapperSync_cb = Sync_PT8154;
			PT8154_SetConfig(clear, Sync);
			break; */
		/* case 0x119:
			mapperSync_cb = Sync_QJ;
			QJ_SetConfig(clear, Sync);
			break; */
		case 0x11A:
		case 0x21A:
			mapperSync_cb = Sync_VRC1;
			VRC1_pwrap = SetPRG_VRC1;
			VRC1_cwrap = SetCHR_VRC1;
			VRC1_SetConfig(clear);
			break;
		case 0x301:
			mapperSync_cb = Sync_H3001;
			H3001_pwrap = SetPRG_H3001;
			H3001_cwrap = SetCHR_H3001;
			H3001_SetConfig(clear);
			break;
		/*case 0x401:
			mapperSync_cb = Sync_TC3294;
			TC3294_SetConfig(clear, Sync);
			break;*/
		default:
			break;
		}
	} else {
		SetWriteHandler(0x5000, 0x5FFF, WriteReg);
		SetReadHandler(0x8000, 0xFFFF, ReadFlash);
		SetWriteHandler(0x8000, 0xFFFF, WriteFlash);
		mapperSync_cb = Sync_supervisor;
		PPU_hook = NULL;
		MapIRQHook = FlashROM_CPUCyle;
		GameHBIRQHook = NULL;
		Sync();
	}
}

static DECLFR(ReadFlash) {
	return FlashROM_Read(A);
}

static DECLFW(WriteFlash) {
	FlashROM_Write(A, V);
}

static DECLFW(WriteReg) {
	m446.reg[A & 0x07] = V;
	if ((A & 0x07) == 0) {
		SetMode(1);
	} else {
		Sync();
	}
}

static void Power(void) {
	memset(&m446, 0, sizeof(m446));
	SetMode(1);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
}

static void StateRestore(int version) {
	SetMode(0);
}

void Mapper446_Init(CartInfo *info) {
	int ws = info-> iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : (32 * 1024);
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	MMC1_Init(info, MMC1B, FALSE, FALSE);
	MMC2_Init(info, FALSE, FALSE);
	MMC3_Init(info, MMC1B, FALSE, FALSE);
	VRC1_Init(info);
	VRC24_Init(info, VRC24_VRC2, 0x01, 0x02, FALSE, FALSE);
	VRC3_Init(info);
	VRC6_Init(info, 0x01, 0x02, FALSE);
	H3001_Init(info);
	info->Reset = Power;
	info->Power = Power;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
	WRAMSIZE = ws;
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
		SetupCartPRGMapping(CHIP_WRAM, WRAM, WRAMSIZE, TRUE);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	}
	SetupCartPRGMapping(CHIP_FLASH, PRGptr[0], PRGsize[0], TRUE);
	FlashROM_Init(PRGptr[0], PRGsize[0], 0x01, 0x7E, SIZE_128K, 0xAAA, 0x555);
}
