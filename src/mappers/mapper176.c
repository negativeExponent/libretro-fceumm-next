/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
 *  Copyright (C) 2020
 *  Copyright (C) 2024 negativeExponent
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

/*	Mappers:
	176 - Standard
	523 - Jncota KT-xxx, re-release of 封神榜꞉ 伏魔三太子: 1 KiB->2 KiB, 2 KiB->4 KiB CHR, hard-wired nametable mirroring)

	Submappers:
	0 - Standard
	1 - FK-xxx
	2 - 外星 FS005/FS006
	3 - JX9003B
	4 - GameStar Smart Genius Deluxe
	5 - HST-162

	Verified on real hardware:
	"Legend of Kage" sets CNROM latch 1 and switches between CHR bank 0 and 1 using 5FF2, causing the wrong bank (1 instead of 0) during gameplay.

	Heuristic for detecting whether the DIP switch should be changed on every soft reset:
	The first write to the $5xxx range is to $501x           => ROM always addresses $501x; changing the DIP switch on reset would break the emulation after reset, so don't do it.
	The first write to the $5xxx range is to $5020 or higher => ROM either uses a DIP switch or writes to $5FFx for safety; changing the DIP switch on reset is possible.
	Exclude the $5FF3 address as well as $5000-$500F from this heuristic.
*/

#include "mapinc.h"

static uint8 fk23_regs[8]     = { 0 }; /* JX9003B has eight registers, all others have four */
static uint8 mmc3_regs[16]    = { 0 }; /* only 12 registers are used here */
static uint8 mmc3_ctrl        = 0;
static uint8 mmc3_mirr        = 0;
static uint8 mmc3_wram        = 0;
static uint8 reg4800          = 0;

static uint8 irq_count        = 0;
static uint8 irq_latch        = 0;
static uint8 irq_enabled      = FALSE;
static uint8 irq_reload       = FALSE;
static uint8 latch            = 0; /* CNROM/UNROM latch @ $8xxx writes */
static uint8 dipswitch        = 0;
static uint8 dipsw_enable     = 0; /* Change the address mask on every reset? */
static uint8 after_power      = 0; /* Used for detecting whether a DIP switch is used or not (see above) */

static void (*FK23_cwrap)(uint16 A, uint16 V);
static void (*SyncMIRR)(void);

static SFORMAT StateRegs[] = {
   { fk23_regs,               8, "EXPR" },
   { mmc3_regs,              16, "M3RG" },
   { &latch,                  1, "LATC" },
   { &dipswitch,              1, "DPSW" },
   { &mmc3_ctrl,              1, "M3CT" },
   { &mmc3_mirr,              1, "M3MR" },
   { &mmc3_wram,              1, "M3WR" },
   { &reg4800,                1, "REG4" },
   { &irq_reload,             1, "IRQR" },
   { &irq_count,              1, "IRQC" },
   { &irq_latch,              1, "IRQL" },
   { &irq_enabled,            1, "IRQA" },
   { 0 }
};

#define INVERT_PRG          !!(mmc3_ctrl & 0x40)
#define INVERT_CHR          !!(mmc3_ctrl & 0x80)

#define PRG_MODE              ( fk23_regs[0] & 0x07)

#define WRAM_ENABLED        !!(mmc3_wram & 0x80)
#define WRAM_EXTENDED      (!!(mmc3_wram & 0x20) && (iNESCart.submapper == 2))    /* Extended A001 register. Only available on FS005 PCB. */
#define FK23_ENABLED       (!!(mmc3_wram & 0x40) || !WRAM_EXTENDED)  /* Enable or disable registers in the $5xxx range. Only available on FS005 PCB. */
#define MMC3_EXTENDED       !!( fk23_regs[3] & 0x02)                 /* Extended MMC3 mode, adding extra registers for switching the normally-fixed PRG banks C and E and for eight independent 1 KiB CHR banks. Only available on FK- and FS005 PCBs. */

