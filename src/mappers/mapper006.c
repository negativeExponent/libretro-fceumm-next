/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 * Front Fareast Magic Card 1M or 2M RAM cartridges
 *
 */

#include "mapinc.h"
#include "mapper006.h"

static struct {
	uint8_t mc1mode;
	uint8_t mc2mode;
	uint8_t smcmode;

	uint8_t prg8K[4];
	uint8_t chr1K[8];
	uint8_t nt[4];
	uint8_t latchMMC4[2];
	uint8_t latch;
	uint8_t chrlock;

	uint8_t smc_IRQa;
	uint32_t smc_IRQCount;

	uint8_t fds_control;
	int16_t fds_IRQCount;

	uint8_t scratchRAM[0x2000];
} m006;

static writefunc writePPU2007;

static SFORMAT StateRegs[] = {
	{ &m006.mc1mode, 1, "MC1M" },
	{ &m006.mc2mode, 1, "MC2M" },
	{ &m006.smcmode, 1, "SMCM" },

	{ m006.prg8K, 4, "PRG8" },
	{ m006.chr1K, 8, "CHR1" },
	{ m006.nt, 4, "NTAR" },
	{ m006.latchMMC4, 2, "MMC4" },
	{ &m006.latch, 1, "LATC" },
	{ &m006.chrlock, 1, "CHRL" },

	{ &m006.smc_IRQa, 1, "IRQA" },
	{ &m006.smc_IRQCount, sizeof(m006.smc_IRQCount) | FCEUSTATE_RLSB, "IRQC" },

	{ &m006.fds_control, 1, "FDSI" },
	{ &m006.fds_IRQCount, sizeof(m006.fds_IRQCount) | FCEUSTATE_RLSB, "FDSC" },

	{ 0 }
};

static void SyncPRG(void) {
	uint8_t prg_writable = !(m006.mc1mode & 0x02);

	SetupCartPRGMapping(0, PRGptr[0], PRGsize[0], prg_writable);

	if (!(m006.mc2mode & 0x01)) {
		setprg8(0x8000, m006.prg8K[0]);
		setprg8(0xA000, m006.prg8K[1]);
		setprg8(0xC000, m006.prg8K[2]);
		setprg8(0xE000, m006.prg8K[3]);
	} else {
		switch (m006.mc1mode >> 5) {
		case 0:
			setprg16(0x8000, m006.latch & 0x07);
			setprg16(0xC000, 0x07);
			break;
		case 1:
			setprg16(0x8000, (m006.latch >> 2) & 0x0F);
			setprg16(0xC000, 0x07);
			break;
		case 2:
			setprg16(0x8000, m006.latch & 0x0F);
			setprg16(0xC000, 0x0F);
			break;
		case 3:
			setprg16(0x8000, 0x0F);
			setprg16(0xC000, m006.latch & 0x0F);
			break;
		case 4:
			setprg32(0x8000, (m006.latch >> 4) & 0x03);
			break;
		case 5:
		case 6:
		case 7:
			setprg32(0x8000, 0x03);
			break;
		}
	}
}

static void SyncCHR(void) {
	uint8_t chr_writable = !(((m006.mc1mode & 0xE1) >= 0x81) || m006.chrlock);

	SetupCartCHRMapping(0, CHRptr[0], CHRsize[0], chr_writable);

	if (m006.smcmode & 0x01) {
		if (m006.smcmode & 0x04) {
			setchr1(0x0000, m006.chr1K[0]);
			setchr1(0x0400, m006.chr1K[1]);
			setchr1(0x0800, m006.chr1K[2]);
			setchr1(0x0C00, m006.chr1K[3]);
			setchr1(0x1000, m006.chr1K[4]);
			setchr1(0x1400, m006.chr1K[5]);
			setchr1(0x1800, m006.chr1K[6]);
			setchr1(0x1C00, m006.chr1K[7]);
		} else {
			setchr4(0x0000, m006.chr1K[0 | m006.latchMMC4[0]] >> 2);
			setchr4(0x1000, m006.chr1K[4 | m006.latchMMC4[1]] >> 2);
		}
	} else {
		switch (m006.mc1mode >> 5) {
		case 0:
		case 2:
			setchr8(0);
			break;
		case 1:
		case 4:
		case 5:
			setchr8(m006.latch & 0x03);
			break;
		case 3:
			setchr8((m006.latch >> 4) & 0x03);
			break;
		case 6:
			setchr8(m006.latch & 0x01);
			break;
		case 7:
			setchr8(0x03);
			break;
		}
	}
}

