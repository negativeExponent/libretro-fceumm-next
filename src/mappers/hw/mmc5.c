/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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
 */

/* None of this code should use any of the iNES bank switching wrappers. */

#include "mapinc.h"
#include "mmc5.h"
#include "mmc5sound.h"

static INLINE void MMC5SPRVROM_BANK1(uint32_t A, uint32_t V) {
	if (CHRptr[0]) {
		V &= CHRmask1[0];
		MMC5SPRVPage[A >> 10] = &CHRptr[0][V << 10] - A;
	}
}

static INLINE void MMC5BGVROM_BANK1(uint32_t A, uint32_t V) {
	if (CHRptr[0]) {
		V &= CHRmask1[0];
		MMC5BGVPage[A >> 10] = &CHRptr[0][V << 10] - A;
	}
}

static INLINE void MMC5SPRVROM_BANK2(uint32_t A, uint32_t V) {
	if (CHRptr[0]) {
		V &= CHRmask2[0];
		MMC5SPRVPage[A >> 10] = MMC5SPRVPage[(A >> 10) + 1] = &CHRptr[0][V << 11] - A;
	}
}

static INLINE void MMC5BGVROM_BANK2(uint32_t A, uint32_t V) {
	if (CHRptr[0]) {
		V &= CHRmask2[0];
		MMC5BGVPage[A >> 10] = MMC5BGVPage[(A >> 10) + 1] = &CHRptr[0][V << 11] - A;
	}
}

static INLINE void MMC5SPRVROM_BANK4(uint32_t A, uint32_t V) {
	if (CHRptr[0]) {
		V &= CHRmask4[0];
		MMC5SPRVPage[A >> 10] = MMC5SPRVPage[(A >> 10) + 1] = MMC5SPRVPage[(A >> 10) + 2] = MMC5SPRVPage[(A >> 10) + 3] = &CHRptr[0][V << 12] - A;
	}
}

static INLINE void MMC5BGVROM_BANK4(uint32_t A, uint32_t V) {
	if (CHRptr[0]) {
		V &= CHRmask4[0];
		MMC5BGVPage[A >> 10] = MMC5BGVPage[(A >> 10) + 1] = MMC5BGVPage[(A >> 10) + 2] = MMC5BGVPage[(A >> 10) + 3] = &CHRptr[0][V << 12] - A;
	}
}

static INLINE void MMC5SPRVROM_BANK8(uint32_t V) {
	if (CHRptr[0]) {
		V &= CHRmask8[0];
		MMC5SPRVPage[0] = MMC5SPRVPage[1] = MMC5SPRVPage[2] = MMC5SPRVPage[3] = MMC5SPRVPage[4] = MMC5SPRVPage[5] = MMC5SPRVPage[6] = MMC5SPRVPage[7] = &CHRptr[0][V << 13];
	}
}

static INLINE void MMC5BGVROM_BANK8(uint32_t V) {
	if (CHRptr[0]) {
		V &= CHRmask8[0];
		MMC5BGVPage[0] = MMC5BGVPage[1] = MMC5BGVPage[2] = MMC5BGVPage[3] = MMC5BGVPage[4] = MMC5BGVPage[5] = MMC5BGVPage[6] = MMC5BGVPage[7] = &CHRptr[0][V << 13];
	}
}

enum { MODE0, MODE1, MODE2, MODE3 };

MMC5 mmc5;

