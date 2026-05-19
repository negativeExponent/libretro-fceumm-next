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

#ifndef _FME7_H
#define _FME7_H
typedef struct __FME7 {
	uint8_t prg[4];
	uint8_t chr[8];
	uint8_t cmd;
	uint8_t mirr;
} FME7;

extern FME7 fme7;

DECLFW(FME7_WriteIndex);
DECLFW(FME7_WriteReg);

void FME7_Init(CartInfo *info, int wram, int battery);
void FME7_SetConfig(uint8_t clear);
void FME7_Power(void);
void FME7_Reset(void);

extern void (*FME7_SyncPRG)(void);
extern void (*FME7_SyncCHR)(void);
extern void (*FME7_SyncMirror)(void);
extern void (*FME7_SyncWRAM)(void);

extern void (*FME7_pwrap)(uint16_t A, uint16_t V);
extern void (*FME7_cwrap)(uint16_t A, uint16_t V);

#endif /* _FME7_H */
