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

#ifndef _MMC1_H
#define _MMC1_H

typedef enum {
    MMC1A,
    MMC1B
} MMC1TYPE;

typedef struct __MMC1 {
	uint8_t reg[4];
	uint8_t buffer;
	uint8_t shift;
} MMC1;

extern MMC1 mmc1;

uint32_t MMC1_GetPRGBank(int index);
uint32_t MMC1_GetCHRBank(int index);
uint8_t MMC1_WRAMEnabled(void);

DECLFW(MMC1_Write);
DECLFR(MMC1_readWRAM);
DECLFW(MMC1_writeWRAM);

void MMC1_Power(void);
void MMC1_Close(void);
void MMC1_Restore(int version);
void MMC1_Reset(void);

void MMC1_Init(CartInfo *info, MMC1TYPE _type, int wram, int saveram);
void MMC1_SetConfig(uint8_t clear, MMC1TYPE _type);

void MMC1_SyncPRG_default(void);
void MMC1_SyncCHR_default(void);
void MMC1_SyncMirror_default(void);
void MMC1_SyncWRAM_default(void);

void MMC1_pwrap_default(uint16_t A, uint16_t V);
void MMC1_cwrap_default(uint16_t A, uint16_t V);

extern void (*MMC1_pwrap)(uint16_t A, uint16_t V);
extern void (*MMC1_cwrap)(uint16_t A, uint16_t V);

extern void (*MMC1_SyncPRG)(void);
extern void (*MMC1_SyncCHR)(void);
extern void (*MMC1_SyncMirror)(void);
extern void (*MMC1_SyncWRAM)(void);

#endif /* _MMC1_H */
