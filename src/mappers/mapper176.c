/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
 *  Copyright (C) 2020
 *  Copyright (C) 2024-2025-2026 negativeExponent
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

static struct {
	uint8_t latch; /* CNROM/UNROM latch @ $8xxx writes */
	uint8_t reg4800;
	uint8_t fk23_regs[8];  /* JX9003B has eight registers, all others have four */
	uint8_t mmc3_regs[16]; /* only 12 registers are used here */
	uint8_t mmc3_ctrl;
	uint8_t mmc3_mirr;
	uint8_t mmc3_wram;

	uint8_t irq_count;
	uint8_t irq_latch;
	uint8_t irq_enabled;
	uint8_t irq_reload;
} m176;

static uint8_t dipswitch = 0;
static uint8_t dipsw_enable = 0; /* Change the address mask on every reset? */
static uint8_t after_power = 0;  /* Used for detecting whether a DIP switch is used or not (see above) */

static SFORMAT StateRegs[] = {
	{ m176.fk23_regs, 8, "EXPR" },
	{ m176.mmc3_regs, 16, "M3RG" },
	{ &m176.latch, 1, "LATC" },
	{ &dipswitch, 1, "DPSW" },
	{ &m176.mmc3_ctrl, 1, "M3CT" },
	{ &m176.mmc3_mirr, 1, "M3MR" },
	{ &m176.mmc3_wram, 1, "M3WR" },
	{ &m176.reg4800, 1, "REG4" },
	{ &m176.irq_reload, 1, "IRQR" },
	{ &m176.irq_count, 1, "IRQC" },
	{ &m176.irq_latch, 1, "IRQL" },
	{ &m176.irq_enabled, 1, "IRQA" },
	{ 0 }
};

#define INVERT_PRG          (m176.mmc3_ctrl & 0x40)
#define INVERT_CHR          (m176.mmc3_ctrl & 0x80)
#define PRG_MODE            (m176.fk23_regs[0] & 0x07)
#define WRAM_ENABLED        (m176.mmc3_wram & 0x80)
#define WRAM_EXTENDED       ((m176.mmc3_wram & 0x20) && (iNESCart.submapper == 2))                                    /* Extended A001 register. Only available on FS005 PCB. */
#define FK23_ENABLED        ((m176.mmc3_wram & 0x40) || !WRAM_EXTENDED)                                               /* Enable or disable registers in the $5xxx range. Only available on FS005 PCB. */
#define MMC3_EXTENDED       (m176.fk23_regs[3] & 0x02)                                                                /* Extended MMC3 mode, adding extra registers for switching the normally-fixed PRG banks C and E and for eight independent 1 KiB CHR banks. Only available on FK- and FS005 PCBs. */
#define CHR_8K_MODE         (m176.fk23_regs[0] & 0x40)                                                                /* MMC3 CHR registers are ignored, apply outer bank only, and CNROM latch if it exists */
#define CHR_CNROM_MODE      (!(m176.fk23_regs[0] & 0x20) && ((iNESCart.submapper == 1) || (iNESCart.submapper == 5))) /* Only subtypes 1 and 5 have a CNROM latch, which can be disabled */
#define CHR_OUTER_BANK_SIZE (m176.fk23_regs[0] & 0x10)                                                                /* Switch between 256 and 128 KiB CHR, or 32 and 16 KiB CHR in CNROM mode */
#define CHR_MIXED           (WRAM_EXTENDED && (m176.mmc3_wram & 0x04))                                                /* First 8 KiB of CHR address space are RAM, then ROM */

