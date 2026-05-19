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

#ifndef _FCG_H
#define _FCG_H

typedef enum __FCGTypes {
    FCG_TYPE_Unknown,
    FCG_TYPE_FCG,
    FCG_TYPE_LZ93D50
} FCGTypes;

typedef enum __FCGEepromTypes {
    FCG_EEPROM_NONE,
    FCG_EEPROM_C01,
    FCG_EEPROM_C02
} FCGEepromTypes;

typedef struct __FCG {
	uint8_t prg;
	uint8_t chr[8];
	uint8_t mirror;
	uint8_t wramEnabled;
	uint8_t IRQa;
	int16_t IRQCount;
	int16_t IRQLatch;
} FCG;

DECLFW(FCG_Write);

void FCG_Power(void);
void FCG_CPUIRQHook(int a);
void FCG_Reset(void);

void FCG_Init(CartInfo *info, uint8_t _FCGType);
void FCG_SetEeprom(X24C0X *e);

void FCG_SyncPRG(void);
void FCG_SyncCHR(void);
void FCG_SyncMirror(void);
void FCG_SyncWRAM(void);

DECLFR(FCG_Read);
DECLFW(FCG_Write);

extern void (*FCG_pwrap)(uint16_t A, uint16_t V);
extern void (*FCG_cwrap)(uint16_t A, uint16_t V);

#endif /* _FCG_H */
