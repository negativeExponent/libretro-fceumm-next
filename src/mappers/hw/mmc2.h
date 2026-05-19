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

#ifndef _MMC2_H
#define _MMC2_H

typedef struct __MMC2 {
	uint8_t prg;
	uint8_t chr[4];
	uint8_t latch[2];
	uint8_t mirr;
} MMC2;

extern MMC2 mmc2;

DECLFW(MMC2_Write);

void MMC2_PPUHook(uint32_t A);

void MMC2_Power(void);
void MMC2_Close(void);
void MMC2_Reset(void);
void MMC2_Restore(int version);
void MMC2_Init(CartInfo *info, int wram, int battery);

void MMC2_SetConfig(uint8_t clear);

void MMC2_SetPRG_default(uint16_t A, uint16_t V);
void MMC2_SetCHR_default(uint16_t A, uint16_t V);

void MMC2_SyncPRG(void);
void MMC2_SyncCHR(void);
void MMC2_SyncMirror(void);

extern void (*MMC2_pwrap)(uint16_t A, uint16_t V);
extern void (*MMC2_cwrap)(uint16_t A, uint16_t V);

#endif /* _MMC2_H */