#define CHR_8K_MODE         !!( fk23_regs[0] & 0x40)                 /* MMC3 CHR registers are ignored, apply outer bank only, and CNROM latch if it exists */
#define CHR_CNROM_MODE        ((~fk23_regs[0] & 0x20) && ((iNESCart.submapper == 1) || (iNESCart.submapper == 5))) /* Only subtypes 1 and 5 have a CNROM latch, which can be disabled */
#define CHR_OUTER_BANK_SIZE !!( fk23_regs[0] & 0x10)                 /* Switch between 256 and 128 KiB CHR, or 32 and 16 KiB CHR in CNROM mode */
#define CHR_MIXED           !!(WRAM_EXTENDED && (mmc3_wram & 0x04))     /* First 8 KiB of CHR address space are RAM, then ROM */

static void CHRWRAP(uint16 A, uint16 V) {
	uint8 bank = 0;

	/* some workaround for chr rom / ram access */
	if (!ROM.chr.size) {
		/* CHR-RAM only */
		bank = 0;
	} else if (CHRRAMSIZE) {
		/* Mixed CHR-ROM + CHR-RAM */
		if ((fk23_regs[0] & 0x20) && ((iNESCart.submapper == 0) || (iNESCart.submapper == 1))) {
			bank = 0x10;
		} else if (CHR_MIXED && (V < 8)) {
			/* first 8K of chr bank is RAM */
			bank = 0x10;
		}
	}

	setchr1r(bank, A, V);
}

static void SyncCHR(void) {
	uint16 base = fk23_regs[2];

	if (iNESCart.submapper == 3) {
		base |= (fk23_regs[6] << 8); /* Outer 8 KiB CHR bank. Subtype 3 has an MSB register providing more bits. */
	}

	if (CHR_8K_MODE) {
		uint16 mask = (CHR_CNROM_MODE ? (CHR_OUTER_BANK_SIZE ? 0x01 : 0x03) : 0x00);
		/* In Submapper 1, address bits come either from outer bank or from latch. In Submapper 5, they are OR'd. Both
		 * verified on original hardware. */
		uint16 bank = ((iNESCart.submapper == 5) ? base : (base & ~mask)) | (latch & mask);

		bank <<= 3;

		FK23_cwrap(0x0000, bank | 0);
		FK23_cwrap(0x0400, bank | 1);
		FK23_cwrap(0x0800, bank | 2);
		FK23_cwrap(0x0C00, bank | 3);

		FK23_cwrap(0x1000, bank | 4);
		FK23_cwrap(0x1400, bank | 5);
		FK23_cwrap(0x1800, bank | 6);
		FK23_cwrap(0x1C00, bank | 7);
	} else {
		uint16 swap = (INVERT_CHR ? 0x1000 : 0);
		uint16 mask = (CHR_OUTER_BANK_SIZE ? 0x7F : 0xFF);

		/* From 8 KiB to 1 KiB banks. Address bits are never OR'd; they either
		 * come from the outer bank or from the MMC3. */
		base <<= 3;

		if (MMC3_EXTENDED) {
			FK23_cwrap(swap ^ 0x0000, (base & ~mask) | (mmc3_regs[0]  & mask));
			FK23_cwrap(swap ^ 0x0400, (base & ~mask) | (mmc3_regs[10] & mask));
			FK23_cwrap(swap ^ 0x0800, (base & ~mask) | (mmc3_regs[1]  & mask));
			FK23_cwrap(swap ^ 0x0c00, (base & ~mask) | (mmc3_regs[11] & mask));

			FK23_cwrap(swap ^ 0x1000, (base & ~mask) | (mmc3_regs[2] & mask));
			FK23_cwrap(swap ^ 0x1400, (base & ~mask) | (mmc3_regs[3] & mask));
			FK23_cwrap(swap ^ 0x1800, (base & ~mask) | (mmc3_regs[4] & mask));
			FK23_cwrap(swap ^ 0x1c00, (base & ~mask) | (mmc3_regs[5] & mask));
		} else {
			FK23_cwrap(swap ^ 0x0000, (base & ~mask) | ((mmc3_regs[0] & 0xFE) & mask));
			FK23_cwrap(swap ^ 0x0400, (base & ~mask) | ((mmc3_regs[0] | 0x01) & mask));
			FK23_cwrap(swap ^ 0x0800, (base & ~mask) | ((mmc3_regs[1] & 0xFE) & mask));
			FK23_cwrap(swap ^ 0x0C00, (base & ~mask) | ((mmc3_regs[1] | 0x01) & mask));

			FK23_cwrap(swap ^ 0x1000, (base & ~mask) | (mmc3_regs[2] & mask));
			FK23_cwrap(swap ^ 0x1400, (base & ~mask) | (mmc3_regs[3] & mask));
			FK23_cwrap(swap ^ 0x1800, (base & ~mask) | (mmc3_regs[4] & mask));
			FK23_cwrap(swap ^ 0x1c00, (base & ~mask) | (mmc3_regs[5] & mask));
		}
	}
}