static SFORMAT MMC5_StateRegs[] = {
	{ mmc5.prg, 5, "PREG" },
	{ &mmc5.chr[0], 2 | FCEUSTATE_RLSB, "M5C0" },
	{ &mmc5.chr[1], 2 | FCEUSTATE_RLSB, "M5C1" },
	{ &mmc5.chr[2], 2 | FCEUSTATE_RLSB, "M5C2" },
	{ &mmc5.chr[3], 2 | FCEUSTATE_RLSB, "M5C3" },
	{ &mmc5.chr[4], 2 | FCEUSTATE_RLSB, "M5C4" },
	{ &mmc5.chr[5], 2 | FCEUSTATE_RLSB, "M5C5" },
	{ &mmc5.chr[6], 2 | FCEUSTATE_RLSB, "M5C6" },
	{ &mmc5.chr[7], 2 | FCEUSTATE_RLSB, "M5C7" },
	{ &mmc5.chr[8], 2 | FCEUSTATE_RLSB, "M5C8" },
	{ &mmc5.chr[9], 2 | FCEUSTATE_RLSB, "M5C9" },
	{ &mmc5.chr[10], 2 | FCEUSTATE_RLSB, "M5CA" },
	{ &mmc5.chr[11], 2 | FCEUSTATE_RLSB, "M5CB" },
	{ mmc5.wramProtect, 2, "WRMK" },
	{ mmc5.mul, 2, "MUL0" },

	{ &mmc5.prgMode, 1, "PRGM" },
	{ &mmc5.chrMode, 1, "CHRM" },
	{ &mmc5.chrLast, 1, "CHRL" },
	{ &mmc5.extMode, 1, "EXTM" },
	{ &mmc5.nmt, 1, "NTAM" },

	{ &mmc5.fillTile, 1, "NTFL" },
	{ &mmc5.fillColor, 1, "ATFL" },
	{ &mmc5.irq.inFrame, 1, "IFRM" },

	{ &mmc5.irq.enabled, 1, "IRQE" },
	{ &mmc5.irq.pending, 1, "IRQP" },
	{ &mmc5.irq.scanlineTarget, 1, "IRQS" },
	{ &mmc5.irq.scanlineCounter, 1, "LCTR" },

	{ 0 }
};

uint8_t *MMC5BGVRAMADR(uint32_t A) {
	if (newppu) {
		if (Sprite16) {
			bool isPattern = PPUON != 0;
			if ((ppuphase == PPUPHASE_OBJ) && isPattern) {
				return &ABANKS[(A) >> 10][(A)];
			}
			if ((ppuphase == PPUPHASE_BG) && isPattern) {
				return &BBANKS[(A) >> 10][(A)];
			} else if (mmc5.chrLast < 8) {
				return &ABANKS[(A) >> 10][(A)];
			} else {
				return &BBANKS[(A) >> 10][(A)];
			}
		} else {
			return &ABANKS[(A) >> 10][(A)];
		}
	}

	if (!Sprite16) {
		if (mmc5.chrLast < 8) {
			return &ABANKS[(A) >> 10][(A)];
		} else {
			return &BBANKS[(A) >> 10][(A)];
		}
	} else {
		return &BBANKS[(A) >> 10][(A)];
	}
}

extern uint8_t PALRAM[0x20];
extern uint8_t UPALRAM[0x03];
extern uint32_t NTRefreshAddr;

static void mmc5_PPUWrite(uint32_t A, uint8_t V) {
	uint32_t tmp = A;

	if (tmp >= 0x3F00) {
		if (!(tmp & 0x03)) {
			if (!(tmp & 0x0C)) {
				PALRAM[0x00] = PALRAM[0x04] = PALRAM[0x08] = PALRAM[0x0C] = V & 0x3F;
				PALRAM[0x10] = PALRAM[0x14] = PALRAM[0x18] = PALRAM[0x1C] = V & 0x3F;
			} else {
				UPALRAM[((tmp & 0x0C) >> 2) - 1] = V & 0x3F;
			}
		} else {
			PALRAM[tmp & 0x1F] = V & 0x3F;
		}
	} else if (tmp < 0x2000) {
		if (PPUCHRRAM & (1 << (tmp >> 10))) {
			VPage[tmp >> 10][tmp] = V;
		}
	} else {
		if (PPUNTARAM & (1 << ((tmp & 0xF00) >> 10))) {
			vnapage[((tmp & 0xF00) >> 10)][tmp & 0x03FF] = V;
		}
	}
}

