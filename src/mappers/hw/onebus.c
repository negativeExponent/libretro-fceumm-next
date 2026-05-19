/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007-2010 CaH4e3
 *  Copyright (C) 2025-2026 negativeExponent
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
 * VR02/VT03 Console and OneBus System (incomplete)
 *
 * Street Dance (Dance pad) (Unl)
 * 101-in-1 Arcade Action II
 * DreamGEAR 75-in-1, etc.
 *
 */

#include "mapinc.h"
#include "onebus.h"

static void (*WSync)(void);

static writefunc defapuwrite[64];
static readfunc defapuread[64];

ONEBUS onebus;

static SFORMAT StateRegs[] = {
	{ onebus.cpu41xx, 0x100, "REGC" },
	{ onebus.ppu20xx, 0x100, "REGS" },
	{ onebus.apu40xx, 0x40, "REGA" },
	{ &onebus.IRQReload, 1, "IRQR" },
	{ &onebus.IRQCount, 1, "IRQC" },
	{ &onebus.IRQa, 1, "IRQA" },
	{ &onebus.pcm_enable, 1, "PCME" },
	{ &onebus.pcm_irq, 1, "PCMI" },
	{ &onebus.pcm_addr, 2 | FCEUSTATE_RLSB, "PCMA" },
	{ &onebus.pcm_size, 2 | FCEUSTATE_RLSB, "PCMS" },
	{ &onebus.pcm_latch, 2 | FCEUSTATE_RLSB, "PCML" },
	{ &onebus.pcm_clock, 2 | FCEUSTATE_RLSB, "PCMC" },
	{ 0 }
};

void OneBus_SyncPRG(uint16_t mmask, uint16_t mblock) {
	uint8_t mode = onebus.cpu41xx[0x0B] & 0x07;
	uint16_t mask = (mode == 7) ? 0xFF : (0x3F >> mode);
	uint16_t block = (((onebus.cpu41xx[0] & 0xF0) << 4) | onebus.cpu41xx[0x0A]) & ~mask;
	uint16_t pswap = (onebus.cpu41xx[0x05] & 0x40) << 8;

	setprg8(0x8000 ^ pswap, mblock | (((block | (onebus.cpu41xx[0x07] & mask)) + onebus.relative_8k) & mmask));
	setprg8(0xA000,         mblock | (((block | (onebus.cpu41xx[0x08] & mask)) + onebus.relative_8k) & mmask));
	setprg8(0xC000 ^ pswap, mblock | (((block | (((onebus.cpu41xx[0x0B] & 0x40) ? onebus.cpu41xx[0x09] : (~1)) & mask)) + onebus.relative_8k) & mmask));
	setprg8(0xE000,         mblock | (((block | ((~0) & mask)) + onebus.relative_8k) & mmask));

	if ((iNESCart.ConsoleType == CONSOLE_VT369) && (onebus.cpu41xx[0x1C] & 0x40)) {
		setprg8(0x6000, mblock | (((block | (onebus.cpu41xx[0x12] & mask)) + onebus.relative_8k) & mmask));
	} else if (WRAM) {
		setprg8r(0x10, 0x6000, 0);
	}
}

void OneBus_SyncPRG16(uint16_t bank0, uint16_t bank1, uint16_t mmask, uint16_t mblock) {
	uint8_t mode = onebus.cpu41xx[0x0B] & 0x07;
	uint16_t mask = (mode == 7) ? 0xFF : (0x3F >> mode);
	uint16_t block = (((onebus.cpu41xx[0] & 0xF0) << 4) | onebus.cpu41xx[0x0A]) & ~mask;

	setprg8(0x8000, mblock | ((block | (((bank0 << 1) | 0) & mask)) & mmask));
	setprg8(0xA000, mblock | ((block | (((bank0 << 1) | 1) & mask)) & mmask));
	setprg8(0xC000, mblock | ((block | (((bank1 << 1) | 0) & mask)) & mmask));
	setprg8(0xE000, mblock | ((block | (((bank1 << 1) | 1) & mask)) & mmask));

	if ((iNESCart.ConsoleType == CONSOLE_VT369) && (onebus.cpu41xx[0x1C] & 0x40)) {
		setprg8(0x6000, mblock | ((block | (onebus.cpu41xx[0x12] & mask)) & mmask));
	} else if (iNESCart.PRGRamSize) {
		setprg8r(0x10, 0x6000, 0);
	}
}

