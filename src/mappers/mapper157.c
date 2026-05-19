/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

/* FIXME: Bar code input interface not attached yet */

#include "mapinc.h"
#include "eeprom_24C0x.h"
#include "fcg.h"

/* Datach Barcode Battler */

static uint8_t BarcodeData[256];
static int BarcodeReadPos;
static int BarcodeCycleCount;
static uint32_t BarcodeOut;

/* #define INTERL2OF5 */

int FCEUI_DatachSet(uint8_t *rcode) {
	int prefix_parity_type[10][6] = {
		{ 0, 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 1, 1 }, { 0, 0, 1, 1, 0, 1 }, { 0, 0, 1, 1, 1, 0 },
		{ 0, 1, 0, 0, 1, 1 }, { 0, 1, 1, 0, 0, 1 }, { 0, 1, 1, 1, 0, 0 }, { 0, 1, 0, 1, 0, 1 },
		{ 0, 1, 0, 1, 1, 0 }, { 0, 1, 1, 0, 1, 0 }
	};
	int data_left_odd[10][7] = {
		{ 0, 0, 0, 1, 1, 0, 1 }, { 0, 0, 1, 1, 0, 0, 1 }, { 0, 0, 1, 0, 0, 1, 1 }, { 0, 1, 1, 1, 1, 0, 1 },
		{ 0, 1, 0, 0, 0, 1, 1 }, { 0, 1, 1, 0, 0, 0, 1 }, { 0, 1, 0, 1, 1, 1, 1 }, { 0, 1, 1, 1, 0, 1, 1 },
		{ 0, 1, 1, 0, 1, 1, 1 }, { 0, 0, 0, 1, 0, 1, 1 }
	};
	int data_left_even[10][7] = {
		{ 0, 1, 0, 0, 1, 1, 1 }, { 0, 1, 1, 0, 0, 1, 1 }, { 0, 0, 1, 1, 0, 1, 1 }, { 0, 1, 0, 0, 0, 0, 1 },
		{ 0, 0, 1, 1, 1, 0, 1 }, { 0, 1, 1, 1, 0, 0, 1 }, { 0, 0, 0, 0, 1, 0, 1 }, { 0, 0, 1, 0, 0, 0, 1 },
		{ 0, 0, 0, 1, 0, 0, 1 }, { 0, 0, 1, 0, 1, 1, 1 }
	};
	int data_right[10][7] = {
		{ 1, 1, 1, 0, 0, 1, 0 }, { 1, 1, 0, 0, 1, 1, 0 }, { 1, 1, 0, 1, 1, 0, 0 }, { 1, 0, 0, 0, 0, 1, 0 },
		{ 1, 0, 1, 1, 1, 0, 0 }, { 1, 0, 0, 1, 1, 1, 0 }, { 1, 0, 1, 0, 0, 0, 0 }, { 1, 0, 0, 0, 1, 0, 0 },
		{ 1, 0, 0, 1, 0, 0, 0 }, { 1, 1, 1, 0, 1, 0, 0 }
	};
	uint8_t code[13 + 1];
	uint32_t tmp_p = 0;
	uint32_t csum = 0;
	int i, j;
	int len;

	for (i = len = 0; i < 13; i++) {
		if (!rcode[i]) {
			break;
		}
		if ((code[i] = rcode[i] - '0') > 9) {
			return (0);
		}
		len++;
	}
	if (len != 13 && len != 12 && len != 8 && len != 7) {
		return (0);
	}

#define BS(x)                                                                                                                                                                                          \
	BarcodeData[tmp_p] = x;                                                                                                                                                                            \
	tmp_p++

	for (j = 0; j < 32; j++) { /* delay before sending a code */
		BS(0x00);
	}

#ifdef INTERL2OF5

	BS(1); BS(1); BS(0); BS(0); /* 1 */
	BS(1); BS(1); BS(0); BS(0); /* 1 */
	BS(1); BS(1); BS(0); BS(0); /* 1 */
	BS(1); BS(1); BS(0); BS(0); /* 1 */
	BS(1); BS(1); BS(0); BS(0); /* 1 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 */
	BS(1);        BS(0); BS(0); /* 0 cs */
	BS(1); BS(1); BS(0); BS(0); /* 1 */

#else
	/* Left guard bars */
	BS(1); BS(0); BS(1);

	if (len == 13 || len == 12) {

		for (i = 0; i < 6; i++) {
			if (prefix_parity_type[code[0]][i]) {
				for (j = 0; j < 7; j++) {
					BS(data_left_even[code[i + 1]][j]);
				}
			} else {
				for (j = 0; j < 7; j++) {
					BS(data_left_odd[code[i + 1]][j]);
				}
			}
		}

		/* Center guard bars */
		BS(0); BS(1); BS(0); BS(1); BS(0);

		for (i = 7; i < 12; i++) {
			for (j = 0; j < 7; j++) {
				BS(data_right[code[i]][j]);
			}
		}
		/* Calc and write down the control code if not assigned, instead, send code as is
		   Battle Rush uses modified type of codes with different control code calculation */
		if (len == 12) {
			for (i = 0; i < 12; i++) {
				csum += code[i] * ((i & 1) ? 3 : 1);
			}
			csum = (10 - (csum % 10)) % 10;
			rcode[12] = csum + 0x30; /* update check code to the input string as well */
			rcode[13] = 0;
			code[12] = csum;
		}
		for (j = 0; j < 7; j++) {
			BS(data_right[code[12]][j]);
		}
	} else if (len == 8 || len == 7) {
		for (i = 0; i < 4; i++) {
			for (j = 0; j < 7; j++) {
				BS(data_left_odd[code[i]][j]);
			}
		}

		/* Center guard bars */
		BS(0); BS(1); BS(0); BS(1); BS(0);

		for (i = 4; i < 7; i++) {
			for (j = 0; j < 7; j++) {
				BS(data_right[code[i]][j]);
			}
		}
		csum = 0;
		for (i = 0; i < 7; i++) {
			csum += (i & 1) ? code[i] : (code[i] * 3);
		}
		csum = (10 - (csum % 10)) % 10;
		rcode[7] = csum + 0x30; /* update check code to the input string as well */
		rcode[8] = 0;
		for (j = 0; j < 7; j++) {
			BS(data_right[csum][j]);
		}
	}

	/* Right guard bars */
	BS(1); BS(0); BS(1);
#endif

	for (j = 0; j < 32; j++) {
		BS(0x00);
	}

	BS(0xFF);

#undef BS

	BarcodeReadPos = 0;
	BarcodeOut = 0x8;
	BarcodeCycleCount = 0;
	return (1);
}

