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
 */

#ifndef _H3001_H
#define _H3001_H

typedef struct __H3001 {
	uint8_t prg[2], chr[8], mirror, cmd;
	uint8_t IRQa;
	int16_t IRQCount, IRQLatch;
} H3001;

extern H3001 h3001;

void H3001_SetPRG_default(uint16_t A, uint16_t V);
void H3001_SetCHR_default(uint16_t A, uint16_t V);
void H3001_SyncPRG_default(void);
void H3001_SyncCHR_default(void);
void H3001_SyncMirror_default(void);

void H3001_CPUIRQHook(int a);
void H3001_Reset(void);
void H3001_Power(void);
void H3001_StateRestore(int version);
void H3001_Init(CartInfo *info);

void H3001_SetConfig(uint8_t clear);

DECLFW(H3001_WritePRG);
DECLFW(H3001_WriteMisc);
DECLFW(H3001_WriteCHR);
DECLFW(H3001_Write);

extern void (*H3001_pwrap)(uint16_t A, uint16_t V);
extern void (*H3001_cwrap)(uint16_t A, uint16_t V);

extern void (*H3001_SyncPRG)(void);
extern void (*H3001_SyncCHR)(void);
extern void (*H3001_SyncMirror)(void);

#endif /* _H3001_H */