static uint8_t mmc5_PPURead(uint32_t A) {
	bool split = false;

	if (newppu) {
		if ((MMC5HackSPMode & 0x80) && !(MMC5HackCHRMode & MODE2)) {
			int target = MMC5HackSPMode & 0x1F;
			int side = MMC5HackSPMode & 0x40;
			int ht = NTRefreshAddr & 0x1F;

			if (side == 0) {
				if (ht < target) {
					split = true;
				}
			} else {
				if (ht >= target) {
					split = true;
				}
			}
		}
	}

	if (A < 0x2000) {
		if (Sprite16) {
			bool isPattern = !!PPUON;

			if ((ppuphase == PPUPHASE_OBJ) && isPattern) {
				return ABANKS[(A) >> 10][(A)];
			}
			if ((ppuphase == PPUPHASE_BG) && isPattern) {
				if (split) {
					return MMC5HackVROMPTR[MMC5HackSPPage * 0x1000 + (A & 0xFFF)];
				}

				/* uhhh call through to this more sophisticated function, only
				 * if it's really needed? we should probably reuse it
				 * completely, if we can
				 */
				if (MMC5HackCHRMode == MODE1) {
					return *FCEUPPU_GetCHR(A, NTRefreshAddr);
				}

				return BBANKS[(A) >> 10][(A)];
			} else if (mmc5.chrLast < 8) {
				return ABANKS[(A) >> 10][(A)];
			} else {
				return BBANKS[(A) >> 10][(A)];
			}
		} else {
			if ((ppuphase == PPUPHASE_BG) && ScreenON) {
				if (split) {
					return MMC5HackVROMPTR[MMC5HackSPPage * 0x1000 + (A & 0xFFF)];
				}

				/* uhhh call through to this more sophisticated function, only
				 * if it's really needed? we should probably reuse it
				 * completely, if we can
				 */
				if (MMC5HackCHRMode == MODE1) {
					return *FCEUPPU_GetCHR(A, NTRefreshAddr);
				}
			}

			return ABANKS[(A) >> 10][(A)];
		}
	} else {
		if (split) {
			static const int kHack = -1; /* dunno if theres science to this or if it just fixes SDF
			           (cant be bothered to think about it) */
			int linetile = (newppu_get_scanline() + kHack) / 8 + MMC5HackSPScroll;

			/* REF NT: return 0x2000 | (v << 0x0B) | (h << 0xA) | (vt << 5) | ht;
			 REF AT: return 0x2000 | (v << 0x0B) | (h << 0xA) | 0x3C0 | ((vt &
			 0x1C) << 1) | ((ht & 0x1C) >> 2); */

			if ((A & 0x03FF) >= 0x3C0) {
				A &= ~(0x1C << 1);           /* mask off VT */
				A |= (linetile & 0x1C) << 1; /* mask on adjusted VT */
				return mmc5.exRam[A & 0x03FF];
			} else {
				A &= ~((0x1F << 5) | (1 << 0x0B)); /* mask off VT and V */
				A |= (linetile & 31) << 5;         /* mask on adjusted VT (V doesnt make */
				                                   /* any sense, I think) */
				return mmc5.exRam[A & 0x03FF];
			}
		}

		if (MMC5HackCHRMode == MODE1) {
			if ((A & 0x03FF) >= 0x3C0) {
				uint8_t byte = mmc5.exRam[NTRefreshAddr & 0x03FF];

				/* get attribute part and paste it 4x across the byte */
				byte >>= 6;
				byte *= 0x55;
				return byte;
			}
		}

		return vnapage[(A >> 10) & 0x03][A & 0x03FF];
	}
}

static void MMC5CHRA(void) {
	int x;
	switch (mmc5.chrMode) {
	case MODE0:
		setchr8(mmc5.chr[7]);
		MMC5SPRVROM_BANK8(mmc5.chr[7]);
		break;
	case MODE1:
		setchr4(0x0000, mmc5.chr[3]);
		setchr4(0x1000, mmc5.chr[7]);
		MMC5SPRVROM_BANK4(0x0000, mmc5.chr[3]);
		MMC5SPRVROM_BANK4(0x1000, mmc5.chr[7]);
		break;
	case MODE2:
		setchr2(0x0000, mmc5.chr[1]);
		setchr2(0x0800, mmc5.chr[3]);
		setchr2(0x1000, mmc5.chr[5]);
		setchr2(0x1800, mmc5.chr[7]);
		MMC5SPRVROM_BANK2(0x0000, mmc5.chr[1]);
		MMC5SPRVROM_BANK2(0x0800, mmc5.chr[3]);
		MMC5SPRVROM_BANK2(0x1000, mmc5.chr[5]);
		MMC5SPRVROM_BANK2(0x1800, mmc5.chr[7]);
		break;
	case MODE3:
		setchr1(0x0000, mmc5.chr[0]);
		setchr1(0x0400, mmc5.chr[1]);
		setchr1(0x0800, mmc5.chr[2]);
		setchr1(0x0C00, mmc5.chr[3]);
		setchr1(0x1000, mmc5.chr[4]);
		setchr1(0x1400, mmc5.chr[5]);
		setchr1(0x1800, mmc5.chr[6]);
		setchr1(0x1C00, mmc5.chr[7]);
		MMC5SPRVROM_BANK1(0x0000, mmc5.chr[0]);
		MMC5SPRVROM_BANK1(0x0400, mmc5.chr[1]);
		MMC5SPRVROM_BANK1(0x0800, mmc5.chr[2]);
		MMC5SPRVROM_BANK1(0x0C00, mmc5.chr[3]);
		MMC5SPRVROM_BANK1(0x1000, mmc5.chr[4]);
		MMC5SPRVROM_BANK1(0x1400, mmc5.chr[5]);
		MMC5SPRVROM_BANK1(0x1800, mmc5.chr[6]);
		MMC5SPRVROM_BANK1(0x1C00, mmc5.chr[7]);
		break;
	}
}

