/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2005 CaH4e3
 *  Copyright (C) 2019 Libretro Team
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

#include "mapinc.h"
#include "jyasic.h"

JYASIC jyasic = { 0 };

static uint8_t dipSwitch;
static uint8_t allow_extended_mirroring;
static uint32_t lastPPUAddress;

uint8_t JYASIC_CPUWriteHandlersSet;
writefunc JYASIC_cpuWrite[0x10000]; /* Actual write handlers for CPU write trapping as a method fo IRQ clocking */

void (*JYASIC_pwrap)(uint16_t A, uint32_t V);
void (*JYASIC_wwrap)(uint16_t A, uint32_t V);
void (*JYASIC_cwrap)(uint16_t A, uint32_t V);
void (*JYASIC_mwrap)(uint16_t A, uint32_t V);

static SFORMAT JYASIC_StateRegs[] = {
	{ jyasic.mode, 4, "TKCO" },
	{ jyasic.prg, 4, "PRGB" },
	{ jyasic.mul, 2, "MUL" },
	{ jyasic.latch, 2, "CLTC" },
	{ &jyasic.chr[0], 2 | FCEUSTATE_RLSB, "JCH0" },
	{ &jyasic.chr[1], 2 | FCEUSTATE_RLSB, "JCH1" },
	{ &jyasic.chr[2], 2 | FCEUSTATE_RLSB, "JCH2" },
	{ &jyasic.chr[3], 2 | FCEUSTATE_RLSB, "JCH3"},
	{ &jyasic.chr[4], 2 | FCEUSTATE_RLSB, "JCH4" },
	{ &jyasic.chr[5], 2 | FCEUSTATE_RLSB, "JCH5" },
	{ &jyasic.chr[6], 2 | FCEUSTATE_RLSB, "JCH6" },
	{ &jyasic.chr[7], 2 | FCEUSTATE_RLSB, "JCH7" },
	{ &jyasic.nt[0], 2 | FCEUSTATE_RLSB, "JNT0" },
	{ &jyasic.nt[1], 2 | FCEUSTATE_RLSB, "JNT1" },
	{ &jyasic.nt[2], 2 | FCEUSTATE_RLSB, "JNT2" },
	{ &jyasic.nt[3], 2 | FCEUSTATE_RLSB, "JNT3" },
	{ &jyasic.adder, 1, "ADDE" },
	{ &jyasic.test, 1, "REGI" },
	{ &jyasic.irq.control, 1, "IRQM" },
	{ &jyasic.irq.prescaler, 1, "IRQP" },
	{ &jyasic.irq.counter, 1, "IRQC" },
	{ &jyasic.irq.xor, 1, "IRQX" },
	{ &jyasic.irq.enable, 1, "IRQA" },
	{ 0 }
};

static uint32_t JYASIC_GetPRGBank_default(uint32_t V) {
	return 0;
}
static uint32_t JYASIC_GetCHRBank_default(uint32_t V) {
	return 0;
}

static void SetPRG_default(uint16_t A, uint32_t V) {
	setprg8(A, V);
}

static void SetCHR_default(uint16_t A, uint32_t V) {
	setchr1(A, V);
}

static void SetWRAM_default(uint16_t A, uint32_t V) {
	setprg8(A, V);
}

static void SetNTMirror_default(uint16_t A, uint32_t V) {
	setntamem(CHRptr[0] + 0x400 * (V & CHRmask1[0]), 0, A & 0x03);
}

static uint8_t reverse_bits(uint8_t val) {
	return (((val << 6) & 0x40) |
		((val << 4) & 0x20) |
		((val << 2) & 0x10) |
		((val << 0) & 0x08) |
		((val >> 2) & 0x04) |
		((val >> 4) & 0x02) |
		((val >> 6) & 0x01));
}