static void SyncPRG(void) {
	const static uint16_t mask_lut[8] = {
		0x3F, 0x1F, 0x0F, 0x00,
		0x00, 0x00, 0x7F, 0xFF
	};

	/* For PRG modes 0-2, the mode# decides how many bits of the inner 8 KiB
	 * bank are used. This is greatly relevant to map the correct bank that
	 * contains the reset vectors. */
	uint16_t mask = mask_lut[PRG_MODE];

	/* The bits for the first 2 MiB are the same between all the variants. */
	uint16_t base = m176.fk23_regs[1] & 0x7F;

	switch (iNESCart.submapper) {
	case 1: /* FK-xxx */
		if (PRG_MODE == 0 || MMC3_EXTENDED) {
			mask = 0xFF; /* Mode 7 allows the MMC3 to address 2 MiB rather than the usual 512 KiB. */
		}
		break;
	case 2: /* FS005 */
		base |= ((m176.fk23_regs[0] << 4) & 0x080) | ((m176.fk23_regs[0] << 1) & 0x100) |
				((m176.fk23_regs[2] << 3) & 0x600) | ((m176.fk23_regs[2] << 6) & 0x800);
		break;
	case 3: /* JX9003B */
		if (PRG_MODE == 0 || MMC3_EXTENDED) {
			mask = 0xFF; /* Mode 7 allows the MMC3 to address 2 MiB rather than the usual 512 KiB. */
		}
		base |= m176.fk23_regs[5] << 7;
		break;
	case 4: /* GameStar Smart Genius Deluxe */
		base |= (m176.fk23_regs[2] & 0x80);
		break;
	case 5: /* HST-162 */
		base = (base & 0x1F) | (m176.reg4800 << 5);
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
		uint16_t swap = (INVERT_PRG ? 0x4000 : 0);

		/* from 16 to 8 KiB. Address bits are never OR'd; they either come from
		 * the outer bank or from the MMC3.  */
		base <<= 1;

		if (MMC3_EXTENDED) {
			setprg8(0x8000 ^ swap, (base & ~mask) | (m176.mmc3_regs[6] & mask));
			setprg8(0xA000,        (base & ~mask) | (m176.mmc3_regs[7] & mask));
			setprg8(0xC000 ^ swap, (base & ~mask) | (m176.mmc3_regs[8] & mask));
			setprg8(0xE000,        (base & ~mask) | (m176.mmc3_regs[9] & mask));
		} else {
			setprg8(0x8000 ^ swap, (base & ~mask) | (m176.mmc3_regs[6] & mask));
			setprg8(0xA000,        (base & ~mask) | (m176.mmc3_regs[7] & mask));
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
		setprg16(0x8000, (base & ~0x07) | (m176.latch & 0x07) | 0x00);
		setprg16(0xC000, (base & ~0x07) | (m176.latch & 0x07) | 0x07);
		break;
	}
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint8_t bank = 0;

	/* some workaround for chr rom / ram access */
	if (ROM.chr.size && CHRRAMSIZE) {
		/* Mixed CHR-ROM + CHR-RAM */
		if ((m176.fk23_regs[0] & 0x20) && ((iNESCart.submapper == 0) || (iNESCart.submapper == 1))) {
			bank = 0x10;
		} else if (CHR_MIXED && (V < 8)) {
			/* first 8K of chr bank is RAM */
			bank = 0x10;
		}
	}

	setchr1r(bank, A, V);
}

static void SyncCHR(void) {
	uint16_t mask = (CHR_OUTER_BANK_SIZE ? 0x7F : 0xFF);
	uint16_t swap = (INVERT_CHR ? 0x1000 : 0);

	/* From 8 KiB to 1 KiB banks. Address bits are never OR'd; they either
	 * come from the outer bank or from the MMC3. */
	uint16_t base = m176.fk23_regs[2] << 3;

	uint16_t chrBank[8];

	if (iNESCart.submapper == 3) {
		base |= (m176.fk23_regs[6] << 11); /* Outer 8 KiB CHR bank. Subtype 3 has an MSB register providing more bits. */
	}

	if (CHR_8K_MODE) {
		mask = (CHR_CNROM_MODE ? (CHR_OUTER_BANK_SIZE ? 0x01 : 0x03) : 0x00);
		/* In Submapper 1, address bits come either from outer bank or from latch. In Submapper 5, they are OR'd. Both
		 * verified on original hardware. */
		base = ((iNESCart.submapper == 5) ? base : (base & ~(mask << 3))) | ((m176.latch & mask) << 3);
		chrBank[0] = base | 0;
		chrBank[1] = base | 1;
		chrBank[2] = base | 2;
		chrBank[3] = base | 3;
		chrBank[4] = base | 4;
		chrBank[5] = base | 5;
		chrBank[6] = base | 6;
		chrBank[7] = base | 7;
	} else {
		if (MMC3_EXTENDED) {
			chrBank[0] = ((base & ~mask) | (m176.mmc3_regs[0]  & mask));
			chrBank[1] = ((base & ~mask) | (m176.mmc3_regs[10] & mask));
			chrBank[2] = ((base & ~mask) | (m176.mmc3_regs[1]  & mask));
			chrBank[3] = ((base & ~mask) | (m176.mmc3_regs[11] & mask));
			chrBank[4] = ((base & ~mask) | (m176.mmc3_regs[2] & mask));
			chrBank[5] = ((base & ~mask) | (m176.mmc3_regs[3] & mask));
			chrBank[6] = ((base & ~mask) | (m176.mmc3_regs[4] & mask));
			chrBank[7] = ((base & ~mask) | (m176.mmc3_regs[5] & mask));
		} else {
			chrBank[0] = ((base & ~mask) | ((m176.mmc3_regs[0] & 0xFE) & mask));
			chrBank[1] = ((base & ~mask) | ((m176.mmc3_regs[0] | 0x01) & mask));
			chrBank[2] = ((base & ~mask) | ((m176.mmc3_regs[1] & 0xFE) & mask));
			chrBank[3] = ((base & ~mask) | ((m176.mmc3_regs[1] | 0x01) & mask));
			chrBank[4] = ((base & ~mask) | (m176.mmc3_regs[2] & mask));
			chrBank[5] = ((base & ~mask) | (m176.mmc3_regs[3] & mask));
			chrBank[6] = ((base & ~mask) | (m176.mmc3_regs[4] & mask));
			chrBank[7] = ((base & ~mask) | (m176.mmc3_regs[5] & mask));
		}
	}
	if (iNESCart.mapper == 523) {
		setchr2(swap ^ 0x0000, chrBank[0]);
		setchr2(swap ^ 0x0800, chrBank[2]);
		setchr2(swap ^ 0x1000, chrBank[4]);
		setchr2(swap ^ 0x1800, chrBank[6]);
	} else {
		SetCHR(swap ^ 0x0000, chrBank[0]);
		SetCHR(swap ^ 0x0400, chrBank[1]);
		SetCHR(swap ^ 0x0800, chrBank[2]);
		SetCHR(swap ^ 0x0C00, chrBank[3]);
		SetCHR(swap ^ 0x1000, chrBank[4]);
		SetCHR(swap ^ 0x1400, chrBank[5]);
		SetCHR(swap ^ 0x1800, chrBank[6]);
		SetCHR(swap ^ 0x1c00, chrBank[7]);
	}
}

static void SyncWRAM(void) {
	/* TODO: WRAM Protected  mode when not in extended mode */
	if (WRAM_ENABLED) {
		if (iNESCart.submapper == 2) {
			setprg8r(0x10, 0x4000, (m176.mmc3_wram + 1) & 0x03);
			setprg8r(0x10, 0x6000, (m176.mmc3_wram + 0) & 0x03);
		} else {
			unsetcpu4(0x5000);
			setprg8r(0x10, 0x6000, 0);
		}
	} else {
		unsetcpu16(0x4000);
	}
}

static void SyncMirror(void) {
	if (iNESCart.mapper == 523) {
		setmirror(iNESCart.mirror);
	} else {
		switch (m176.mmc3_mirr & (iNESCart.submapper == 2 ? 0x03 : 0x01)) {
		case 0:
			setmirror(MI_V);
			break;
		case 1:
			setmirror(MI_H);
			break;
		case 2:
			setmirror(MI_0);
			break;
		case 3:
			setmirror(MI_1);
			break;
		}
	}
}

static void Sync(void) {
	SyncPRG();
	SyncCHR();
	SyncWRAM();
	SyncMirror();
}

static DECLFW(Write4800) {
	/* Only used by submapper 5 (HST-162) */
	m176.reg4800 = V;
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
		m176.fk23_regs[A & (iNESCart.submapper == 3 ? 7 : 3)] = V;
		SyncPRG();
		SyncCHR();
	} else {
		/* FK23C Registers disabled, $5000-$5FFF maps to the second 4 KiB of the
		 * 8 KiB WRAM bank 2 */
		CartBW(A, V);
	}
}