static struct {
	uint8_t epromLatch;
	/* first 256K is internal m157.epromData data, 2nd 256 is for external m157.epromData if used.
 	 * Combined here for simplicity and frontend save compatibility */
	uint8_t epromData[512];
} m157;

static X24C0X internalEeprom = { 0 };
static X24C0X extraEeprom = { 0 };

static uint8_t hasExternalEEPROM = FALSE;

static void SetPRG(uint16_t A, uint16_t V) {
	setprg16(A, V & 0x0F);
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr8(0);
}

static DECLFR(ReadEeprom) {
	uint8_t ret = 0x10;
	ret &= eeprom_read(&internalEeprom);
	if (hasExternalEEPROM) {
		ret &= eeprom_read(&extraEeprom);
	}
	ret |= BarcodeOut & 0x08;
	return ((cpu.openbus & ~0x18) | (ret & 0x18));
}

static DECLFW(WriteEepromReg) {
	switch (A & 0x0F) {
	case 0x00:
		FCG_Write(A, V);
		m157.epromLatch &= ~0x20;
		m157.epromLatch |= (V << 2) & 0x20;
		if (hasExternalEEPROM) {
			eeprom_i2c_step(&extraEeprom, (m157.epromLatch & 0x20) >> 5, (m157.epromLatch & 0x40) >> 6);
		}
		break;
	case 0x0D:
		m157.epromLatch &= ~0x40;
		m157.epromLatch |= (V & 0x40);
		if (hasExternalEEPROM) {
			eeprom_i2c_step(&extraEeprom, (m157.epromLatch & 0x20) >> 5, (m157.epromLatch & 0x40) >> 6);
		}
		eeprom_i2c_step(&internalEeprom,
			(V & 0x80) ? TRUE : (V & 0x20) >> 5,
			(V & 0x80) ? TRUE : (V & 0x40) >> 6);
		break;
	default:
		FCG_Write(A, V);
		break;
	}
}

static void HBIRQHook(int a) {
	FCG_CPUIRQHook(a);

	BarcodeCycleCount += a;
	if (BarcodeCycleCount >= 1000) {
		BarcodeCycleCount -= 1000;
		if (BarcodeData[BarcodeReadPos] == 0xFF) {
			BarcodeOut = 0;
		} else {
			BarcodeOut = (BarcodeData[BarcodeReadPos] ^ 1) << 3;
			BarcodeReadPos++;
		}
	}
}

static void Power(void) {
	FCG_Power();

	BarcodeData[0] = 0xFF;
	BarcodeReadPos = 0;
	BarcodeOut = 0;
	BarcodeCycleCount = 0;

	SetReadHandler(0x6000, 0x7FFF, ReadEeprom);
	SetWriteHandler(0x8000, 0xFFFF, WriteEepromReg);
}

void Mapper157_Init(CartInfo *info) {
	FCG_Init(info, FCG_TYPE_LZ93D50);
	FCG_pwrap = SetPRG;
	FCG_cwrap = SetCHR;

	info->Power = Power;
	MapIRQHook = HBIRQHook;

	GameInfo->cspecial = SIS_DATACH;

	/* internal eeprom.data shared among all games
	and always enabled regardless of battery flag */
	info->battery = 1;
	eeprom_24C02_init(&internalEeprom, &m157.epromData[0]);
	eeprom_AddStateInfo(&internalEeprom);

	if (!info->iNES2 || (info->PRGRamSaveSize & 0xF0)) {
		/* additional 128 external epromData */
		eeprom_24C01_init(&extraEeprom, &m157.epromData[256]);
		eeprom_AddStateInfo(&extraEeprom);
		AddExState(&m157.epromLatch, 1, 0, "LATC");
		hasExternalEEPROM = TRUE;
	}

	info->SaveGame[0] = m157.epromData;
	if (hasExternalEEPROM) {
		info->SaveGameLen[0] = 512;
	} else {
		info->SaveGameLen[0] = 256;
	}
}