static void MMC5CHRB(void) {
	int x;
	switch (mmc5.chrMode) {
	case MODE0:
		setchr8(mmc5.chr[11]);
		MMC5BGVROM_BANK8(mmc5.chr[11]);
		break;
	case MODE1:
		setchr4(0x0000, mmc5.chr[11]);
		setchr4(0x1000, mmc5.chr[11]);
		MMC5BGVROM_BANK4(0x0000, mmc5.chr[11]);
		MMC5BGVROM_BANK4(0x1000, mmc5.chr[11]);
		break;
	case MODE2:
		setchr2(0x0000, mmc5.chr[9]);
		setchr2(0x0800, mmc5.chr[11]);
		setchr2(0x1000, mmc5.chr[9]);
		setchr2(0x1800, mmc5.chr[11]);
		MMC5BGVROM_BANK2(0x0000, mmc5.chr[9]);
		MMC5BGVROM_BANK2(0x0800, mmc5.chr[11]);
		MMC5BGVROM_BANK2(0x1000, mmc5.chr[9]);
		MMC5BGVROM_BANK2(0x1800, mmc5.chr[11]);
		break;
	case MODE3:
		setchr1(0x0000, mmc5.chr[8]);
		setchr1(0x0400, mmc5.chr[9]);
		setchr1(0x0800, mmc5.chr[10]);
		setchr1(0x0C00, mmc5.chr[11]);
		setchr1(0x1000, mmc5.chr[8]);
		setchr1(0x1400, mmc5.chr[9]);
		setchr1(0x1800, mmc5.chr[10]);
		setchr1(0x1C00, mmc5.chr[11]);
		MMC5BGVROM_BANK1(0x0000, mmc5.chr[8]);
		MMC5BGVROM_BANK1(0x0400, mmc5.chr[9]);
		MMC5BGVROM_BANK1(0x0800, mmc5.chr[10]);
		MMC5BGVROM_BANK1(0x0C00, mmc5.chr[11]);
		MMC5BGVROM_BANK1(0x1000, mmc5.chr[8]);
		MMC5BGVROM_BANK1(0x1400, mmc5.chr[9]);
		MMC5BGVROM_BANK1(0x1800, mmc5.chr[10]);
		MMC5BGVROM_BANK1(0x1C00, mmc5.chr[11]);
		break;
	}
}

static void MMC5_SyncCHR(void) {
	if (mmc5.chrLast < 8) {
		MMC5CHRB();
		MMC5CHRA();
	} else {
		MMC5CHRA();
		MMC5CHRB();
	}
}

static void MMC5PWRAP(uint16_t A, uint16_t V) {
	int chip = (V & 0x80) ? 0 : 0x10; /*wrom : wram */
	setprg8r(chip, A, V);
}