void JYASIC_SyncPRG(void) {
	uint8_t prgLast = (jyasic.mode[0] & 0x04) ? jyasic.prg[3] : 0xFF;

	switch (jyasic.mode[0] & 0x03) {
	case 0:
		JYASIC_pwrap(0x8000, (prgLast << 2) | 0);
		JYASIC_pwrap(0xA000, (prgLast << 2) | 1);
		JYASIC_pwrap(0xC000, (prgLast << 2) | 2);
		JYASIC_pwrap(0xE000, (prgLast << 2) | 3);
		break;
	case 1:
		JYASIC_pwrap(0x8000, (jyasic.prg[1] << 1) | 0);
		JYASIC_pwrap(0xA000, (jyasic.prg[1] << 1) | 1);
		JYASIC_pwrap(0xC000, (prgLast << 1) | 0);
		JYASIC_pwrap(0xE000, (prgLast << 1) | 1);
		break;
	case 2:
		JYASIC_pwrap(0x8000, jyasic.prg[0]);
		JYASIC_pwrap(0xA000, jyasic.prg[1]);
		JYASIC_pwrap(0xC000, jyasic.prg[2]);
		JYASIC_pwrap(0xE000, prgLast);
		break;
	case 3:
		JYASIC_pwrap(0x8000, reverse_bits(jyasic.prg[0]));
		JYASIC_pwrap(0xA000, reverse_bits(jyasic.prg[1]));
		JYASIC_pwrap(0xC000, reverse_bits(jyasic.prg[2]));
		JYASIC_pwrap(0xE000, reverse_bits(prgLast));
		break;
	}
}

void JYASIC_SyncCHR(void) {
	switch (jyasic.mode[0] & 0x18) {
	case 0x00: /* 8 KiB CHR mode */
		JYASIC_cwrap(0x0000, (jyasic.chr[0] << 3) | 0);
		JYASIC_cwrap(0x0400, (jyasic.chr[0] << 3) | 1);
		JYASIC_cwrap(0x0800, (jyasic.chr[0] << 3) | 2);
		JYASIC_cwrap(0x0C00, (jyasic.chr[0] << 3) | 3);
		JYASIC_cwrap(0x1000, (jyasic.chr[0] << 3) | 4);
		JYASIC_cwrap(0x1400, (jyasic.chr[0] << 3) | 5);
		JYASIC_cwrap(0x1800, (jyasic.chr[0] << 3) | 6);
		JYASIC_cwrap(0x1C00, (jyasic.chr[0] << 3) | 7);
		break;
	case 0x08: /* 4 KiB CHR mode */
		JYASIC_cwrap(0x0000, (jyasic.chr[(jyasic.latch[0] & 0x02) | 0] << 2) | 0);
		JYASIC_cwrap(0x0400, (jyasic.chr[(jyasic.latch[0] & 0x02) | 0] << 2) | 1);
		JYASIC_cwrap(0x0800, (jyasic.chr[(jyasic.latch[0] & 0x02) | 0] << 2) | 2);
		JYASIC_cwrap(0x0C00, (jyasic.chr[(jyasic.latch[0] & 0x02) | 0] << 2) | 3);
		JYASIC_cwrap(0x1000, (jyasic.chr[(jyasic.latch[1] & 0x02) | 4] << 2) | 0);
		JYASIC_cwrap(0x1400, (jyasic.chr[(jyasic.latch[1] & 0x02) | 4] << 2) | 1);
		JYASIC_cwrap(0x1800, (jyasic.chr[(jyasic.latch[1] & 0x02) | 4] << 2) | 2);
		JYASIC_cwrap(0x1C00, (jyasic.chr[(jyasic.latch[1] & 0x02) | 4] << 2) | 3);
		break;
	case 0x10: /* 2 KiB CHR mode */
		JYASIC_cwrap(0x0000, (jyasic.chr[0] << 1) | 0);
		JYASIC_cwrap(0x0400, (jyasic.chr[0] << 1) | 1);
		JYASIC_cwrap(0x0800, (jyasic.chr[2] << 1) | 0);
		JYASIC_cwrap(0x0C00, (jyasic.chr[2] << 1) | 1);
		JYASIC_cwrap(0x1000, (jyasic.chr[4] << 1) | 0);
		JYASIC_cwrap(0x1400, (jyasic.chr[4] << 1) | 1);
		JYASIC_cwrap(0x1800, (jyasic.chr[6] << 1) | 0);
		JYASIC_cwrap(0x1C00, (jyasic.chr[6] << 1) | 1);
		break;
	case 0x18: /* 1 KiB CHR mode */
		JYASIC_cwrap(0x0000, jyasic.chr[0]);
		JYASIC_cwrap(0x0400, jyasic.chr[1]);
		JYASIC_cwrap(0x0800, jyasic.chr[2]);
		JYASIC_cwrap(0x0C00, jyasic.chr[3]);
		JYASIC_cwrap(0x1000, jyasic.chr[4]);
		JYASIC_cwrap(0x1400, jyasic.chr[5]);
		JYASIC_cwrap(0x1800, jyasic.chr[6]);
		JYASIC_cwrap(0x1C00, jyasic.chr[7]);
		break;
	}

	PPUCHRRAM = (jyasic.mode[2] & 0x40) ? 0xFF : 0x00; /* Write-protect or write-enable CHR-RAM */
}