static void SyncPRG(void) {
	const static uint16 mask_lut[8] = {
		0x3F, 0x1F, 0x0F, 0x00,
		0x00, 0x00, 0x7F, 0xFF
	};

	/* For PRG modes 0-2, the mode# decides how many bits of the inner 8 KiB
	 * bank are used. This is greatly relevant to map the correct bank that
	 * contains the reset vectors. */
	uint16 mask = mask_lut[PRG_MODE];

	/* The bits for the first 2 MiB are the same between all the variants. */
	uint16 base = fk23_regs[1] & 0x7F;

	switch (iNESCart.submapper) {
	case 1: /* FK-xxx */
		if (PRG_MODE == 0 || MMC3_EXTENDED) {
			mask = 0xFF; /* Mode 7 allows the MMC3 to address 2 MiB rather than the usual 512 KiB. */
		}
		break;
	case 2: /* FS005 */
		base |= ((fk23_regs[0] << 4) & 0x080) | ((fk23_regs[0] << 1) & 0x100) |
				((fk23_regs[2] << 3) & 0x600) | ((fk23_regs[2] << 6) & 0x800);
		break;
	case 3: /* JX9003B */
		if (PRG_MODE == 0 || MMC3_EXTENDED) {
			mask = 0xFF; /* Mode 7 allows the MMC3 to address 2 MiB rather than the usual 512 KiB. */
		}
		base |= fk23_regs[5] << 7;
		break;
	case 4: /* GameStar Smart Genius Deluxe */
		base |= (fk23_regs[2] & 0x80);
		break;
	case 5: /* HST-162 */
		base = (base & 0x1F) | (reg4800 << 5);
		break;
	}

	switch (PRG_MODE) {
	default: {
		/* 0: MMC3 with 512 KiB addressable */
		/* 1: MMC3 with 256 KiB addressable */
		/* 2: MMC3 with 128 KiB addressable */
		/* 7: MMC3 with   2  MB addressable. Used byc at least on 2 games:
			- 最终幻想 2 - 光明篇 (Final Fantasy 2 - Arc of Light)
			- 梦幻仙境 - (Fantasy Wonderworld) */
		uint16 swap = (INVERT_PRG ? 0x4000 : 0);

		/* from 16 to 8 KiB. Address bits are never OR'd; they either come from
		 * the outer bank or from the MMC3.  */
		base <<= 1;

		if (MMC3_EXTENDED) {
			setprg8(0x8000 ^ swap, (base & ~mask) | (mmc3_regs[6] & mask));
			setprg8(0xA000,        (base & ~mask) | (mmc3_regs[7] & mask));
			setprg8(0xC000 ^ swap, (base & ~mask) | (mmc3_regs[8] & mask));
			setprg8(0xE000,        (base & ~mask) | (mmc3_regs[9] & mask));
		} else {
			setprg8(0x8000 ^ swap, (base & ~mask) | (mmc3_regs[6] & mask));
			setprg8(0xA000,        (base & ~mask) | (mmc3_regs[7] & mask));
			setprg8(0xC000 ^ swap, (base & ~mask) | (0xFE & mask));
			setprg8(0xE000,        (base & ~mask) | (0xFF & mask));
		}
		break;
	}
	case 3: /* NROM-128 */
		setprg16(0x8000, base);
		setprg16(0xC000, base);
		break;
	case 4: /* NROM-256 */
		setprg32(0x8000, (base >> 1));
		break;
	case 5: /* UNROM */
		setprg16(0x8000, (base & ~0x07) | (latch & 0x07) | 0x00);
		setprg16(0xC000, (base & ~0x07) | (latch & 0x07) | 0x07);
		break;
	}
}