static void MMC5_SyncPRG(void) {
	int x;
	setprg8r(0x10, 0x6000, mmc5.prg[0]);
	switch (mmc5.prgMode) {
	case MODE0:
		setprg32(0x8000, (mmc5.prg[4]) >> 2);
		break;
	case MODE1:
		MMC5PWRAP(0x8000, mmc5.prg[2] & 0xFE);
		MMC5PWRAP(0xA000, (mmc5.prg[2] & 0xFE) + 1);
		setprg16(0xC000, (mmc5.prg[4]) >> 1);
		break;
	case MODE2:
		MMC5PWRAP(0x8000, mmc5.prg[2] & 0xFE);
		MMC5PWRAP(0xA000, (mmc5.prg[2] & 0xFE) + 1);
		MMC5PWRAP(0xC000, mmc5.prg[3]);
		setprg8(0xE000, mmc5.prg[4]);
		break;
	case MODE3:
		MMC5PWRAP(0x8000, mmc5.prg[1]);
		MMC5PWRAP(0xA000, mmc5.prg[2]);
		MMC5PWRAP(0xC000, mmc5.prg[3]);
		setprg8(0xE000, mmc5.prg[4]);
		break;
	}
}

static void MMC5_SyncMirror(void) {
	int x;
	for (x = 0; x < 4; x++) {
		switch ((mmc5.nmt >> (x << 1)) & 0x03) {
		case 0:
			setntamem(NTARAM + 0x000, TRUE, x);
			break;
		case 1:
			setntamem(NTARAM + 0x400, TRUE, x);
			break;
		case 2:
			setntamem(mmc5.exRam, TRUE, x);
			break;
		case 3:
			setntamem(mmc5.fillTable, FALSE, x);
			break;
		}
	}
}

DECLFW(Mapper5_write) {
	switch (A) {
	case 0x5100:
		mmc5.prgMode = V & 0x03;
		MMC5_SyncPRG();
		break;
	case 0x5101:
		mmc5.chrMode = V & 0x03;
		MMC5_SyncCHR();
		break;
	case 0x5102:
		mmc5.wramProtect[0] = V & 0x03;
		break;
	case 0x5103:
		mmc5.wramProtect[1] = V & 0x03;
		break;
	case 0x5104:
		MMC5HackCHRMode = mmc5.extMode = V & 0x03;
		MMC5_SyncCHR();
		break;
	case 0x5105:
		mmc5.nmt = V;
		MMC5_SyncMirror();
		break;
	case 0x5106:
		if (V != mmc5.fillTile) {
			FCEU_dwmemset32(mmc5.fillTable, (V | (V << 8) | (V << 16) | (V << 24)), 0x3c0);
		}
		mmc5.fillTile = V;
		break;
	case 0x5107:
		if (V != mmc5.fillColor) {
			unsigned char moop = V | (V << 2) | (V << 4) | (V << 6);
			FCEU_dwmemset32(mmc5.fillTable + 0x3c0, moop | (moop << 8) | (moop << 16) | (moop << 24), 0x40);
		}
		mmc5.fillColor = V;
		break;
	case 0x5113:
	case 0x5114:
	case 0x5115:
	case 0x5116:
	case 0x5117:
		mmc5.prg[A - 0x5113] = V;
		MMC5_SyncPRG();
		break;
	case 0x5120:
	case 0x5121:
	case 0x5122:
	case 0x5123:
	case 0x5124:
	case 0x5125:
	case 0x5126:
	case 0x5127:
	case 0x5128:
	case 0x5129:
	case 0x512a:
	case 0x512b:
		mmc5.chrLast = (A - 0x5120);
		mmc5.chr[mmc5.chrLast] = V | ((MMC50x5130 & 0x03) << 8);
		MMC5_SyncCHR();
		break;
	case 0x5130:
		MMC50x5130 = V;
		break;
	case 0x5200:
		MMC5HackSPMode = V;
		MMC5_SyncCHR();
		break;
	case 0x5201:
		MMC5HackSPScroll = (V >> 3) & 0x1F;
		break;
	case 0x5202:
		MMC5HackSPPage = V & 0x3F;
		break;
	case 0x5203:
		X6502_IRQEnd(FCEU_IQEXT);
		mmc5.irq.scanlineTarget = V;
		break;
	case 0x5204:
		X6502_IRQEnd(FCEU_IQEXT);
		mmc5.irq.enabled = V & 0x80;
		break;
	case 0x5205:
		mmc5.mul[0] = V;
		break;
	case 0x5206:
		mmc5.mul[1] = V;
		break;
	}
}

static DECLFR(MMC5_ReadROMRAM) {
	return CartBROB(A);
}

