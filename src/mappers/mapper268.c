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

/*
 * Submapper(s)	PCB Name(s)							Notes
 * ============================================================================================================================================
 * 0/1			COOLBOY/MINDKIDS/KT-008				CHR-RAM only, max. 32 MiB PRG-ROM
 *				YH2018A								CHR-RAM only, max. 64 MiB PRG-ROM
 *				JTH-813								CHR-ROM only, max. 8 MiB PRG-ROM, max. 1 MiB CHR-ROM
 *				KT-???								CHR-ROM+CHR-RAM; uses the ASIC's mixed CHR-ROM/RAM functionality selected by register $xxx4
 * 2/3			GH2009_V01/SMD173C_60/SMD173C_L1	Later revision of 0/1. Die version AA6023B, chip still labelled as SMD133
 * 4/5			KP-6022, LD622D						Max. 4 MiB PRG-ROM
 * 6/7			J-852C								Max. 128 KiB of CHR-RAM, as CHR A17 selects between two PRG-ROM chips
 * 8/9			SMD72A_V5S_V01						Max. 2 MiB PRG-ROM, Max. 256 KiB CHR-RAM that can be write-protected
 * 10/11		SMD172C-L1							Max. 8 MiB PRG-ROM, with single-screen mirroring
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[8];
} m268;

static void SetPRGBank_mmc3(uint16_t A, uint16_t V) {
	uint8_t chip = 0;
	uint8_t maskGnrom = 0;
	uint8_t gnromMode = (m268.reg[3] & 0x10) != 0;
	uint16_t maskMmc3 = (gnromMode ? 0x00 : 0x0F) /* PRG A13-A16 */
	    | ((~m268.reg[0] >> 2) & 0x10)          /* PRG A17     */
	    | ((~m268.reg[1] >> 2) & 0x20)          /* PRG A18     */
	    | ((m268.reg[1] >> 0) & 0x40)           /* PRG A19     */
	    | ((m268.reg[1] << 2) & 0x80)           /* PRG A20     */
	    ;
	uint16_t base = 0;

	switch (iNESCart.submapper & ~1) {
	default: /* Original implementation */
		maskGnrom = (gnromMode ? ((m268.reg[1] & 0x02) ? 0x03 : 0x01) : 0x00);
		base = (m268.reg[3] & 0x0E)
		    | ((m268.reg[0] << 4) & 0x70)
		    | ((m268.reg[1] << 3) & 0x80)
		    | ((m268.reg[1] << 6) & 0x300)
		    | ((m268.reg[0] << 6) & 0xC00)
			| ((~m268.reg[1] << 12) & 0x1000);
		break;
	case 2: /* Later revision with different arrangement of register 1 */
		maskGnrom = (gnromMode ? ((m268.reg[1] & 0x10) ? 0x01 : 0x03) : 0x00);
		base = (m268.reg[3] & 0x0E)
		    | ((m268.reg[0] << 4) & 0x70)
		    | ((m268.reg[1] << 4) & 0x80)
		    | ((m268.reg[1] << 6) & 0x100)
		    | ((m268.reg[1] << 8) & 0x200)
		    | ((m268.reg[0] << 6) & 0xC00);
		break;
	case 4: /* LD622D: PRG A20-21 moved to register 0 */
		maskGnrom = (gnromMode ? ((m268.reg[1] & 0x02) ? 0x03 : 0x01) : 0x00);
		base = (m268.reg[3] & 0x0E)
		    | ((m268.reg[0] << 4) & 0x70)
		    | ((m268.reg[0] << 3) & 0x180);
		break;
	case 6: /* J-852C: CHR A17 selects between two PRG chips */
		maskGnrom = (gnromMode ? ((m268.reg[1] & 0x02) ? 0x03 : 0x01) : 0x00);
		base = (m268.reg[3] & 0x0E)
		    | ((m268.reg[0] << 4) & 0x70)
		    | ((m268.reg[1] << 3) & 0x80)
		    | ((m268.reg[1] << 6) & 0x300)
		    | ((m268.reg[0] << 6) & 0xC00);
		base &= (PRG_BANK_COUNT(16) - 1);
		if (m268.reg[0] & 0x80) {
			chip = (m268.reg[0] & 0x08) != 0;
		} else {
			chip = (MMC3_GetPRGBank(0) & 0x80) != 0;
		}
		if (chip) {
			base |= PRG_BANK_COUNT(16);
		}
		break;
	}

	setprg8(A, (base & ~(maskMmc3 | maskGnrom)) | (V & maskMmc3) | ((A >> 13) & maskGnrom));

	if (m268.reg[3] & 0x40) {
		/* incomplete MMC4-like weird mode */
		if (!(mmc3.cmd & 0x40)) {
			setprg8(0xC000, (base & ~(maskMmc3 | maskGnrom)) | (2 & maskGnrom));
			setprg8(0xE000, (base & ~(maskMmc3 | maskGnrom)) | (3 & maskGnrom));
		}
	}

	if (mmc3.wram & 0x20) {
		/* Hack for FS005 games on Mindkids board that only work with emulation. */
		mmc3.wram &= ~0x40;
	}
}

