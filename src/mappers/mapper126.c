/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 NewRisingSun
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

/* Mapper 422: "Normal" version of the mapper. Represents UNIF boards BS-400R and BS-4040R.
 * Mapper 126: Power Joy version of the mapper, connecting CHR A18 and A19 in reverse order.
 * Mapper 534: Waixing version of the mapper, inverting the reload value of the MMC3 scanline counter.
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 dipSwitch;

static void M126PW(uint32 A, uint8 V) {
	uint16 mask = (mmc3.expregs[0] & 0x40) ? 0x0F : 0x1F; /* 128 KiB or 256 KiB inner PRG bank selection */
	uint16 base = (((mmc3.expregs[0] << 4) & 0x70) | ((mmc3.expregs[0] << 3) & 0x180)) & ~mask; /* outer PRG bank */

	switch (iNESCart.submapper) {
	case 1:
		/* uses PRG A21 as a chip select between two 1 MiB chips. */
		base |= ((base & 0x100) >> 1);
		break;
	case 2:
		/* uses $6001 bit 2 as a chip select between two 1 MiB chips. */
		base |= ((mmc3.expregs[1] & 0x02) << 5);
		break;
	}

	switch (mmc3.expregs[3] & 0x03) {
	case 0:
		/* MMC3 PRG mode */
		setprg8(A, (base & ~mask) | (V & mask));
		break;
	case 1:
	case 2:
		/* NROM-128 mode: MMC3 register 6 applies throughout $8000-$FFFF, MMC3 A13 replaced with CPU A13. */
		setprg8(0x8000, (base | ((mmc3.regs[6] & ~1) & mask)) | 0);
		setprg8(0xA000, (base | ((mmc3.regs[6] & ~1) & mask)) | 1);
		setprg8(0xC000, (base | ((mmc3.regs[6] & ~1) & mask)) | 0);
		setprg8(0xE000, (base | ((mmc3.regs[6] & ~1) & mask)) | 1);
		break;
	case 3:
		/* NROM-256 mode: MMC3 register 6 applies throughout $8000-$FFFF, MMC3 A13-14 replaced with CPU A13-14. */
		setprg8(0x8000, (base | ((mmc3.regs[6] & ~3) & mask)) | 0);
		setprg8(0xA000, (base | ((mmc3.regs[6] & ~3) & mask)) | 1);
		setprg8(0xC000, (base | ((mmc3.regs[6] & ~3) & mask)) | 2);
		setprg8(0xE000, (base | ((mmc3.regs[6] & ~3) & mask)) | 3);
		break;
	}
}

static void M126CW(uint32 A, uint8 V) {
	uint16 mask = (mmc3.expregs[0] & 0x80) ? 0x7F : 0xFF; /* 128 KiB or 256 KiB innter CHR bank selection */
	uint16 base; /* outer CHR bank */

	if (iNESCart.mapper == 126) {
		/* Mapper 126 swaps CHR A18 and A19 */
		base = ((mmc3.expregs[0] << 4) & 0x080) | ((mmc3.expregs[0] << 3) & 0x100) | ((mmc3.expregs[0] << 5) & 0x200);
	} else {
		base = (mmc3.expregs[0] << 4) & 0x380;
	}

	if (mmc3.expregs[3] & 0x10) {
		/* CNROM mode: 8 KiB inner CHR bank comes from outer bank register #2 */
		setchr8((mmc3.expregs[2] & (mask >> 3)) | ((base & ~mask) >> 3));
	} else {
		/* MMC3 CHR mode */
		setchr1(A, (V & mask) | (base & ~mask));
	}
}

static DECLFW(M126WRAMWrite) {
	if (~mmc3.expregs[3] & 0x80) {
		/* Lock bit clear: Update any outer bank register */
		mmc3.expregs[A & 0x03] = V;
		MMC3_FixPRG();
		MMC3_FixCHR();
	} else if ((A & 0x03) == 2) {
		/* Lock bit set: Only update the bottom one or two bits of the CNROM bank */
		uint8 mask = (mmc3.expregs[2] & 0x10) ? 1 : 3; /* 16 or 32 KiB inner CHR bank selection */
		mmc3.expregs[2] &= ~mask;
		mmc3.expregs[2] |= V & mask;
		MMC3_FixCHR();
	}
	CartBW(A, V);
}

static DECLFR(M126ReadDIP) {
	uint8 result = CartBR(A);
	if (mmc3.expregs[1] & 0x01) {
		/* Replace bottom two bits with solder pad or DIP switch setting if so selected */
		result = (result & ~0x03) | (dipSwitch & 0x03);
	}
	return result;
}

static DECLFW(M126IRQWrite) {
	if (iNESCart.mapper == 534) {
		/* Mapper 534 takes the inverted $C000 value for the scanline counter */
		V ^= 0xFF;
	}
	MMC3_IRQWrite(A, V);
}

static void M126Reset(void) {
	dipSwitch++; /* Soft-resetting cycles through solder pad or DIP switch settings */
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
	MMC3RegReset();
}

static void M126Power(void) {
	dipSwitch = 0;
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, M126WRAMWrite);
	SetReadHandler(0x8000, 0xFFFF, M126ReadDIP);
	SetWriteHandler(0xC000, 0xDFFF, M126IRQWrite);
}

void Mapper126_Init(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	MMC3_cwrap = M126CW;
	MMC3_pwrap = M126PW;
	info->Power = M126Power;
	info->Reset = M126Reset;
	AddExState(mmc3.expregs, 4, 0, "EXPR");
	AddExState(&dipSwitch, 1, 0, "DPSW");
}

void Mapper422_Init(CartInfo *info) {
	Mapper126_Init(info);
}

void Mapper534_Init(CartInfo *info) {
	Mapper126_Init(info);
}