static DECLFW(MMC5_WriteROMRAM) {
	if ((mmc5.wramProtect[0] == 0x02) && (mmc5.wramProtect[1] == 0x01)) {
		CartBW(A, V);
	}
}

DECLFW(MMC5_ExRAMWr) {
	if (MMC5HackCHRMode != MODE3) {
		mmc5.exRam[A & 0x03FF] = V;
	}
}

static DECLFR(MMC5_ExRAMRd) {
	return mmc5.exRam[A & 0x03FF];
}

static DECLFR(MMC5_read) {
	uint8_t ret = cpu.openbus;
	switch (A) {
	case 0x5204:
		X6502_IRQEnd(FCEU_IQEXT);
		ret = mmc5.irq.inFrame ? 0x40 : 0;
		ret |= mmc5.irq.pending ? 0x80 : 0;
#ifdef FCEUDEF_DEBUGGER
		if (!fceuindbg)
#endif
			mmc5.irq.pending = 0;
		return ret;
	case 0x5205:
		return ((uint32_t)(mmc5.mul[0] * mmc5.mul[1]) & 0xFF);
	case 0x5206:
		return ((uint32_t)(mmc5.mul[0] * mmc5.mul[1]) >> 8);
	}
	return ret;
}

static void Sync(void) {
	uint8_t moop;

	MMC5_SyncPRG();
	MMC5_SyncCHR();
	MMC5_SyncMirror();

	/* in case the fill register changed, we need to overwrite the fill buffer */
	FCEU_dwmemset32(mmc5.fillTable, mmc5.fillTile | (mmc5.fillTile << 8) | (mmc5.fillTile << 16) | (mmc5.fillTile << 24), 0x03C0);
	moop = mmc5.fillColor | (mmc5.fillColor << 2) | (mmc5.fillColor << 4) | (mmc5.fillColor << 6);
	FCEU_dwmemset32(mmc5.fillTable + 0x03C0, moop | (moop << 8) | (moop << 16) | (moop << 24), 0x40);

	MMC5HackCHRMode = mmc5.extMode & 0x03;

	/* zero 17-apr-2013 - why the heck should this happen here? anything in a `synco` should be depending on the state.
	 * im going to leave it commented out to see what happens
	 */
	/* X6502_IRQEnd(FCEU_IQEXT); */
}