static DECLFW(Write8000) {
	uint8_t old_ctrl = 0;
	uint8_t ctrl_mask = 0;
	uint8_t updatePRG = FALSE;
	uint8_t updateCHR = FALSE;

	if (m176.latch != V) {
		m176.latch = V;
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
			m176.mmc3_regs[m176.mmc3_ctrl & ctrl_mask] = V;
			switch (m176.mmc3_ctrl & ctrl_mask) {
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
			old_ctrl = m176.mmc3_ctrl;
			/* Subtype 2, 8192 or more KiB PRG-ROM, no CHR-ROM: Like Subtype 0,
			 * but MMC3 registers $46 and $47 swapped. */
			if ((iNESCart.submapper == 2) && ((V == 0x46) || (V == 0x47))) {
				V ^= 0x01;
			}

			m176.mmc3_ctrl = V;

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
			m176.mmc3_wram = V;
			SyncWRAM();
			updateCHR = TRUE;
		} else {
			m176.mmc3_mirr = V;
			SyncMirror();
		}
		break;
	case 0xC000:
	case 0xD000:
		if (A & 0x01) {
			m176.irq_reload = TRUE;
		} else {
			m176.irq_latch = V;
		}
		break;
	case 0xE000:
	case 0xF000:
		if (A & 0x01) {
			m176.irq_enabled = TRUE;
		} else {
			X6502_IRQEnd(FCEU_IQEXT);
			m176.irq_enabled = FALSE;
		}
		break;
	}

	if (updatePRG) {
		SyncPRG();
	}
	if (updateCHR) {
		SyncCHR();
	}
}

