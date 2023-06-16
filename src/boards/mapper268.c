/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2023
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

static uint8 *CHRRAM = NULL;
static uint32 CHRRAMSize = 0;
static uint8 submapper;

static void M268CW(uint32 A, uint8 V) {
	uint16 base = ((mmc3.expregs[0] << 4) & 0x380) | ((mmc3.expregs[2] << 3) & 0x078);
	uint16 mask = (mmc3.expregs[0] & 0x80) ? 0x7F : 0xFF;
	uint8  ram  = CHRRAM && (mmc3.expregs[4] & 0x01) && ((V & 0xFE) == (mmc3.expregs[4] & 0xFE));
	/* CHR-RAM write protect on submapper 8/9) */
	if ((submapper & ~1) == 8) {
		if (mmc3.expregs[0] & 0x10) {
			SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], 0);
		} else {
			SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], 1);
		}
	}
	if (mmc3.expregs[3] & 0x10) {
		/* GNROM */
		mask = 0x07;
		V = A >> 10;
	}
	setchr1r(ram ? 0x10 : 0x00, A, (base & ~mask) | (V & mask));
}

static void M268PW(uint32 A, uint8 V) {
	uint16 base;
	uint16 mask = 0x0F						/* PRG A13-A16 */
	    | ((~mmc3.expregs[0] >> 2) & 0x10)	/* PRG A17     */
	    | ((~mmc3.expregs[1] >> 2) & 0x20)	/* PRG A18     */
	    | ((mmc3.expregs[1] >> 0) & 0x40)	/* PRG A19     */
	    | ((mmc3.expregs[1] << 2) & 0x80)	/* PRG A20     */
	    ;
	switch (submapper & ~1) {
	default: /* Original implementation */
		base = (mmc3.expregs[3] & 0x0E) | ((mmc3.expregs[0] << 4) & 0x70) | ((mmc3.expregs[1] << 3) & 0x80) |
		    ((mmc3.expregs[1] << 6) & 0x300) | ((mmc3.expregs[0] << 6) & 0xC00);
		break;
	case 2: /* Later revision with different arrangement of register 1 */
		base = (mmc3.expregs[3] & 0x0E) | ((mmc3.expregs[0] << 4) & 0x70) | ((mmc3.expregs[1] << 4) & 0x80) |
		    ((mmc3.expregs[1] << 6) & 0x100) | ((mmc3.expregs[1] << 8) & 0x200) | ((mmc3.expregs[0] << 6) & 0xC00);
		break;
	case 4: /* LD622D: PRG A20-21 moved to register 0 */
		base = (mmc3.expregs[3] & 0x0E) | ((mmc3.expregs[0] << 4) & 0x70) | ((mmc3.expregs[0] << 3) & 0x180);
		break;
	case 6: /* J-852C: CHR A17 selects between two PRG chips */
		base = (mmc3.expregs[3] & 0x0E) | ((mmc3.expregs[0] << 4) & 0x70) | ((mmc3.expregs[1] << 3) & 0x80) |
		    ((mmc3.expregs[1] << 6) & 0x300) | ((mmc3.expregs[0] << 6) & 0xC00);
		base &= ROM_size - 1;
		if ((mmc3.expregs[0] & 0x80) ? !!(mmc3.expregs[0] & 0x08) : !!(mmc3.regs[0] & 0x80)) {
			base |= ROM_size;
		}
		break;
	}
	if (mmc3.expregs[3] & 0x10) {
		/* GNROM */
		switch (submapper & ~1) {
		default:
			mask = (mmc3.expregs[1] & 0x02) ? 0x03 : 0x01;
			break;
		case 2:
			mask = (mmc3.expregs[1] & 0x10) ? 0x01 : 0x03;
			break;
		}
		V = A >> 13;
	}
	setprg8(A, (base & ~mask) | (V & mask));
}

static DECLFR(M268WramRead) {
	if (mmc3.wram & 0xA0) {
		return CartBR(A);
	}
	return X.DB;
}

static DECLFW(M268WramWrite) {
	if (MMC3CanWriteToWRAM() || (mmc3.wram & 0x20)) {
		CartBW(A, V);
	}
}

static DECLFW(M268Write) {
	uint8 index = A & 0x07;
	if (~submapper & 0x01) {
		M268WramWrite(A, V);
	}
	if ((~mmc3.expregs[3] & 0x80) || (index == 2)) {
		if (index == 2) {
			if (mmc3.expregs[2] & 0x80) {
				V = (V & 0x0F) | (mmc3.expregs[2] & ~0x0F);
			}
			V &= ((~mmc3.expregs[2] >> 3) & 0x0E) | 0xF1;
		}
		mmc3.expregs[index] = V;
		FixMMC3PRG();
		FixMMC3CHR();
	}
}

static void M268Reset(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0x00;
	mmc3.expregs[4] = mmc3.expregs[5] = mmc3.expregs[6] = mmc3.expregs[7] = 0x00;
	MMC3RegReset();
}

static void M268Power(void) {
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0x00;
	mmc3.expregs[4] = mmc3.expregs[5] = mmc3.expregs[6] = mmc3.expregs[7] = 0x00;
	GenMMC3Power();
	SetReadHandler(0x6000, 0x7FFF, M268WramRead);
	if (submapper & 1) {
		SetWriteHandler(0x5000, 0x5FFF, M268Write);
		SetWriteHandler(0x6000, 0x7FFF, M268WramWrite);
	} else {
		SetWriteHandler(0x6000, 0x7FFF, M268Write);
	}
}

static void M268Close(void) {
	GenMMC3Close();
	if (CHRRAM) {
		FCEU_gfree(CHRRAM);
	}
	CHRRAM = NULL;
}

static void ComminInit(CartInfo *info, int _submapper) {
	GenMMC3_Init(info, (info->PRGRamSize + info->PRGRamSaveSize) >> 10, info->battery);
	submapper = _submapper;
	mmc3.pwrap = M268PW;
	mmc3.cwrap = M268CW;
	info->Power = M268Power;
	info->Reset = M268Reset;
	info->Close = M268Close;
	CHRRAMSize = info->CHRRamSize + info->CHRRamSaveSize;
	if (!UNIFchrrama && CHRRAMSize) {
		CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSize);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSize, 1);
		AddExState(CHRRAM, CHRRAMSize, 0x00, "CRAM");
	}
	AddExState(mmc3.expregs, 8, 0x00, "EXPR");
}

void COOLBOY_Init(CartInfo *info) {
	ComminInit(info, 0);
}

void MINDKIDS_Init(CartInfo *info) {
	ComminInit(info, 1);
}

void Mapper268_Init(CartInfo *info) {
	ComminInit(info, info->submapper);
}