void OneBus_SetCHR(uint8_t **banks, uint8_t *base, uint8_t bit4pp, uint8_t extended, uint16_t EVA, uint16_t mmask, uint16_t mblock) {
	static const uint8_t chr_mask_lut[8] = { 0xFF, 0x7F, 0x3F, 0xFF, 0x1F, 0x0F, 0x07, 0x7F };
	uint16_t mask = chr_mask_lut[onebus.ppu20xx[0x1A] & 0x07];
	uint16_t block = (onebus.ppu20xx[0x1A] & 0xF8) & ~mask;
	uint16_t cswap = (onebus.cpu41xx[0x05] & 0x80) ? 4 : 0;

	uint32_t chrMask = onebus.chr.size - 1;
	uint16_t relative = onebus.relative_8k << 3;

	if (bit4pp) {
		relative >>= 1;
		chrMask >>= 1;
		mmask >>= 1;
		mblock >>= 1;
	} else {
		base = onebus.chr.data;
	}

	if (extended) {
		#define CHRPTR(value) ((((mblock | ((((block | ((value) & mask)) << 3) | EVA | ((onebus.cpu41xx[0] & 0x0F) << 11)) & mmask)) + relative) << 10) & chrMask)
		banks[0 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x16] & ~1) ] - (0x0000 ^ (cswap ? 0x1000 : 0));
		banks[1 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x16] |  1) ] - (0x0400 ^ (cswap ? 0x1000 : 0));
		banks[2 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x17] & ~1) ] - (0x0800 ^ (cswap ? 0x1000 : 0));
		banks[3 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x17] |  1) ] - (0x0C00 ^ (cswap ? 0x1000 : 0));
		banks[4 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x12])      ] - (0x1000 ^ (cswap ? 0x1000 : 0));
		banks[5 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x13])      ] - (0x1400 ^ (cswap ? 0x1000 : 0));
		banks[6 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x14])      ] - (0x1800 ^ (cswap ? 0x1000 : 0));
		banks[7 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x15])      ] - (0x1C00 ^ (cswap ? 0x1000 : 0));
		#undef CHRPTR
	} else {
		block |= (onebus.ppu20xx[0x18] & 0x70) << 4;
		#define CHRPTR(value) ((((mblock | (((block | ((value) & mask)) | ((onebus.cpu41xx[0] & 0x0F) << 11)) & mmask)) + relative) << 10) & chrMask)
		banks[0 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x16] & ~1) ] - (0x0000 ^ (cswap ? 0x1000 : 0));
		banks[1 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x16] |  1) ] - (0x0400 ^ (cswap ? 0x1000 : 0));
		banks[2 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x17] & ~1) ] - (0x0800 ^ (cswap ? 0x1000 : 0));
		banks[3 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x17] |  1) ] - (0x0C00 ^ (cswap ? 0x1000 : 0));
		banks[4 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x12])      ] - (0x1000 ^ (cswap ? 0x1000 : 0));
		banks[5 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x13])      ] - (0x1400 ^ (cswap ? 0x1000 : 0));
		banks[6 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x14])      ] - (0x1800 ^ (cswap ? 0x1000 : 0));
		banks[7 ^ cswap] = &base[ CHRPTR(onebus.ppu20xx[0x15])      ] - (0x1C00 ^ (cswap ? 0x1000 : 0));
		#undef CHRPTR
	}
}