static void HBIRQHook(void) {
	if (!m176.irq_count || m176.irq_reload) {
		m176.irq_count = m176.irq_latch;
	} else {
		m176.irq_count--;
	}
	if (!m176.irq_count && m176.irq_enabled) {
		X6502_IRQBegin(FCEU_IQEXT);
	}
	m176.irq_reload = FALSE;
}

static void RegReset(void) {
	memset(&m176, 0, sizeof(m176));
	m176.mmc3_regs[0] = 0;
	m176.mmc3_regs[1] = 2;
	m176.mmc3_regs[2] = 4;
	m176.mmc3_regs[3] = 5;
	m176.mmc3_regs[4] = 6;
	m176.mmc3_regs[5] = 7;
	m176.mmc3_regs[6] = 0;
	m176.mmc3_regs[7] = 1;
	m176.mmc3_regs[8] = ~1;
	m176.mmc3_regs[9] = ~0;
	m176.mmc3_regs[10] = ~0;
	m176.mmc3_regs[11] = ~0;

	if (iNESCart.submapper == 2) {
		m176.mmc3_wram = 0xC0;
	} else {
		m176.mmc3_wram = 0x80;
	}

	if (iNESCart.submapper == 1) {
		m176.fk23_regs[1] = ~0;
	}

	SyncPRG();
	SyncCHR();
	SyncWRAM();
	SyncMirror();
}

