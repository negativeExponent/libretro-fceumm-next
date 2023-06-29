/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 Libretro Team
 *  Copyright (C) 2023
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

/* NES 2.0 Mapper 350 - BMC-891227
 * Super 15-in-1 Game Card 
 * https://wiki.nesdev.com/w/index.php/NES_2.0_Mapper_350
 */

#include "mapinc.h"
#include "latch.h"

static uint16 bank;

static SFORMAT StateRegs[] = {
	{ &bank, 2, "REG0" },
	{ 0 }
};

static void Sync(void) {
    if ((latch.addr & 0xC000) == 0xC000) {
        bank = (bank & ~0x07) | (latch.data & 0x07);
    } else {
        bank = (latch.data & ~0x07) | (bank & 0x07);
    }

	setprg8(0x6000, 1);
	switch ((bank >> 5) & 3) {
	case 0: /* NROM-128 */
		setprg16(0x8000, bank & 0x1F);
		setprg16(0xC000, bank & 0x1F);
		break;
	case 1: /* NROM-256 */
		setprg32(0x8000, (bank & 0x1F) >> 1);
		break;
	case 2:
	case 3: /* UNROM */
        if (bank & 0x20) {
            /* 2nd chip */
            setprg16(0x8000, 0x20 | (bank & 0x07));
		    setprg16(0xC000, 0x20 | 7);
        } else {
            setprg16(0x8000, bank & 0x1F);
		    setprg16(0xC000, (bank & 0x1F) | 7);
        }
		break;
	}

	/* CHR-RAM Protect... kinda */
	SetupCartCHRMapping(0, CHRptr[0], 0x2000, (bank & 0x40) != 0);
	setchr8(0);
	setmirror(((bank >> 7) & 1) ^ 1);
}

static void M350Reset(void) {
    bank = 0;
    LatchHardReset();
}

void Mapper350_Init(CartInfo *info) {
    Latch_Init(info, Sync, NULL, 1, 0);
    info->Reset = M350Reset;
	AddExState(StateRegs, ~0, 0, NULL);
}