extern uint8_t **VPageR;
void OneBus_SyncCHR(uint16_t mmask, uint16_t mblock) {
#define BK16EN  (onebus.ppu20xx[0x10] & 0x02)
#define SP16EN  (onebus.ppu20xx[0x10] & 0x04)
#define SPEXTEN (onebus.ppu20xx[0x10] & 0x08)
#define BKEXTEN (onebus.ppu20xx[0x10] & 0x10)
#define V16BEN  ((onebus.ppu20xx[0x10] & 0x40) && (iNESCart.submapper != 6) || (iNESCart.submapper == 7))
#define VRWB    (onebus.ppu20xx[0x18] & 0x07)
#define BKPAGE  (onebus.ppu20xx[0x18] & 0x08)

	OneBus_SetCHR(VPageR, V16BEN ? onebus.chr.low16 : onebus.chr.low, BK16EN || SP16EN, BKEXTEN || SPEXTEN, VRWB, mmask, mblock); /* 0000-1FFF: 2007 CHR Low */
#if 0
	OneBus_SetCHR(0x10, V16BEN ? onebus.chr.high16 : onebus.chr.high, BK16EN || SP16EN, BKEXTEN || SPEXTEN, VRWB, mmask, mblock);/* 4000-5FFF: 2007 CHR High */
	OneBus_SetCHR(0x20, V16BEN ? onebus.chr.low16 : onebus.chr.low, BK16EN, BKEXTEN, BKPAGE ? 4 : 0, mmask, mblock);			  /* 8000-9FFF: BG   CHR Low */
	OneBus_SetCHR(0x28, V16BEN ? onebus.chr.low16 : onebus.chr.low, SP16EN, SPEXTEN, 0, mmask, mblock);						  /* A000-BFFF: SPR  CHR Low */
	OneBus_SetCHR(0x30, V16BEN ? onebus.chr.high16 : onebus.chr.high, BK16EN, BKEXTEN, BKPAGE ? 4 : 0, mmask, mblock);			  /* C000-BFFF: BG   CHR High */
	OneBus_SetCHR(0x38, V16BEN ? onebus.chr.high16 : onebus.chr.high, SP16EN, SPEXTEN, 0, mmask, mblock);						  /* E000-FFFF: SPR  CHR High */
#endif
}

void OneBus_SyncMirror(void) {
	setmirror((onebus.cpu41xx[0x06] & 1) ^ 1);
}

/* write $2010 - $2FFF */
DECLFW(OneBus_WritePPU20XX) {
	/*	FCEU_printf("PPU %04x:%04x\n",A,V); */
	if (A >= 0x2010) {
		if (iNESCart.ConsoleType <= CONSOLE_VT09) {
			A &= ~0x40;
		}
		onebus.ppu20xx[A & 0x00FF] = V;
		WSync();
	}
}

/* read $4000 - $403F */
DECLFR(OneBus_ReadAPU40XX) {
	uint8_t result = defapuread[A & 0x003F](A);
	/*	FCEU_printf("read %04x, %02x\n",A,result); */
	switch (A & 0x3F) {
	case 0x15:
		if (onebus.apu40xx[0x30] & 0x10) {
			result = (result & 0x7F) | onebus.pcm_irq;
		}
		break;
	}
	return result;
}

/* write $4000 - $403F */
DECLFW(OneBus_WriteAPU40XX) {
	/* if(((A & 0x3f)!=0x16) && ((onebus.apu40xx[0x30] & 0x10) || ((A & 0x3f)>0x17)))FCEU_printf("APU %04x:%04x\n",A,V);  */
	if ((A >= 0x4000) && (A <= 0x403F)) {
		onebus.apu40xx[A & 0x3F] = V;
		switch (A & 0x3F) {
		case 0x12:
			if (onebus.apu40xx[0x30] & 0x10) {
				onebus.pcm_addr = V << 6;
			}
			break;
		case 0x13:
			if (onebus.apu40xx[0x30] & 0x10) {
				onebus.pcm_size = (V << 4) + 1;
			}
			break;
		case 0x15:
			if (onebus.apu40xx[0x30] & 0x10) {
				onebus.pcm_enable = V & 0x10;
				if (onebus.pcm_irq) {
					X6502_IRQEnd(FCEU_IQEXT);
					onebus.pcm_irq = 0;
				}
				if (onebus.pcm_enable) {
					onebus.pcm_latch = onebus.pcm_clock;
				}
				V &= 0xEF;
			}
			break;
		}
	}
	defapuwrite[A & 0x3F](A, V);
}

