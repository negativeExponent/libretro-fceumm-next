/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* Chipset used on various PCBs named WX-KB4K, T4A54A, BS-5652... */
/* "Rockman 3" on YH-322 and "King of Fighters 97" on "Super 6-in-1" enable interrupts without initializing the frame IRQ register and therefore freeze on real hardware.
   They can run if another game is selected that does initialize the frame IRQ register, then soft-resetting to the menu and selecting the previously-freezing games. */

#include "mapinc.h"
#include "mmc3.h"

static uint8 dip;

static void Mapper134_PRGWrap(uint32 A, uint8 V) {
	int prgAND = (mmc3.expregs[1] & 0x04) ? 0x0F : 0x1F;
	int prgOR = ((mmc3.expregs[1] << 4) & 0x30) | ((mmc3.expregs[0] << 2) & 0x40);
	if (mmc3.expregs[1] & 0x80) { /* NROM mode */
		if (mmc3.expregs[1] & 0x08) { /* NROM-128 mode */
			setprg8(0x8000, (((mmc3.regs[6] & ~1) | 0) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xA000, (((mmc3.regs[6] & ~1) | 1) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xC000, (((mmc3.regs[6] & ~1) | 0) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xE000, (((mmc3.regs[6] & ~1) | 1) & prgAND) | (prgOR & ~prgAND));
		} else { /* NROM-256 mode */
			setprg8(0x8000, (((mmc3.regs[6] & ~3) | 0) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xA000, (((mmc3.regs[6] & ~3) | 1) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xC000, (((mmc3.regs[6] & ~3) | 2) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xE000, (((mmc3.regs[6] & ~3) | 3) & prgAND) | (prgOR & ~prgAND));
		}
	} else
		setprg8(A, (V & prgAND) | (prgOR & ~prgAND));
}

static void Mapper134_CHRWrap(uint32 A, uint8 V) {
	int chrAND = (mmc3.expregs[1] & 0x40) ? 0x7F : 0xFF;
	int chrOR = ((mmc3.expregs[1] << 3) & 0x180) | ((mmc3.expregs[0] << 4) & 0x200);
	if (mmc3.expregs[0] & 0x08)
		V = (mmc3.expregs[2] << 3) | ((A >> 10) & 7); /* In CNROM mode, outer bank register 2 replaces the MMC3's CHR registers,
		                                      and CHR A10-A12 are PPU A10-A12. */
	setchr1(A, (V & chrAND) | (chrOR & ~chrAND));
}

static DECLFR(Mapper134_Read) {
	return (mmc3.expregs[0] & 0x40) ? dip : CartBR(A);
}

static DECLFW(Mapper134_Write) {
	if (~mmc3.expregs[0] & 0x80) {
		mmc3.expregs[A & 3] = V;
		FixMMC3PRG(mmc3.cmd);
		FixMMC3CHR(mmc3.cmd);
	} else if ((A & 3) == 2) {
		mmc3.expregs[A & 3] = (mmc3.expregs[A & 3] & ~3) | (V & 3);
		FixMMC3CHR(mmc3.cmd);
	}
	CartBW(A, V);
}

static void Mapper134_Reset(void) {
	dip++;
	dip &= 15;
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
	MMC3RegReset();
}

static void Mapper134_Power(void) {
	dip = 0;
	mmc3.expregs[0] = mmc3.expregs[1] = mmc3.expregs[2] = mmc3.expregs[3] = 0;
	GenMMC3Power();
	SetWriteHandler(0x6000, 0x7FFF, Mapper134_Write);
	SetReadHandler(0x8000, 0xFFFF, Mapper134_Read);
}

void Mapper134_Init(CartInfo *info) {
	GenMMC3_Init(info, 256, 256, info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) / 1024 : 8, info->battery);
	mmc3.cwrap = Mapper134_CHRWrap;
	mmc3.pwrap = Mapper134_PRGWrap;
	info->Power = Mapper134_Power;
	info->Reset = Mapper134_Reset;
	AddExState(mmc3.expregs, 4, 0, "EXPR");
	AddExState(&dip, 1, 0, "DIPS");
}