static void SyncWRAM(void) {
	/* TODO: WRAM Protected  mode when not in extended mode */
	if (WRAM_ENABLED) {
		if (iNESCart.submapper == 2) {
			setprg8r(0x10, 0x4000, (mmc3_wram + 1) & 0x03);
			setprg8r(0x10, 0x6000, (mmc3_wram + 0) & 0x03);
		} else {
			setprg8r(0x10, 0x6000, 0);
		}
	}
}

static void FixMir(void) {
	switch (mmc3_mirr & (iNESCart.submapper == 2 ? 0x03 : 0x01)) {
	case 0: setmirror(MI_V); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror(MI_0); break;
	case 3: setmirror(MI_1); break;
	}
}

static void Sync(void) {
	SyncPRG();
	SyncCHR();
	SyncWRAM();
	SyncMIRR();
}

static DECLFW(Write4800) {
	/* Only used by submapper 5 (HST-162) */
	reg4800 = V;
	SyncPRG();
}

static DECLFW(Write5000) {
	if (after_power && (A > 0x5010) && (A != 0x5FF3)) {
		/* Ignore writes from $5000-$500F, in particular to $5008, but not $5FF3 */
		after_power = 0;
		/* The DIP switch change on soft-reset is enabled if the first write
		 * after power-on is not to $501x */
		dipsw_enable = (A >= 0x5020);
	}
	if (FK23_ENABLED && (A & (0x10 << dipswitch))) {
		fk23_regs[A & (iNESCart.submapper == 3 ? 7 : 3)] = V;
		SyncPRG();
		SyncCHR();
	} else {
		/* FK23C Registers disabled, $5000-$5FFF maps to the second 4 KiB of the
		 * 8 KiB WRAM bank 2 */
		CartBW(A, V);
	}
}

static DECLFW(Write8000) {
	uint8 old_ctrl = 0;
	uint8 ctrl_mask = 0;
	uint8 updatePRG = FALSE;
	uint8 updateCHR = FALSE;

	if (latch != V) {
		latch = V;
		if (CHR_8K_MODE) {
			updateCHR = TRUE; /* CNROM latch updated */
		}
		if (PRG_MODE == 5) {
			updatePRG = TRUE; /* UNROM latch has been updated */
		}
	}

	switch (A & 0xF000) {
	case 0x8000:
	case 0x9000:
		/* Confirmed on real hardware: writes to 8002 and 8003, or 9FFE and
		   9FFF, are ignored. Needed for Dr. Mario on some of the "bouncing
		   ball" multis. */
		if (A & 0x02) {
			break; 
		}
		
		if (A & 0x01) {
			ctrl_mask = MMC3_EXTENDED ? 0x0F : 0x07;
			mmc3_regs[mmc3_ctrl & ctrl_mask] = V;

			switch (mmc3_ctrl & ctrl_mask) {
			case 6:
			case 7:
			case 8:
			case 9:
				updatePRG = TRUE;
				break;
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 10:
			case 11:
			default:
				updateCHR = TRUE;
				break;
			}
		} else {
			old_ctrl = mmc3_ctrl;
			/* Subtype 2, 8192 or more KiB PRG-ROM, no CHR-ROM: Like Subtype 0,
		 	* but MMC3 registers $46 and $47 swapped. */
			if ((iNESCart.submapper == 2) && ((V == 0x46) || (V == 0x47))) {
				V ^= 0x01;
			}

			mmc3_ctrl = V;

			if (INVERT_PRG != (old_ctrl & 0x40)) {
				updatePRG = TRUE;
			}
			if (INVERT_CHR != (old_ctrl & 0x80)) {
				updateCHR = TRUE;
			}
		}
		break;
	case 0xA000:
	case 0xB000:
		if (A & 0x01) {
			/* ignore bits when ram config register is disabled */
			if ((V & 0x20) == 0) {
				V &= 0xC0;
			}
			mmc3_wram = V;
			SyncWRAM();
			updateCHR = TRUE;
		} else {
			mmc3_mirr = V;
			SyncMIRR();
		}
		break;
	case 0xC000:
	case 0xD000:
		if (A & 0x01) {
			irq_reload = TRUE;
		} else {
			irq_latch = V;
		}
		break;
	case 0xE000:
	case 0xF000:
		if (A & 0x01) {
			irq_enabled = TRUE;
		} else {
			X6502_IRQEnd(FCEU_IQEXT);
			irq_enabled = FALSE;
		}
		break;
	}

	if (updatePRG) SyncPRG();
	if (updateCHR) SyncCHR();
}