/* read $4100 - 4FFF */
DECLFR(OneBus_ReadCPU41XX) {
	if ((A >= 0x4100) && (A <= 0x4FFF)) {
		switch (A & 0x0FFF) {
		case 0x140: case 0x141: case 0x142: case 0x143: case 0x144: case 0x145: case 0x146: case 0x147:
		case 0x148: case 0x149: case 0x14A: case 0x14B: case 0x14C: case 0x14D: case 0x14E: case 0x14F:
		case 0x150: case 0x151: case 0x152: case 0x153: case 0x154: case 0x155: case 0x156: case 0x157:
		case 0x158: case 0x159: case 0x15A: case 0x15B: case 0x15D: case 0x15E: case 0x15F:
			if (iNESCart.ConsoleType == CONSOLE_VT369) {
				/* TODO: VT369 gpio */
				/* FCEU_printf("unimplemented read: %04x\n", A); */
			}
			return 0xFF;
		case 0x15C:
			return (0x10);
		case 0x18A:
			return (0x04);
		case 0x1B7:
			return (0x04);
		case 0x1B9:
			return (0x80);
		default:
			if ((A <= 0x410D) || ((A >= 0x4160) && (A < 0x4800))) {
				return (onebus.cpu41xx[A & 0xFF]);
			}
		}
	}
}

/* write $4100 - $41FF */
DECLFW(OneBus_WriteCPU41XX) {
	/*	FCEU_printf("CPU %04x:%04x\n",A,V); */
	if ((A >= 0x4100) && (A <= 0x41FF)) {
		onebus.cpu41xx[A & 0xFF] = V;
		switch (A & 0xFFF) {
		case 0x101:
			onebus.cpu41xx[0x01] = V & 0xFE;
			break;
		case 0x102:
			onebus.IRQReload = TRUE;
			break;
		case 0x103:
			X6502_IRQEnd(FCEU_IQEXT);
			onebus.IRQa = FALSE;
			break;
		case 0x104:
			onebus.IRQa = TRUE;
			break;
		case 0x140: case 0x141: case 0x142: case 0x143: case 0x144: case 0x145: case 0x146: case 0x147:
		case 0x148: case 0x149: case 0x14A: case 0x14B: case 0x14C: case 0x14D: case 0x14E: case 0x14F:
		case 0x150: case 0x151: case 0x152: case 0x153: case 0x154: case 0x155: case 0x156: case 0x157:
		case 0x158: case 0x159: case 0x15A: case 0x15B: case 0x15C: case 0x15D: case 0x15E: case 0x15F:
			if (iNESCart.ConsoleType == CONSOLE_VT369) {
				/* TODO: VT369 gpio */
				/* FCEU_printf("unimplemented write: %04x <- %02x\n", A, V); */
			}
			break;
		case 0x160:
		case 0x161:
			/* VT369 relative address */
			if (iNESCart.ConsoleType == CONSOLE_VT369) {
				onebus.relative_8k = ((onebus.cpu41xx[0x61] << 8) & 0xF00) | onebus.cpu41xx[0x60];
			}
			break;
		}
		WSync();
	}
}