static void SyncMirror(void) {
	if (m006.smcmode & 0x02) {
		switch (m006.mc1mode & 0x11) {
		case 0x00:
			setmirror(MI_0);
			break;
		case 0x10:
			setmirror(MI_1);
			break;
		case 0x01:
			setmirror(MI_V);
			break;
		case 0x11:
			setmirror(MI_H);
			break;
		}
	} else {
		setntamem(CHRptr[0] + 0x400 * (m006.nt[0] & CHRmask1[0]), 1, 0);
		setntamem(CHRptr[0] + 0x400 * (m006.nt[1] & CHRmask1[0]), 1, 1);
		setntamem(CHRptr[0] + 0x400 * (m006.nt[2] & CHRmask1[0]), 1, 2);
		setntamem(CHRptr[0] + 0x400 * (m006.nt[3] & CHRmask1[0]), 1, 3);
	}
}

static void SyncWRAM(void) {
	setprg4r(0x11, 0x5000, 0); /* m006.scratchRAM ram */
	setprg8r(0x10, 0x6000, 0); /* wram */
}

extern uint32_t RefreshAddr;
static DECLFW(WritePPU2007) {
	if (!(RefreshAddr & 0x2000)) {
		if ((m006.mc1mode >= 0xA0) && !(m006.mc1mode & 0x01)) {
			m006.chrlock = !!(m006.mc1mode & 0x10);
		}
	}
	writePPU2007(A, V);
}

static DECLFR(ReadReg) {
	switch (A) {
	case 0x4500:
		return m006.smcmode;
	default:
		break;
	}
	return cpu.openbus;
}

static DECLFW(WriteReg) {
	switch (A) {
	case 0x4024:
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0x4025:
		X6502_IRQEnd(FCEU_IQEXT);
		if (!m006.smc_IRQa) {
			m006.fds_control = V;
			if (V & 0x42) {
				m006.fds_IRQCount = 0;
			}
		}
		break;
	case 0x42FC:
	case 0x42FD:
	case 0x42FE:
	case 0x42FF:
		m006.mc1mode = (V & 0xF0) | (A & 0x03);
		if (m006.mc1mode >= 0x80) {
			m006.chrlock = FALSE;
		}
		SyncPRG();
		SyncCHR();
		SyncMirror();
		break;
	case 0x43FC:
	case 0x43FD:
	case 0x43FE:
	case 0x43FF:
		m006.mc2mode = (V & 0xF0) | (A & 0x03);
		m006.latch = V;
		SyncPRG();
		SyncCHR();
		break;
	case 0x4500:
		m006.smcmode = V;
		SyncCHR();
		SyncMirror();
		break;
	case 0x4501:
		m006.smc_IRQa = FALSE;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0x4502:
		m006.smc_IRQCount = (m006.smc_IRQCount & 0xFF00) | V;
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0x4503:
		m006.smc_IRQa = TRUE;
		m006.smc_IRQCount = (m006.smc_IRQCount & 0x00FF) | (V << 8);
		X6502_IRQEnd(FCEU_IQEXT);
		break;
	case 0x4504:
	case 0x4505:
	case 0x4506:
	case 0x4507:
		if (m006.mc2mode & 0x02) {
			V >>= 2;
		}
		m006.prg8K[A & 0x03] = V;
		SyncPRG();
		break;
	case 0x4510:
	case 0x4511:
	case 0x4512:
	case 0x4513:
	case 0x4514:
	case 0x4515:
	case 0x4516:
	case 0x4517:
		m006.chr1K[A & 0x07] = V;
		SyncCHR();
		break;
	case 0x4518:
	case 0x4519:
	case 0x451A:
	case 0x451B:
		m006.nt[A & 0x03] = V;
		SyncMirror();
		break;
	}
	if ((A >= 0x4500) && (A <= 0x451F)) {
		m006.scratchRAM[A - 0x4000] = V;
	}
}

static DECLFW(WriteLatch) {
	if (m006.mc1mode & 0x02) {
		m006.latch = V;
		if (m006.mc2mode & 0x03) {
			m006.prg8K[(A >> 13) & 0x03] = V >> 2;
		}
		SyncPRG();
		SyncCHR();
	} else {
		CartBW(A, V);
	}
}