static void M176HBIRQHook(void) {
	if (!irq_count || irq_reload) {
		irq_count = irq_latch;
	} else {
		irq_count--;
	}
	if (!irq_count && irq_enabled) {
		X6502_IRQBegin(FCEU_IQEXT);
	}
	irq_reload = FALSE;
}

static void RegReset(void) {
	fk23_regs[0]  = fk23_regs[1] = fk23_regs[2] = fk23_regs[3] = 0;
	fk23_regs[4]  = fk23_regs[5] = fk23_regs[6] = fk23_regs[7] = 0;
	mmc3_regs[0]  = 0;
	mmc3_regs[1]  = 2;
	mmc3_regs[2]  = 4;
	mmc3_regs[3]  = 5;
	mmc3_regs[4]  = 6;
	mmc3_regs[5]  = 7;
	mmc3_regs[6]  = 0;
	mmc3_regs[7]  = 1;
	mmc3_regs[8]  = ~1;
	mmc3_regs[9]  = ~0;
	mmc3_regs[10] = ~0;
	mmc3_regs[11] = ~0;
	mmc3_ctrl = mmc3_mirr = irq_count = irq_latch = irq_enabled = 0;
	reg4800 = 0;

	if (iNESCart.submapper == 2) {
		mmc3_wram = 0xC0;
	} else {
		mmc3_wram = 0x80;
	}

	if (iNESCart.submapper == 1) {
		fk23_regs[1] = ~0;
	}

	Sync();
}

static void M176Reset(void) {
	/* this little hack makes sure that we try all the dip switch settings eventually, if we reset enough */
	if (dipsw_enable) {
		dipswitch = (dipswitch + 1) & 7;
		FCEU_printf("BMCFK23C dipswitch set to $%04x\n", 0x5000 | 0x10 << dipswitch);
	}

	RegReset();
}

static void M176Power(void) {
	RegReset();

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, Write5000);
	SetWriteHandler(0x8000, 0xFFFF, Write8000);

	if (iNESCart.submapper == 5) {
		SetWriteHandler(0x4800, 0x4FFF, Write4800);
	}

	if (WRAMSIZE) {
		if (iNESCart.submapper == 2) {
			SetReadHandler(0x5000, 0x5FFF, CartBR);
		}
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
}

static void M176Close(void) {
}

static void StateRestore(int version) {
	Sync();
}