static void Reset(void) {
	/* this little hack makes sure that we try all the dip switch settings eventually, if we reset enough */
	if (dipsw_enable) {
		dipswitch = (dipswitch + 1) & 7;
		FCEU_printf("BMCFK23C dipswitch set to $%04x\n", 0x5000 | 0x10 << dipswitch);
	}

	RegReset();
}

static void Power(void) {
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

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncWRAM();
	SyncMirror();
}

static void InitCommon(CartInfo *info) {
	/* Initialization for iNES and UNIF. iNESCart.submapper and dipsw_enable must have been set. */
	info->Power = Power;
	info->Reset = Reset;
	GameHBIRQHook = HBIRQHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	if (CHRRAMSIZE) {
		CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
		SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);
		AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
	}

	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
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
			WRAMSIZE = SIZE_32K;
		} else {
			/* Always enable WRAM for iNES-headered files */
			WRAMSIZE = SIZE_8K;

			if ((ROM.prg.size == SIZE_1M) && (ROM.chr.size == SIZE_1M)) {
				info->submapper = 1;
			} else if ((ROM.prg.size == SIZE_256K) && (ROM.chr.size == SIZE_128K)) {
				info->submapper = 1;
			} else if ((ROM.prg.size == SIZE_128K) && (ROM.chr.size == SIZE_64K)) {
				info->submapper = 1;
			} else if ((ROM.prg.size >= SIZE_8M) && !ROM.chr.size) {
				info->submapper = 2;
			} else if ((ROM.prg.size == SIZE_4M) && !ROM.chr.size) {
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
	InitCommon(info);
}

/* Jncota board with unusual wiring that turns 1 KiB CHR banks into 2 KiB banks, and has hard-wired nametable mirroring. */
void Mapper523_Init(CartInfo *info) { /* Jncota Fengshengban */
	WRAMSIZE = SIZE_8K;
	dipsw_enable = 0;
	after_power = 0;
	info->submapper = 1;
	InitCommon(info);
}

/* UNIF LOADER */

/* UNIF FK23C. Also includes mislabelled WAIXING-FS005, recognizable by their PRG-ROM size. */
void BMCFK23C_Init(CartInfo *info) {
	if (ROM.chr.size) {
		/* Rockman I-VI uses mixed chr rom/ram */
		if (ROM.prg.size == SIZE_2M && ROM.chr.size == SIZE_512K) {
			CHRRAMSIZE = SIZE_8K;
		}
	}
	WRAMSIZE = SIZE_8K;
	dipsw_enable = 0;
	after_power = 1;
	info->submapper = (ROM.prg.size >= SIZE_4M) ? 2 : ((ROM.prg.size == SIZE_64K) && ROM.chr.size == SIZE_128K) ? 1 : 0;
	if (info->submapper == 2) {
		CHRRAMSIZE = SIZE_256K;
	}
	InitCommon(info);
}

/* UNIF FK23CA. Also includes mislabelled WAIXING-FS005, recognizable by their PRG-ROM size. */
void BMCFK23CA_Init(CartInfo *info) {
	WRAMSIZE = SIZE_8K;
	dipsw_enable = 0;
	after_power = 1;
	info->submapper = (ROM.prg.size >= SIZE_2M) ? 2 : 1;
	if (info->submapper == 2) {
		CHRRAMSIZE = SIZE_256K;
	}
	InitCommon(info);
}

/* UNIF BMC-Super24in1SC03 */
void Super24_Init(CartInfo *info) {
	CHRRAMSIZE = SIZE_8K;
	dipsw_enable = 0;
	after_power = 0;
	info->submapper = 0;
	InitCommon(info);
}

/* UNIF WAIXING-FS005 */
void WAIXINGFS005_Init(CartInfo *info) {
	CHRRAMSIZE = SIZE_8K;
	WRAMSIZE = SIZE_32K;
	dipsw_enable = 0;
	after_power = 0;
	info->submapper = 2;
	InitCommon(info);
}
