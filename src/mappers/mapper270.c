/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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
 */

/* NES 2.0 Mapper 270 denotes OneBus console multicarts that use the consoles'
 * universal input/output (UIO) register $412C to bankswitch higher-order PRG
 * address lines or several PRG chips, and select CHR-RAM via $4242.*/

/* TODO: Family Pocket 638-in-1 freeze on jumper enable */

#include "mapinc.h"
#include "onebus.h"

static struct {
	uint8_t reg; /* $4242 */
	uint8_t dipsw; /* jumper */
} m270;

static SFORMAT StateRegs[] = {
	{ &m270.reg, 1, "EXPR" },
	{ &m270.dipsw, 1, "DPSW" },
	{ 0 }
};

static void Sync(void) {
	uint16_t mblock = 0;
	switch (iNESCart.submapper) {
	case 1:
		mblock |= (onebus.cpu41xx[0x2C] & 0x02) << 10; /* PRG/CHR A24 */
		break;
	case 2:
		mblock |= (onebus.cpu41xx[0x2C] & 0x02) << 10; /* PRG/CHR A24 */
		mblock |= (onebus.cpu41xx[0x2C] & 0x01) << 12; /* PRG/CHR A25 */
		break;
	case 3:
		mblock |= (onebus.cpu41xx[0x2C] & 0x04) << 9; /* PRG/CHR A24 */
		break;
	case 0:
	default:
		mblock |= (onebus.cpu41xx[0x2C] & 0x06) ? 0x800 : 0; /* PRG/CHR A24 */
		mblock |= (onebus.cpu41xx[0x2C] & 0x01) << 12; /* PRG/CHR A25 */
		break;
	}
	OneBus_SyncPRG(0x07FF, mblock);
	if (m270.reg & 0x01) {
		/* CHR-RAM enabled, use 8K unbancked CHR RAM */
		SetupCartCHRMapping(0, CHRRAM, CHRRAMSIZE, TRUE);
		setchr8(0);
	} else {
		OneBus_SyncCHR(0x3FFF, mblock << 3);
	}
	OneBus_SyncMirror();
}

static DECLFR(ReadDIP) {
	return m270.dipsw << 3;
}

static DECLFW(WriteCHREnable) {
	m270.reg = V;
	Sync();
}

static void Power(void) {
	m270.dipsw = 0;
	m270.reg = 0;
	OneBus_Power();
	SetReadHandler(0x412C, 0x412C, ReadDIP);
	SetWriteHandler(0x4242, 0x4242, WriteCHREnable);
}

static void Reset(void) {
	m270.dipsw = !m270.dipsw; /* toggle jumper */
	m270.reg = 0;
	onebus.cpu41xx[0x2C] = 0;
	OneBus_Reset();
}

void Mapper270_Init(CartInfo *info) {
	int ws = (info->PRGRamSize + info->PRGRamSaveSize) / 1024;

	if (!info->iNES2) {
		ws = 8;
	}

	OneBus_Init(info, Sync, ws, info->battery);
	info->Power = Power;
	info->Reset = Reset;

	AddExState(StateRegs, ~0, 0, NULL);
}