static void ClockFDSCounter (int a) {
	m006.fds_IRQCount += 3 * a;
	while ((m006.fds_IRQCount >= 448) && (m006.fds_control & 0x80)) {
		X6502_IRQBegin(FCEU_IQEXT);
		m006.fds_IRQCount -= 448;
	}
}

static void ClockSMCCounter(int a) {
	if (m006.smc_IRQa) {
		m006.smc_IRQCount += a;
		if (m006.smc_IRQCount >= 0x10000) {
			m006.smc_IRQCount = 0;
			m006.smc_IRQa = FALSE;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}
}

static void CPUIRQHook(int a) {
	ClockFDSCounter(a);
	if (!(m006.smcmode & 0x08)) {
		ClockSMCCounter(a);
	}
}

static void HBIRQHook(void) {
	if (m006.smcmode & 0x08) {
		ClockSMCCounter(8);
	}
	
}

static void PPUIRQHook(uint32_t A) {
	if (!(A & 0x2000) && (m006.smcmode & 0x01) && !(m006.smcmode & 0x04)) {
		uint8_t value = (A >> 4) & 0x02;
		uint8_t bank = (A >> 12) & 0x01;

		switch (A & 0x0FF0) {
		case 0x0FD0:
		case 0x0FE0:
			if (m006.latchMMC4[bank] != value) {
				m006.latchMMC4[bank] = value;
				SyncCHR();
			}
			break;
		default:
			break;
		}
	}
}

static void SetTrainer(void) {
#define PRGPAGE_DMR(a)	  Page[(a) >> 11][(a)]
#define PRGPAGE_DMW(a, d) Page[(a) >> 11][(a)] = (d)
	int i, nmiHandler;

	nmiHandler = PRGPAGE_DMR(0xFFFA) | (PRGPAGE_DMR(0xFFFB) << 8);
	if (nmiHandler == 0x5032) {
		PRGPAGE_DMW(0xFFFA, m006.scratchRAM[0x4F]);
		PRGPAGE_DMW(0xFFFB, m006.scratchRAM[0x50]);
	}
	for (i = 0; i < 4096; i++) {
		m006.scratchRAM[i] = smc5000[i];
	}
	m006.scratchRAM[0x4F] = PRGPAGE_DMR(0xFFFA);
	m006.scratchRAM[0x50] = PRGPAGE_DMR(0xFFFB);
	if (iNESCart.mapper == 17) {
		PRGPAGE_DMW(0xFFFA, 0x32);
		PRGPAGE_DMW(0xFFFB, 0x50);
	}
	if (iNESCart.trainer && WRAM) {
		uint8_t *trainerData = 0;
		uint32_t trainerSize = ROM.misc.size;
		uint16_t trainerAddr = 0x7000;

		if (iNESCart.mapper == 17) {
			if (iNESCart.submapper == 0) {
				trainerAddr = 0x7000;
			} else {
				trainerAddr = ((iNESCart.submapper << 8) & 0x0300) | 0x5C00;
			}
		}
		if (trainerAddr < 0x6000) {
			trainerData = &m006.scratchRAM[(trainerAddr & 0xF00)];
		} else {
			trainerData = &WRAM[trainerAddr & 0x1F00];
		}
		for (i = 0; i < (int)ROM.misc.size; i++) {
			trainerData[i] = ROM.misc.data[i];
		}
		FCEU_printf(" load addr : %04x\n", trainerAddr);
		FCEU_printf(" data size : %d\n", trainerSize);
		FCEU_printf(" reset     : %04x\n", (iNESCart.mapper == 17) ? trainerAddr : 0x5000);
		X6502_SetNewPC((iNESCart.mapper == 17) ? trainerAddr : 0x5000);
	}
	(GetWriteHandler(0x4017))(0x4017, 0x40);

#undef PRGPAGE_DMR
#undef PRGPAGE_DMW
}

static void Reset(void) {
	m006.smc_IRQa = FALSE;
	m006.smc_IRQCount = 0;
	m006.fds_control = 0;
	m006.fds_IRQCount = 0;

	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

static void Power(void) {
	m006.mc1mode = (((iNESCart.mapper == 6) ? iNESCart.submapper : 1) << 5) |
	               ((iNESCart.mirror == MI_V) ? 0x01 : 0x11) | 0x02;
	m006.mc2mode = ((iNESCart.mapper == 12) || (iNESCart.mapper == 17)) ? 0x00 : 0x03;
	m006.smcmode = (iNESCart.mapper == 17) ? 0x47 : 0x42;

	m006.latch = 0;
	m006.chrlock = FALSE;

	m006.prg8K[0] = PRG_BANK_COUNT(8) - 4;
	m006.prg8K[1] = PRG_BANK_COUNT(8) - 3;
	m006.prg8K[2] = PRG_BANK_COUNT(8) - 2;
	m006.prg8K[3] = PRG_BANK_COUNT(8) - 1;

	m006.chr1K[0] = 0;
	m006.chr1K[1] = 1;
	m006.chr1K[2] = 2;
	m006.chr1K[3] = 3;
	m006.chr1K[4] = 4;
	m006.chr1K[5] = 5;
	m006.chr1K[6] = 6;
	m006.chr1K[7] = 7;

	Reset();

	writePPU2007 = GetWriteHandler(0x2007);
	SetWriteHandler(0x2007, 0x2007, WritePPU2007);

	SetReadHandler(0x4020, 0x47FF, ReadReg);
	SetWriteHandler(0x4020, 0x47FF, WriteReg);

	SetReadHandler(0x5000, 0x7FFF, CartBR);
	SetWriteHandler(0x5000, 0x7FFF, CartBW);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);

	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, WriteLatch);

	SetTrainer();
}

