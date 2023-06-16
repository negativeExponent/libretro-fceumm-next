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
 *
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static void M432CW(uint32 A, uint8 V) {
	int chrAND = (mmc3.expregs[1] & 0x04) ? 0x7F : 0xFF;
	int chrOR  = ((mmc3.expregs[1] << 7) & 0x080) | ((mmc3.expregs[1] << 5) & 0x100) | ((mmc3.expregs[1] << 4) & 0x200);
	setchr1(A, (V & chrAND) | (chrOR & ~chrAND));
}

static void M432PW(uint32 A, uint8 V) {
	int prgAND = (mmc3.expregs[1] & 0x02) ? 0x0F : 0x1F;
	int prgOR  = ((mmc3.expregs[1] << 4) & 0x10) | ((mmc3.expregs[1] << 1) & 0x60);
	if (mmc3.expregs[1] & 0x40) { /* NROM */
		if (mmc3.expregs[1] & 0x80) { /* NROM-256 */
			setprg8(0x8000, ((mmc3.regs[6] & ~2) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xA000, ((mmc3.regs[7] & ~2) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xC000, ((mmc3.regs[6] |  2) & prgAND) | (prgOR & ~prgAND));
			setprg8(0xE000, ((mmc3.regs[7] |  2) & prgAND) | (prgOR & ~prgAND));
		} else { /* NROM-128 */
			setprg8(0x8000, (mmc3.regs[6] & prgAND) | (prgOR & ~prgAND));
			setprg8(0xA000, (mmc3.regs[7] & prgAND) | (prgOR & ~prgAND));
			setprg8(0xC000, (mmc3.regs[6] & prgAND) | (prgOR & ~prgAND));
			setprg8(0xE000, (mmc3.regs[7] & prgAND) | (prgOR & ~prgAND));
		}
	} else { /* MMC3 */
		setprg8(A, (V & prgAND) | (prgOR & ~prgAND));
	}
}

static DECLFR(M432Read) {
	if (mmc3.expregs[0] & 1 || ((mmc3.expregs[1] & 0x20) && (ROM_size < 64)))
		return mmc3.expregs[2];
	return CartBR(A);
}

static DECLFW(M432Write) {
	if (MMC3CanWriteToWRAM()) {
		mmc3.expregs[A & 1] = V;
		if (~A & 1 && ~V & 1 && ROM_size < 64)
			mmc3.expregs[1] &= ~0x20; /* Writing 0 to register 0 clears register 1's DIP bit */
		FixMMC3PRG();
		FixMMC3CHR();
	}
}

static void M432Reset(void) {
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] = 0;
	mmc3.expregs[2]++;
	MMC3RegReset();
}

static void M432Power(void) {
	mmc3.expregs[0] = 0;
	mmc3.expregs[1] = 0;
	mmc3.expregs[2] = 0;
	GenMMC3Power();
	SetReadHandler(0x8000, 0xFFFF, M432Read);
	SetWriteHandler(0x6000, 0x7FFF, M432Write);
}

void Mapper432_Init(CartInfo *info) {
	GenMMC3_Init(info, 0, 0);
	mmc3.cwrap       = M432CW;
	mmc3.pwrap       = M432PW;
	info->Power = M432Power;
	info->Reset = M432Reset;
	AddExState(mmc3.expregs, 3, 0, "EXPR");
}
