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
   Mapper 126: Power Joy version of the mapper, connecting CHR A18 and A19 in reverse order.
   Mapper 534: Waixing version of the mapper, inverting the reload value of the MMC3 scanline counter.
*/

#include "mapinc.h"
#include "mmc3.h"

static uint8 reverseCHR_A18_A19;
static uint8 invertC000;
static uint8 dipSwitch;

static void wrapPRG(uint32 A, uint8 V) {
	int prgAND = (mmc3.expregs[0] & 0x40) ? 0x0F : 0x1F; /* 128 KiB or 256 KiB inner PRG bank selection */
	int prgOR = (((mmc3.expregs[0] << 4) & 0x70) | ((mmc3.expregs[0] << 3) & 0x180)) & ~prgAND; /* outer PRG bank */
	switch (mmc3.expregs[3] & 3) {
		case 0: /* MMC3 PRG mode */
			break;
		case 1:
		case 2: /* NROM-128 mode: MMC3 register 6 applies throughout $8000-$FFFF, MMC3 A13 replaced with CPU A13. */
			V = (mmc3.regs[6] & ~1) | ((A >> 13) & 1);
			setprg8(A ^ 0x4000, (V & prgAND) | prgOR);	/* wrapPRG is only called with A containing the switchable banks, so we need to
														manually switch the normally fixed banks in this mode as well. */
			break;
		case 3: /* NROM-256 mode: MMC3 register 6 applies throughout $8000-$FFFF, MMC3 A13-14 replaced with CPU A13-14. */
			V = (mmc3.regs[6] & ~3) | ((A >> 13) & 3);
			setprg8(A ^ 0x4000, ((V ^ 2) & prgAND) | prgOR);	/* wrapPRG is only called with A containing the switchable banks, so we need to manually
															switch the normally fixed banks in this mode as well. */
			break;
	}
	setprg8(A, (V & prgAND) | prgOR);
}

static void wrapCHR(uint32 A, uint8 V) {
	int chrAND = mmc3.expregs[0] & 0x80 ? 0x7F : 0xFF; /* 128 KiB or 256 KiB innter CHR bank selection */
	int chrOR; /* outer CHR bank */
	if (reverseCHR_A18_A19) /* Mapper 126 swaps CHR A18 and A19 */
		chrOR = ((mmc3.expregs[0] << 4) & 0x080) | ((mmc3.expregs[0] << 3) & 0x100) | ((mmc3.expregs[0] << 5) & 0x200);
	else
		chrOR = (mmc3.expregs[0] << 4) & 0x380;

	if (mmc3.expregs[3] & 0x10) /* CNROM mode: 8 KiB inner CHR bank comes from outer bank register #2 */
		setchr8((mmc3.expregs[2] & (chrAND >> 3)) | ((chrOR & ~chrAND) >> 3));
	else /* MMC3 CHR mode */
		setchr1(A, (V & chrAND) | (chrOR & ~chrAND));
}

static DECLFW(writeWRAM) {
	if (~mmc3.expregs[3] & 0x80) {
		/* Lock bit clear: Update any outer bank register */
		mmc3.expregs[A & 3] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	} else if ((A & 3) == 2) {
		/* Lock bit set: Only update the bottom one or two bits of the CNROM bank */
		int latchMask = (mmc3.expregs[2] & 0x10) ? 1 : 3; /* 16 or 32 KiB inner CHR bank selection */
		mmc3.expregs[2] &= ~latchMask;
		mmc3.expregs[2] |= V & latchMask;
		FixMMC3CHR(mmc3.cmd);
	}
	CartBW(A, V);
}

static DECLFR(readDIP) {
	uint8 result = CartBR(A);
	if (mmc3.expregs[1] & 1)
		result = (result & ~3) | (dipSwitch & 3); /* Replace bottom two bits with solder pad or DIP switch setting if so selected */
	return result;
}

static DECLFW(writeIRQ) {
	MMC3_IRQWrite(A, V ^ 0xFF);
}

static void ResetCommon(void) {
	dipSwitch++; /* Soft-resetting cycles through solder pad or DIP switch settings */
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
	MMC3RegReset();
}

static void PowerCommon(void) {
	dipSwitch = 0;
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, writeWRAM);
	SetReadHandler(0x8000, 0xFFFF, readDIP);
	if (invertC000)
		SetWriteHandler(0xC000, 0xDFFF, writeIRQ); /* Mapper 534 inverts the MMC3 scanline counter reload value */
}

static void InitCommon(CartInfo *info) {
	GenMMC3_Init(info, 8, info->battery);
	mmc3.cwrap = wrapCHR;
	mmc3.pwrap = wrapPRG;

	info->Power = PowerCommon;
	info->Reset = ResetCommon;

	AddExState(mmc3.expregs, 4, 0, "EXPR");
	AddExState(&dipSwitch, 1, 0, "DPSW");
}

void Mapper126_Init(CartInfo *info) {
	reverseCHR_A18_A19 = 1;
	invertC000 = 0;
	InitCommon(info);
}

void Mapper422_Init(CartInfo *info) {
	reverseCHR_A18_A19 = 0;
	invertC000 = 0;
	InitCommon(info);
}

void Mapper534_Init(CartInfo *info) {
	reverseCHR_A18_A19 = 0;
	invertC000 = 1;
	InitCommon(info);
}