/* WRITE $8000 - $FFFF */
DECLFW(OneBus_WriteMMC3) {
	/*	FCEU_printf("MMC %04x:%04x\n",A,V); */
	if (onebus.cpu41xx[0x0B] & 0x08) {
		CartBW(A, V);
	} else {
		switch (A & 0xE001) {
		case 0x8000:
			OneBus_WriteCPU41XX(0x4105, V & ~0x20);
			break;
		case 0x8001: {
			uint8_t reg = onebus.cpu41xx[0x05] & 7;
			switch (reg) {
			case 0:
			case 1:
				OneBus_WritePPU20XX(0x2016 + reg, V);
				break;
			case 2:
			case 3:
			case 4:
			case 5:
				OneBus_WritePPU20XX(0x2010 + reg, V);
				break;
			case 6:
			case 7:
				OneBus_WriteCPU41XX(0x4101 + reg, V);
				break;
			}
			break;
		}
		case 0xA000:
			OneBus_WriteCPU41XX(0x4106, V);
			break;
		case 0xC000:
			OneBus_WriteCPU41XX(0x4101, V);
			break;
		case 0xC001:
			OneBus_WriteCPU41XX(0x4102, V);
			break;
		case 0xE000:
			OneBus_WriteCPU41XX(0x4103, V);
			break;
		case 0xE001:
			OneBus_WriteCPU41XX(0x4104, V);
			break;
		}
	}
}