static void StateRestore(int version) {
	SyncPRG();
	SyncCHR();
	SyncMirror();
	SyncWRAM();
}

void Mapper006_Init(CartInfo *info) {
	uint32_t wramsize = info->PRGRamSize + info->PRGRamSaveSize;
	uint32_t prgsize = ROM.prg.size;

	info->Power = Power;
	info->Reset = Reset;

	MapIRQHook = CPUIRQHook;
	PPU_hook = PPUIRQHook;
	GameHBIRQHook = HBIRQHook;

	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, NULL);

	WRAMSIZE = wramsize ? wramsize : 8192;
	WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	SetupCartPRGMapping(0x11, m006.scratchRAM, 4096, TRUE);
	AddExState(m006.scratchRAM, sizeof(m006.scratchRAM), 0, "SRAM");

	if (ROM.chr.size) {
		if ((info->mapper == 12) && (info->submapper == 1)) {
			uint8_t *newbuffer;
			size_t prg_size = (ROM.prg.size > SIZE_256K) ? SIZE_256K : ROM.prg.size;
			size_t chr_size = (ROM.chr.size > SIZE_256K) ? SIZE_256K : ROM.chr.size;

			newbuffer = (uint8_t *)FCEU_malloc(SIZE_512K);
			memset(newbuffer, 0xFF, SIZE_512K);
			/* copy main PRG-ROM data */
			memcpy(newbuffer, ROM.prg.data, prg_size);
			/* Append CHR-ROM data at offset 0x40000 (256KB) */
			memcpy(newbuffer + SIZE_256K, ROM.chr.data, chr_size);

			FCEU_free(ROM.prg.data);

			/* setup and map new prg data */
			ROM.prg.data = newbuffer;
			SetupCartPRGMapping(0, ROM.prg.data, SIZE_512K, FALSE);

			/* setup and map chr ram */
			CHRRAMSIZE = info->CHRRamSize;
			CHRRAM = (uint8_t *)FCEU_malloc(CHRRAMSIZE);
			SetupCartCHRMapping(0, CHRRAM, CHRRAMSIZE, TRUE);
			AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");

			prgsize = SIZE_512K;
		} else {
			/* need data in CHR-RAM, not CHR-ROM */
			SetupCartCHRMapping(0, ROM.chr.data, ROM.chr.size, TRUE);
			AddExState(ROM.chr.data, ROM.chr.size, 0, "CRAM");
		}
	}

	if ((iNESCart.iNES2 == 0) && (iNESCart.mapper == 6)) {
		iNESCart.submapper = 1;
	}

	if (iNESCart.mapper == 8) {
		iNESCart.mapper = 6;
		iNESCart.submapper = 4;
	}

	/* PRG memory can be writable, so add to states */
	AddExState(ROM.prg.data, prgsize, 0, "PRAM");
}
