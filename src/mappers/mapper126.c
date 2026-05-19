/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020
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

/* Mapper 422: "Normal" version of the mapper. Represents UNIF boards BS-400R and BS-4040R.
 * Mapper 126: Power Joy version of the mapper, connecting CHR A18 and A19 in reverse order.
 * Mapper 534: Waixing version of the mapper, inverting the reload V of the MMC3 scanline counter.
 */

#include "mapinc.h"
#include "mmc3.h"

static struct {
	uint8_t reg[4];
} m126;

static uint8_t dipsw;
static uint8_t oldump = FALSE;

static SFORMAT StateRegs[] = {
	{ m126.reg, 4, "EXPR" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	uint8_t reg0 = m126.reg[0] ^ (oldump ? 0 : 0x20);
	uint16_t mask = (reg0 & 0x40) ? 0x0F : 0x1F;
	uint16_t base = (((reg0 << 4) & 0x70) | ((reg0 << 3) & 0x180)) & ~mask;

	switch (iNESCart.submapper) {
	case 1:
		base = ((base >> 1) & 0x80) | (base & 0x7F);
		break;
	case 2:
		base = ((m126.reg[1] << 5) & 0x80) | (base & 0x7F);
		break;
	}

	if (m126.reg[3] & 0x08) {
		uint8_t b = (A >> 13) & 0x03;
		V = MMC3_GetPRGBank(b & (((m126.reg[3] & 0x0D) == 0x0D) ? 0x02 : ((m126.reg[3] & 0x01) ? 0 : 0x03)));
		switch (m126.reg[3] & 0x03) {
		case 0:
			V = ((V << 1) & ~0x03) | (V & 0x03);
			break;
		case 1:
			V = ((V << 1) & ~0x01) | (b & 0x03);
			break;
		case 2:
			V = ((V << 2) & ~0x03) | (V & 0x03);
			break;
		case 3:
			V = ((V << 2) & ~0x03) | (b & 0x03);
			break;
		}
	} else {
		switch (m126.reg[3] & 0x03) {
		case 1:
		case 2:
			base = base | (mmc3.reg[6] & mask);
			V = (A >> 13) & 0x01;
			mask = 0x01;
			break;
		case 3:
			base = base | (mmc3.reg[6] & mask);
			V = (A >> 13) & 0x03;
			mask = 0x03;
			break;
		}
	}
	setprg8(A, (base & ~mask) | (V & mask));
}

static void SetCHR(uint16_t A, uint16_t V) {
	uint8_t reg0 = m126.reg[0] ^ (oldump ? 0 : 0x20);
	uint16_t mask = (reg0 & 0x80) ? 0x7F : 0xFF;
	uint16_t base = (reg0 << 4) & 0x380;

	if (iNESCart.mapper == 126) {
		base = ((reg0 << 4) & 0x080) | ((reg0 << 3) & 0x100) | ((reg0 << 5) & 0x200);
	}
	if (m126.reg[3] & 0x10) {
		base = (((base & ~mask) >> 3) | (m126.reg[2] & (mask >> 3))) << 3;
		V = (A >> 10) & 0x07;
		mask = 0x07;
	}
	setchr1(A, (base & ~mask) | (V & mask));
}

static void SyncMirror(void) {
	if (m126.reg[3] & 0x20) {
		setmirror(MI_0 + ((mmc3.reg[6] & 0x10) >> 4));
	} else if (m126.reg[1] & 0x02) {
		switch (mmc3.mirr & 0x03) {
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
	} else {
		setmirror((mmc3.mirr & 0x01) ^ 0x01);
	}
}

static DECLFR(ReadDIP) {
	if (m126.reg[1] & 0x01) {
		return CartBR((A & ~0x01) | (dipsw & 0x01));
	}
	return CartBR(A);
}

static DECLFW(WriteReg) {
	CartBW(A, V);
	if ((A & 0x03) == 0x02) {
		const uint8_t mask = 0xFF & (~((m126.reg[2] & 0x80) ? 0xF0 : 0x00)) & (~(((m126.reg[2]) >> 3) & 0x0E));
		m126.reg[2] = (m126.reg[2] & ~mask) | (V & mask);
		MMC3_SyncCHR();
	} else {
		if (!(m126.reg[3] & 0x80)) {
			m126.reg[A & 0x03] = V;
			MMC3_SyncPRG();
			MMC3_SyncCHR();
			MMC3_SyncMirror();
		}
	}
}

static DECLFW(WriteIRQ) {
	V ^= 0xFF;
	MMC3_IRQWrite(A, V);
}

static DECLFW(WriteASIC) {
	if (m126.reg[3] & 0x08) {
		A = A & ~0x01 | 0x01;
	}
	if ((m126.reg[3] & 0x09) == 0x09) {
		MMC3_Write(0x8000 | (A & 0x01), V);
	} else {
		MMC3_Write(A, V);
	}
}

static void Reset(void) {
	memset(&m126, 0, sizeof(m126));
	dipsw++;
	MMC3_Reset();
}

static void Power(void) {
	memset(&m126, 0, sizeof(m126));
	dipsw = 0;
	MMC3_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
	SetWriteHandler(0x8000, 0xFFFF, WriteASIC);
	SetReadHandler(0x8000, 0xFFFF, ReadDIP);
	if (iNESCart.mapper == 534) {
		SetWriteHandler(0xC000, 0xDFFF, WriteIRQ);
	}
}

static void InitCommon(CartInfo *info) {
	uint8_t ws = 8;
	if (info->iNES2) {
		ws = (info->PRGRamSize + info->PRGRamSaveSize) / 1024;
	}
	MMC3_Init(info, MMC3B, ws, info->battery);
	MMC3_SyncMirror = SyncMirror;
	MMC3_cwrap = SetCHR;
	MMC3_pwrap = SetPRG;
	info->Power = Power;
	info->Reset = Reset;
	AddExState(StateRegs, ~0, 0, NULL);

	switch (iNESCart.CRC32) {
	case 0xEAD80031: /* Gamezone 118-in-1 (AT-207) */
	case 0x6FCBC309: /* Power Joy Classic TV Game 84-in-1 (PJ-008) */
	case 0x6D61FE21: /* 1998 4000000-in-1 (BS-400 PCB) */
	case 0x3FF46175:
	case 0xA3FF9D9B:
	case 0x2BDD0FC2:
	case 0x5789017D: /* (GD-106) 18-in-1 */
	case 0x46A01871: /* 3000000-in-1 (BS-300 PCB) */
	case 0x2466B80A: /* 700000-in-1 (BS-400 PCB) */
	case 0x871CFD16:
	case 0xB2724618:
	case 0x42A9219D:
		oldump = TRUE;
		break;
	}
}

void Mapper126_Init(CartInfo *info) {
	InitCommon(info);
}

void Mapper422_Init(CartInfo *info) {
	InitCommon(info);
}

void Mapper534_Init(CartInfo *info) {
	InitCommon(info);
}
