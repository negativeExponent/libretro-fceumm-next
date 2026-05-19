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

#ifndef _MMC3_H
#define _MMC3_H

typedef enum __MMC3TYPE {
    MMC3A = 0,
    MMC3B = 1,
    MMC6B = 2
} MMC3TYPE;

typedef struct __MMC3 {
	uint8_t cmd;
	uint8_t opts;
	uint8_t mirr;
	uint8_t wram;
	uint8_t reg[8];
} MMC3;

extern MMC3 mmc3;

uint8_t MMC3_GetPRGBank(int V);
uint8_t MMC3_GetCHRBank(int V);

DECLFW(MMC3_CMDWrite); /* $ 0x8000 - 0xBFFF */
DECLFW(MMC3_IRQWrite); /* $ 0xC000 - 0xFFFF */
DECLFW(MMC3_Write);    /* $ 0x8000 - 0xFFFF */
DECLFW(MBWRAMMMC6);
DECLFR(MAWRAMMMC6);

void MMC3_Power(void);
void MMC3_Reset(void);
void MMC3_Close(void);
void MMC3_IRQHBHook(void);
int MMC3_WramIsWritable(void);
void MMC3_Init(CartInfo *info, MMC3TYPE _type, int wram, int battery);

void MMC3_SetConfig(uint8_t clear, MMC3TYPE _type);

extern void (*MMC3_SyncPRG)(void);
extern void (*MMC3_SyncCHR)(void);
extern void (*MMC3_SyncMirror)(void);

extern void (*MMC3_pwrap)(uint16_t A, uint16_t V);
extern void (*MMC3_cwrap)(uint16_t A, uint16_t V);

#endif /* _MMC3_H */