static void Init(CartInfo *info) {
	/* Setup default function wrappers */
	FK23_cwrap = CHRWRAP;
	SyncMIRR = FixMir;

	/* Initialization for iNES and UNIF. iNESCart.submapper and dipsw_enable must have been set. */
	info->Power = M176Power;
	info->Reset = M176Reset;
	info->Close = M176Close;
	GameHBIRQHook = M176HBIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	if (CHRRAMSIZE) {
		CHRRAM = (uint8 *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
	}

	if (WRAMSIZE) {
		WRAM = (uint8 *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");

		if (info->battery) {
			info->SaveGame[0] = WRAM;
			if (info->iNES2 && info->PRGRamSaveSize) {
				info->SaveGameLen[0] = info->PRGRamSaveSize;
			} else {
				info->SaveGameLen[0] = WRAMSIZE;
			}
		}
	}
}

void Mapper176_Init(CartInfo *info) { /* .NES file */
	dipsw_enable = 0;
	if (info->iNES2) {
		after_power = iNESCart.submapper != 2; /* FS005 never has DIP switches, the others may have one, so use the heuristic. */
		if (ROM.chr.size) {
			CHRRAMSIZE = info->CHRRamSize + info->CHRRamSaveSize;
		}
		WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
	} else {
		/* Waixing boards have 32K battery backed wram */
		if (info->battery) {
			info->submapper = 2;
			after_power = 0;
			WRAMSIZE = 32 * 1024;
		} else {
			/* Always enable WRAM for iNES-headered files */
			WRAMSIZE = 8 * 1024;

			if ((ROM.prg.size == (1024 * 1024)) && (ROM.chr.size == (1024 * 1024))) {
				info->submapper = 1;
			} else if ((ROM.prg.size == (256 * 1024)) && (ROM.chr.size == (128 * 1024))) {
				info->submapper = 1;
			} else if ((ROM.prg.size == (128 * 1024)) && (ROM.chr.size == (64 * 1024))) {
				info->submapper = 1;
			} else if ((ROM.prg.size >= (8192 * 1024)) && (ROM.chr.size == (0 * 1024))) {
				info->submapper = 2;
			} else if ((ROM.prg.size == (4096 * 1024)) && (ROM.chr.size == (0 * 1024))) {
				info->submapper = 3;
			}

			/* Detect heuristically whether the address mask should be changed on every soft reset */
			after_power = 1;

			if (CHRRAMSIZE && !ROM.chr.size) {
				/* FIXME: CHR-RAM is already set in iNES mapper initializer when there is no CHR ROM present */
				/* so avoid reallocation it. */
				CHRRAMSIZE = 0;
			}
		}
	}
	Init(info);
}

/* UNIF FK23C. Also includes mislabelled WAIXING-FS005, recognizable by their PRG-ROM size. */
void BMCFK23C_Init(CartInfo *info) {
	if (ROM.chr.size) {
		/* Rockman I-VI uses mixed chr rom/ram */
		if (ROM.prg.size == (2048 * 1024) && ROM.chr.size == (512 * 1024)) {
			CHRRAMSIZE = 8 * 1024;
		}
	}
	WRAMSIZE = 8 * 1024;

	dipsw_enable = 0;
	after_power = 1;
	info->submapper = (ROM.prg.size >= (4096 * 1024)) ? 2 : (ROM.prg.size == (64 * 1024) && ROM.chr.size == (128 * 1024)) ? 1 : 0;
	if (info->submapper == 2) {
		CHRRAMSIZE = 256 * 1024;
	}

	Init(info);
}

 /* UNIF FK23CA. Also includes mislabelled WAIXING-FS005, recognizable by their PRG-ROM size. */
void BMCFK23CA_Init(CartInfo *info) {
	WRAMSIZE = 8 * 1024;

	dipsw_enable = 0;
	after_power = 1;
	info->submapper = (ROM.prg.size >= (2048 * 1024)) ? 2 : 1;
	if (info->submapper == 2) {
		CHRRAMSIZE = 256 * 1024;
	}

	Init(info);
}

/* UNIF BMC-Super24in1SC03 */
void Super24_Init(CartInfo *info) {
	CHRRAMSIZE = 8 * 1024;
	dipsw_enable = 0;
	after_power = 0;
	info->submapper = 0;
	Init(info);
}

/* UNIF WAIXING-FS005 */
void WAIXINGFS005_Init(CartInfo *info) {
	CHRRAMSIZE = 8 * 1024;
	WRAMSIZE = 32 * 1024;
	dipsw_enable = 0;
	after_power = 0;
	info->submapper = 2;
	Init(info);
}

static void M523CW(uint16 A, uint16 V) {
	if (~A & 0x0400) {
		setchr2(A, V);
	}
}

static void M523MIR(void) {
	/* Jncota board has hard-wired mirroring */
}

/* Jncota board with unusual wiring that turns 1 KiB CHR banks into 2 KiB banks, and has hard-wired nametable mirroring. */
void Mapper523_Init(CartInfo *info) { /* Jncota Fengshengban */
	WRAMSIZE = 8 * 1024;
	dipsw_enable = 0;
	after_power = 0;
	info->submapper = 1;

	Init(info);
	SyncMIRR = M523MIR;
	FK23_cwrap = M523CW;
}