static void SetCHRBank_mmc3(uint16_t A, uint16_t V) {
	uint16_t base = ((m268.reg[0] << 4) & 0x380) | ((m268.reg[2] << 3) & 0x078);
	uint8_t gnromMode = (m268.reg[3] & 0x10) != 0;
	uint8_t maskMmc3 = (gnromMode ? 0x00 : ((m268.reg[0] & 0x80) ? 0x7F : 0xFF));
	uint8_t maskGnrom = (gnromMode ? 0x07 : 0x00);

	/* CHR-RAM write protect on submapper 8/9) */
	if ((iNESCart.submapper & ~1) == 8) {
		if (m268.reg[0] & 0x10) {
			SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], 0);
		} else {
			SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], 1);
		}
	}

	if (CHRRAM && (m268.reg[4] & 0x01) && ((V & 0xFE) == (m268.reg[4] & 0xFE))) {
		setchr1r(0x10, A, ((base & ~(maskMmc3 | maskGnrom)) | (V & maskMmc3) | ((A >> 10) & maskGnrom)));
	} else {
		setchr1(A, ((base & ~(maskMmc3 | maskGnrom)) | (V & maskMmc3) | ((A >> 10) & maskGnrom)));
	}

	if (m268.reg[3] & 0x40) {
		/* incomplete MMC4-like weird mode */
		uint16_t cswap = (mmc3.cmd & 0x80) << 5;

		setchr1(0x0000 ^ cswap, ((base & ~(maskMmc3 | maskGnrom)) | (mmc3.reg[0] & maskMmc3) | (0 & maskGnrom)));
		setchr1(0x0400 ^ cswap, ((base & ~(maskMmc3 | maskGnrom)) | (0 & maskMmc3) | (1 & maskGnrom)));
		setchr1(0x0800 ^ cswap, ((base & ~(maskMmc3 | maskGnrom)) | (mmc3.reg[1] & maskMmc3) | (2 & maskGnrom)));
		setchr1(0x0C00 ^ cswap, ((base & ~(maskMmc3 | maskGnrom)) | (0 & maskMmc3) | (3 & maskGnrom)));
	}
}

static void SyncMirror(void) {
	if (!(m268.reg[0] & 0x20)) {
		switch (iNESCart.submapper) {
		case 10:
		case 11:
			setmirror(MI_0 + ((m268.reg[0] >> 4) & 0x01));
			break;
		default:
			setmirror((mmc3.mirr & 0x01) ^ 0x01);
			break;
		}
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFR(ReadWRAM) {
	if (mmc3.wram & 0xA0) {
		return CartBR(A);
	}
	return cpu.openbus;
}

static DECLFW(WriteWRAM) {
	if (MMC3_WramIsWritable() || (mmc3.wram & 0x20)) {
		CartBW(A, V);
	}
}

static DECLFW(WriteReg) {
	uint8_t index = A & 0x07;

	if (A >= 0x6000) {
		WriteWRAM(A, V);
	}

	if (!(m268.reg[3] & 0x80) || (index == 2)) {
		if (index == 2) {
			if (m268.reg[2] & 0x80) {
				V = (V & 0x0F) | (m268.reg[2] & ~0x0F);
			}
			V &= (0xF1 | ((~m268.reg[2] >> 3) & 0x0E));
		}
		m268.reg[index] = V;
		MMC3_SyncPRG();
		MMC3_SyncCHR();
		MMC3_SyncMirror();
	}
}

static void Reset(void) {
	memset(&m268, 0, sizeof(m268));
	MMC3_Reset();
}

static void Power(void) {
	uint16_t startAddr = (iNESCart.submapper & 0x01) ? 0x5000 : 0x6000;
	uint16_t endAddr = (iNESCart.submapper & 0x01) ? 0x5FFF : 0x7FFF;

	memset(&m268, 0, sizeof(m268));
	MMC3_Power();
	SetReadHandler(0x6000, 0x7FFF, ReadWRAM);
	SetWriteHandler(0x6000, 0x7FFF, WriteWRAM);
	SetWriteHandler(startAddr, endAddr, WriteReg);
}

static void Close(void) {
	MMC3_Close();
}

static void Common_Init(CartInfo *info) {
	int ws = info->PRGRamSize + info->PRGRamSaveSize;

	MMC3_Init(info, MMC3B, ws / 1024, info->battery);
	MMC3_SyncMirror = SyncMirror;
	MMC3_pwrap = SetPRGBank_mmc3;
	MMC3_cwrap = SetCHRBank_mmc3;

	info->Power = Power;
	info->Reset = Reset;
	info->Close = Close;

	AddExState(m268.reg, 8, 0, "EXPR");

	if (ROM.chr.size) {
		CHRRAMSIZE = info->CHRRamSize + info->CHRRamSaveSize;
		if (CHRRAMSIZE) {
			CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
			SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
			AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
		}
	}
}

void Mapper268_Init(CartInfo *info) {
	Common_Init(info);
}

/* UNIF loader */

void COOLBOY_Init(CartInfo *info) {
	info->submapper = 0;
	info->PRGRamSize = 8192;
	Common_Init(info);
}

void MINDKIDS_Init(CartInfo *info) { /* mapper 224 */
	info->submapper = 1;
	info->PRGRamSize = 8192;
	Common_Init(info);
}