void MMC5_hb(int cur_scanline) {
	/* zero 24-jul-2014 - revised for newer understanding, to fix metal slader glory credits. see r7371 in bizhawk */
	int sl = cur_scanline + 1;
	int ppuon = (PPU[1] & 0x18);

	if (!ppuon || sl >= 241) {
		/* whenever rendering is off for any reason (vblank or forced disable
		 * the irq counter resets, as well as the inframe flag (easily verifiable from software)
		 */
		mmc5.irq.inFrame = FALSE;
		mmc5.irq.pending = FALSE;
		mmc5.irq.scanlineCounter = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		return;
	}

	if (!mmc5.irq.inFrame) {
		mmc5.irq.inFrame = TRUE;
		mmc5.irq.pending = FALSE;
		mmc5.irq.scanlineCounter = 0;
		X6502_IRQEnd(FCEU_IQEXT);
	} else {
		mmc5.irq.scanlineCounter++;
		if (mmc5.irq.scanlineCounter == mmc5.irq.scanlineTarget) {
			mmc5.irq.pending = TRUE;
			if (mmc5.irq.enabled) {
				X6502_IRQBegin(FCEU_IQEXT);
			}
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

static void MMC5_Reset(void) {
	int x;
	uint8_t nval;
	uint8_t aval;

	for (x = 0; x < 5; x++) {
		mmc5.prg[x] = ((~4) + x);
	}
	for (x = 0; x < 12; x++) {
		mmc5.chr[x] = x;
	}
	for (x = 0; x < 2; x++) {
		mmc5.wramProtect[x] = 0;
	}
	for (x = 0; x < 2; x++) {
		mmc5.mul[x] = ~0;
	}

	mmc5.extMode = MODE0;
	mmc5.prgMode = MODE3;
	mmc5.chrMode = MODE0;
	mmc5.chrLast = 0;

	mmc5.irq.scanlineTarget = 0;
	mmc5.irq.enabled = 0;
	mmc5.irq.scanlineCounter = 0;
	mmc5.irq.pending = FALSE;
	mmc5.irq.inFrame = FALSE;

	mmc5.nmt = 0;
	mmc5.fillTile = mmc5.fillColor = 0;

	/* mmc5.fillTable is and 8-bit tile index, and a 2-bit attribute implented as a mirrored nametable */
	nval = mmc5.fillTable[0x000];
	aval = mmc5.fillTable[0x3C0] & 0x03;
	aval = aval | (aval << 2) | (aval << 4) | (aval << 6);
	FCEU_dwmemset32(mmc5.fillTable + 0x000, nval | (nval << 8) | (nval << 16) | (nval << 24), 0x3C0);
	FCEU_dwmemset32(mmc5.fillTable + 0x3C0, aval | (aval << 8) | (aval << 16) | (aval << 24), 0x040);

	if (mmc5.batteryFlag == 0) {
		FCEU_MemoryRand(WRAM, WRAMSIZE);
		FCEU_MemoryRand(mmc5.fillTable, 1024);
		FCEU_MemoryRand(mmc5.exRam, 1024);
	}

	MMC5Hack = TRUE;
	MMC5HackVROMMask = CHRmask4[0];
	MMC5HackVROMPTR = CHRptr[0];
	MMC5HackExNTARAMPtr = mmc5.exRam;
	MMC5HackCHRMode = MODE0;
	MMC5HackSPMode = MMC5HackSPScroll = MMC5HackSPPage = 0;

	FFCEUX_PPURead = mmc5_PPURead;
	FFCEUX_PPUWrite = mmc5_PPUWrite;

	Sync();
}

static void MMC5_Power(void) {
	SetWriteHandler(0x4020, 0x5BFF, Mapper5_write);
	SetReadHandler(0x4020, 0x5BFF, MMC5_read);

	SetWriteHandler(0x5C00, 0x5FFF, MMC5_ExRAMWr);
	SetReadHandler(0x5C00, 0x5FFF, MMC5_ExRAMRd);

	SetWriteHandler(0x6000, 0xFFFF, MMC5_WriteROMRAM);
	SetReadHandler(0x6000, 0xFFFF, MMC5_ReadROMRAM);

	SetReadHandler(0x5015, 0x5015, MMC5Sound_ReadStatus);
	SetWriteHandler(0x5000, 0x5015, MMC5Sound_Write);
	SetWriteHandler(0x5205, 0x5206, Mapper5_write);
	SetReadHandler(0x5205, 0x5206, MMC5_read);

	/*	GameHBIRQHook=MMC5_hb; */
	/*	FCEU_CheatAddRAM(8, 0x6000, WRAM); */
	FCEU_CheatAddRAM(1, 0x5C00, mmc5.exRam);

	MMC5_Reset();
}

void MMC5_Init(CartInfo *info, int wsize, int battery) {
	if (wsize) {
		WRAM = (uint8_t *)FCEU_gmalloc(wsize * 1024);
		memset(WRAM, 0, wsize * 1024);
		SetupCartPRGMapping(0x10, WRAM, wsize * 1024, 1);
		AddExState(WRAM, wsize * 1024, 0, "WRAM");
	}

	AddExState(mmc5.exRam, 1024, 0, "ERAM");
	AddExState(&MMC5HackSPMode, 1, 0, "SPLM");
	AddExState(&MMC5HackSPScroll, 1, 0, "SPLS");
	AddExState(&MMC5HackSPPage, 1, 0, "SPLP");
	AddExState(&MMC50x5130, 1, 0, "5130");
	AddExState(MMC5_StateRegs, ~0, 0, 0);

	WRAMSIZE = wsize * 1024;
	GameStateRestore = StateRestore;
	info->Power = MMC5_Power;
	info->Reset = MMC5_Reset;

	mmc5.batteryFlag = battery;
	if (battery) {
		uint32_t saveramsize;
		if (info->iNES2) {
			saveramsize = info->PRGRamSaveSize;
		} else {
			if (wsize <= 16) {
				saveramsize = 8 * 1024;
			} else if (wsize >= 64) {
				saveramsize = 64 * 1024;
			} else {
				saveramsize = 32 * 1024;
			}
		}
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = saveramsize;
	}

	MMC5Sound_ESI();
	MMC5Sound_AddStateInfo();
}