void JYASIC_SyncWRAM(void) {
	uint8_t prg6000 = jyasic.prg[3];

	switch (jyasic.mode[0] & 0x03) {
	case 0:
		prg6000 = (prg6000 << 2) | 3;
		break;
	case 1:
		prg6000 = (prg6000 << 1) | 1;
		break;
	case 2:
		break;
	case 3:
		prg6000 = reverse_bits(prg6000);
		break;
	}
	if (jyasic.mode[0] & 0x80) { /* Map ROM */
		JYASIC_wwrap(0x6000, prg6000);
	} else if (WRAMSIZE) { /* Otherwise map WRAM if it exists */
		setprg8r(0x10, 0x6000, 0);
	}
}

void JYASIC_SyncMirror(void) {
	if (jyasic.mode[0] & 0x20) {
		int i;
		for (i = 0; i < 4; i++) {
			if (((jyasic.nt[i] ^ jyasic.mode[2]) & 0x80) | (jyasic.mode[0] & 0x40)) {
				JYASIC_mwrap(i, jyasic.nt[i]);
			} else {
				setntamem(NTARAM + (0x0400 * (jyasic.nt[i] & 0x01)), TRUE, i);
			}
		}
	} else if (jyasic.mode[1] & 0x08) {
		int i;
		for (i = 0; i < 4; i++) {
			setntamem(NTARAM + (0x0400 * (jyasic.nt[i] & 0x01)), TRUE, i);
		}
	} else {
		switch (jyasic.mode[1] & 0x03) {
		/* Regularly mirrored CIRAM */
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

static void clockIRQ(void) {
	uint8_t mask = jyasic.irq.control & 0x04 ? 0x07 : 0xFF;
	uint8_t prescaler = jyasic.irq.prescaler & mask;
	uint8_t clockIrqCounter = FALSE;

	if (jyasic.irq.enable) {
		switch (jyasic.irq.control & 0xC0) {
		case 0x40:
			prescaler++;
			if ((prescaler & mask) == 0) {
				clockIrqCounter = TRUE;
			}
			break;
		case 0x80:
			if (--prescaler == 0) {
				clockIrqCounter = TRUE;
			}
			break;
		}

		jyasic.irq.prescaler = (jyasic.irq.prescaler & ~mask) | (prescaler & mask);

		if (clockIrqCounter) {
			switch (jyasic.irq.control & 0xC0) {
			case 0x40:
				if ((jyasic.irq.control & 0x08) == 0) {
					jyasic.irq.counter++;
				}
				if (jyasic.irq.counter == 0x00) {
					X6502_IRQBegin(FCEU_IQEXT);
				}
				break;
			case 0x80:
				if ((jyasic.irq.control & 0x08) == 0) {
					jyasic.irq.counter--;
				}
				if (jyasic.irq.counter == 0xFF) {
					X6502_IRQBegin(FCEU_IQEXT);
				}
				break;
			}
		}
	}
}

DECLFW(JYASIC_trapCPUWrite) {
	if ((jyasic.irq.control & 0x03) == 0x03) {
		clockIRQ(); /* Clock IRQ counter on CPU writes */
	}
	JYASIC_cpuWrite[A](A, V);
}

static void trapPPUAddressChange(uint32_t A) {
	if (((jyasic.irq.control & 0x03) == 0x02) && (lastPPUAddress != A)) {
		int i;
		for (i = 0; i < 2; i++) {
			clockIRQ(); /* Clock IRQ counter on PPU "reads" */
		}
	}
	if (jyasic.mode[3] & 0x80) {
		switch (A & 0x2FF0) {
		case 0x0FD0:
		case 0x0FE0:
			/* If MMC4 jyasic.mode[0] is enabled, and CHR jyasic.mode[0] is
			 * 4 KiB, and tile FD or FE is being fetched ... */
			if ((jyasic.mode[0] & 0x18) == 0x08) {
				/* switch the left or right pattern table's
				 * latch to 0 (FD) or 2 (FE), being used as
				 * an offset for the CHR register index. */
				uint8_t chr = (A >> 4) & (((A >> 10) & 0x04) | 0x02);
				uint8_t bank = (A >> 12) & 0x01;

				if (jyasic.latch[bank] != chr) {
					jyasic.latch[bank] = chr;
					JYASIC_SyncCHR();
				}
			}
			break;
		}
	}
	lastPPUAddress = A;
}

static void ppuScanline(void) {
	if ((jyasic.irq.control & 0x03) == 0x01) {
		int i;
		for (i = 0; i < 8; i++) {
			clockIRQ(); /* Clock IRQ counter on A12 rises (eight per scanline). This should be done in
						   trapPPUAddressChange, but would require more accurate PPU emulation for that. */
		}
	}
}

static void cpuCycle(int a) {
	if ((jyasic.irq.control & 0x03) == 0x00) {
		while (a--) {
			clockIRQ(); /* Clock IRQ counter on M2 cycles */
		}
	}
}

DECLFR(JYASIC_ReadALU_DIP) {
	if ((A & 0x3FF) == 0 && A != 0x5800) { /* 5000, 5400, 5C00: read solder pad setting */
		return dipSwitch | (cpu.openbus & 0x3F);
	}

	if (A & 0x800) {
		switch (A & 3) {
		/* 5800-5FFF: read ALU */
		case 0:
			return (jyasic.mul[0] * jyasic.mul[1]) & 0xFF;
		case 1:
			return (jyasic.mul[0] * jyasic.mul[1]) >> 8;
		case 2:
			return jyasic.adder;
		case 3:
			return jyasic.test;
		}
	}
	/* all others */
	return cpu.openbus;
}

DECLFW(JYASIC_WriteALU) {
	switch (A & 3) {
	case 0:
		jyasic.mul[0] = V;
		break;
	case 1:
		jyasic.mul[1] = V;
		break;
	case 2:
		jyasic.adder += V;
		break;
	case 3:
		jyasic.test = V;
		jyasic.adder = 0;
		break;
	}
}

DECLFW(JYASIC_WritePRG) {
	jyasic.prg[A & 0x03] = V;
	JYASIC_SyncPRG();
	JYASIC_SyncWRAM();
}

DECLFW(JYASIC_WriteCHRLow) {
	jyasic.chr[A & 0x07] = (jyasic.chr[A & 0x07] & 0xFF00) | V;
	JYASIC_SyncCHR();
}

DECLFW(JYASIC_WriteCHRHigh) {
	jyasic.chr[A & 0x07] = (jyasic.chr[A & 0x07] & 0x00FF) | V << 8;
	JYASIC_SyncCHR();
}

DECLFW(JYASIC_WriteNT) {
	if (!(A & 0x04)) {
		jyasic.nt[A & 0x03] = (jyasic.nt[A & 0x03] & 0xFF00) | V;
	} else {
		jyasic.nt[A & 0x03] = (jyasic.nt[A & 0x03] & 0x00FF) | V << 8;
	}
	JYASIC_SyncMirror();
}

DECLFW(JYASIC_WriteIRQ) {
	switch (A & 0x07) {
	case 0:
		jyasic.irq.enable = !!(V & 0x01);
		if (!jyasic.irq.enable) {
			jyasic.irq.prescaler = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	case 1:
		jyasic.irq.control = V;
		break;
	case 2:
		jyasic.irq.enable = FALSE;
		jyasic.irq.prescaler = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 3:
		jyasic.irq.enable = TRUE;
		break;
	case 4:
		jyasic.irq.prescaler = V ^ jyasic.irq.xor ;
		break;
	case 5:
		jyasic.irq.counter = V ^ jyasic.irq.xor ;
		break;
	case 6:
		jyasic.irq.xor = V;
		break;
	}
}

DECLFW(JYASIC_WriteMode) {
	switch (A & 0x03) {
	case 0:
		jyasic.mode[0] = V;
		if (!allow_extended_mirroring) {
			jyasic.mode[0] &= ~0x20;
		}
		break;
	case 1:
		jyasic.mode[1] = V;
		if (!allow_extended_mirroring) {
			jyasic.mode[1] &= ~0x08;
		}
		break;
	case 2:
		jyasic.mode[2] = V;
		break;
	case 3:
		jyasic.mode[3] = V;
		break;
	}
	JYASIC_SyncPRG();
	JYASIC_SyncCHR();
	JYASIC_SyncWRAM();
	JYASIC_SyncMirror();
}

void JYASIC_restoreWriteHandlers(void) {
	int i;
	if (JYASIC_CPUWriteHandlersSet) {
		for (i = 0; i < 0x10000; i++) {
			SetWriteHandler(i, i, JYASIC_cpuWrite[i]);
		}
		JYASIC_CPUWriteHandlersSet = 0;
	}
}

void JYASIC_RegReset(void) {
	memset(&jyasic, 0, sizeof(jyasic));

	jyasic.latch[0] = 0x00;
	jyasic.latch[1] = 0x04;

	JYASIC_SyncPRG();
	JYASIC_SyncCHR();
	JYASIC_SyncWRAM();
	JYASIC_SyncMirror();
}

void JYASIC_Reset(void) {
	dipSwitch = (dipSwitch + 0x40) & 0xC0;
	JYASIC_SyncPRG();
	JYASIC_SyncCHR();
	JYASIC_SyncWRAM();
	JYASIC_SyncMirror();
}

void JYASIC_Power(void) {
	int i;

	SetWriteHandler(0x5000, 0x5FFF, JYASIC_WriteALU);
	SetWriteHandler(0x6000, 0x7fff, CartBW);
	SetWriteHandler(0x8000, 0x87FF, JYASIC_WritePRG);     /* 8800-8FFF ignored */
	SetWriteHandler(0x9000, 0x97FF, JYASIC_WriteCHRLow);  /* 9800-9FFF ignored */
	SetWriteHandler(0xA000, 0xA7FF, JYASIC_WriteCHRHigh); /* A800-AFFF ignored */
	SetWriteHandler(0xB000, 0xB7FF, JYASIC_WriteNT);      /* B800-BFFF ignored */
	SetWriteHandler(0xC000, 0xCFFF, JYASIC_WriteIRQ);
	SetWriteHandler(0xD000, 0xD7FF, JYASIC_WriteMode); /* D800-DFFF ignored */

	JYASIC_restoreWriteHandlers();
	for (i = 0; i < 0x10000; i++) {
		JYASIC_cpuWrite[i] = GetWriteHandler(i);
	}
	SetWriteHandler(0x0000, 0xFFFF, JYASIC_trapCPUWrite); /* Trap all CPU writes for IRQ clocking purposes */
	JYASIC_CPUWriteHandlersSet = 1;

	SetReadHandler(0x5000, 0x5FFF, JYASIC_ReadALU_DIP);
	SetReadHandler(0x6000, 0xFFFF, CartBR);

	JYASIC_RegReset();
}

static void StateRestore(int version) {
	JYASIC_SyncPRG();
	JYASIC_SyncCHR();
	JYASIC_SyncWRAM();
	JYASIC_SyncMirror();
}

void JYASIC_Init(CartInfo *info, int extended_mirr) {
	JYASIC_pwrap = SetPRG_default;
	JYASIC_cwrap = SetCHR_default;
	JYASIC_wwrap = SetWRAM_default;
	JYASIC_mwrap = SetNTMirror_default;

	allow_extended_mirroring = extended_mirr;

	JYASIC_CPUWriteHandlersSet = 0;
	info->Reset = JYASIC_Reset;
	info->Power = JYASIC_Power;

	PPU_hook = trapPPUAddressChange;
	MapIRQHook = cpuCycle;
	GameHBIRQHook2 = ppuScanline;

	AddExState(JYASIC_StateRegs, ~0, 0, 0);
	GameStateRestore = StateRestore;

	/* WRAM is present only in iNES mapper 35, or in mappers with numbers above 255 that require NES 2.0, which
	 * explicitly denotes WRAM size */
	if (info->iNES2) {
		WRAMSIZE = info->PRGRamSize + info->PRGRamSaveSize;
	} else {
		WRAMSIZE = info->mapper == 35 ? 8192 : 0;
	}

	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
	}
}