static void OneBus_IRQHook(void) {
	uint32_t count = onebus.IRQCount;
	if (!count || onebus.IRQReload) {
		onebus.IRQCount = onebus.cpu41xx[0x01];
		onebus.IRQReload = 0;
	} else {
		onebus.IRQCount--;
	}
	if (count && !onebus.IRQCount) {
		if (onebus.IRQa) {
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void OneBus_CpuHook(int a) {
	if (onebus.pcm_enable) {
		onebus.pcm_latch -= a;
		if (onebus.pcm_latch <= 0) {
			onebus.pcm_latch += onebus.pcm_clock;
			onebus.pcm_size--;
			if (onebus.pcm_size < 0) {
				onebus.pcm_irq = 0x80;
				onebus.pcm_enable = 0;
				X6502_IRQBegin(FCEU_IQEXT);
			} else {
				uint16_t addr = onebus.pcm_addr | ((onebus.apu40xx[0x30] ^ 3) << 14);
				uint8_t raw_pcm = ARead[addr](addr) >> 1;
				defapuwrite[0x11](0x4011, raw_pcm);
				onebus.pcm_addr++;
				onebus.pcm_addr &= 0x7FFF;
			}
		}
	}
}

void OneBus_Power(void) {
	uint32_t i;

	for (i = 0; i < 64; i++) {
		defapuread[i] = GetReadHandler(0x4000 | i);
		defapuwrite[i] = GetWriteHandler(0x4000 | i);
	}
	SetReadHandler(0x4000, 0x403F, OneBus_ReadAPU40XX);
	SetWriteHandler(0x4000, 0x403F, OneBus_WriteAPU40XX);

	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x2010, 0x2FFF, OneBus_WritePPU20XX);

	SetReadHandler(0x4100, 0x4FFF, OneBus_ReadCPU41XX);
	SetWriteHandler(0x4100, 0x41FF, OneBus_WriteCPU41XX);

	SetWriteHandler(0x8000, 0xFFFF, OneBus_WriteMMC3);

	if (iNESCart.ConsoleType == CONSOLE_VT369) {
		OneBus_WriteCPU41XX(0x4162, 0);
		/* TODOL gpio */
		/* $3000 - $3FFF reads/writes */
	}

	if (WRAM) {
		FCEU_CheatAddRAM(8, 0x6000, WRAM);
	}

	memset(onebus.cpu41xx, 0, sizeof(onebus.cpu41xx));
	memset(onebus.ppu20xx, 0, sizeof(onebus.ppu20xx));
	memset(onebus.apu40xx, 0, sizeof(onebus.apu40xx));

	onebus.pcm_enable = onebus.pcm_irq = 0;
	onebus.pcm_addr = onebus.pcm_size = onebus.pcm_latch = 0;
	onebus.pcm_clock = 0xE1;

	OneBus_Reset();
}

void OneBus_Reset(void) {
	int i;

	onebus.IRQReload = onebus.IRQCount = onebus.IRQa = 0;

	onebus.relative_8k = 0x00;
	onebus.ppu20xx[0x10] = 0x00;
	onebus.ppu20xx[0x12] = 0x04;
	onebus.ppu20xx[0x13] = 0x05;
	onebus.ppu20xx[0x14] = 0x06;
	onebus.ppu20xx[0x15] = 0x07;
	onebus.ppu20xx[0x16] = 0x00;
	onebus.ppu20xx[0x17] = 0x02;
	onebus.ppu20xx[0x18] = 0x00;
	onebus.ppu20xx[0x1A] = 0x00;
	onebus.cpu41xx[0x00] = 0x00;
	onebus.cpu41xx[0x05] = 0x00;
	onebus.cpu41xx[0x07] = 0x00;
	onebus.cpu41xx[0x08] = 0x01;
	onebus.cpu41xx[0x09] = 0xFE;
	onebus.cpu41xx[0x0A] = 0x00;
	onebus.cpu41xx[0x0B] = 0x00;
	onebus.cpu41xx[0x0F] = 0xFF;
	onebus.cpu41xx[0x60] = 0x00;
	onebus.cpu41xx[0x61] = 0x00;

	WSync();
}

static void StateRestore(int version) {
	WSync();
}

static void OneBus_Close(void) {
	if (onebus.chr.low) {
		FCEU_free(onebus.chr.low);
		onebus.chr.low = NULL;
	}
	if (onebus.chr.high) {
		FCEU_free(onebus.chr.high);
		onebus.chr.high = NULL;
	}
	if (onebus.chr.low16) {
		FCEU_free(onebus.chr.low16);
		onebus.chr.low16 = NULL;
	}
	if (onebus.chr.high16) {
		FCEU_free(onebus.chr.high16);
		onebus.chr.high16 = NULL;
	}
}

void OneBus_Init(CartInfo *info, void (*proc)(void), int wram, int battery) {
	uint32_t i;

	WSync = proc;

	if (ROM.chr.size) {
		onebus.chr.data = ROM.chr.data;
		onebus.chr.size = ROM.chr.size;
	} else {
		onebus.chr.data = ROM.prg.data;
		onebus.chr.size = ROM.prg.size;
	}

	{
		int ssize = 0;
		while ((onebus.chr.size - 1) >> ssize) {
			ssize++;
		}
		onebus.chr.size = 1 << ssize;
	}

	onebus.chr.low = (uint8_t *)FCEU_malloc(onebus.chr.size >> 1);
	onebus.chr.high = (uint8_t *)FCEU_malloc(onebus.chr.size >> 1);
	if ((iNESCart.ConsoleType == CONSOLE_VT09) || (iNESCart.ConsoleType == CONSOLE_VT369)) {
		onebus.chr.low16 = (uint8_t *)FCEU_malloc(onebus.chr.size >> 1);
		onebus.chr.high16 = (uint8_t *)FCEU_malloc(onebus.chr.size >> 1);
	}

	for (i = 0; i < onebus.chr.size; i++) {
		int shiftedAddress = i & 0x0F | i >> 1 & ~0x0F;
		if (i & 0x10) {
			onebus.chr.high[shiftedAddress] = onebus.chr.data[i];
		} else {
			onebus.chr.low[shiftedAddress] = onebus.chr.data[i];
		}
	}
	if ((iNESCart.ConsoleType == CONSOLE_VT09) || (iNESCart.ConsoleType == CONSOLE_VT369)) {
		for (i = 0; i < onebus.chr.size; i++) {
			if (i & 0x1) {
				onebus.chr.high16[i >> 1] = onebus.chr.data[i];
			} else {
				onebus.chr.low16[i >> 1] = onebus.chr.data[i];
			}
		}
	}

	info->Power = OneBus_Power;
	info->Reset = OneBus_Reset;
	info->Close = OneBus_Close;

	if (wram) {
		WRAMSIZE = wram * 1024;
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}

	GameHBIRQHook = OneBus_IRQHook;
	MapIRQHook = OneBus_CpuHook;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);
}
