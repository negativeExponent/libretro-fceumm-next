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

#ifndef _VRC1_H
#define _VRC1_H

typedef struct __VRC1 {
	uint8_t prg[3], chr[2], mode;
} VRC1;

extern VRC1 vrc1;

void VRC1_SetPRG_default(uint16_t A, uint16_t V);
void VRC1_SetCHR_default(uint16_t A, uint16_t V);
void VRC1_SyncPRG_default(void);
void VRC1_SyncCHR_default(void);
void VRC1_SyncMirror_default(void);

DECLFW(VRC1_WritePRG);
DECLFW(VRC1_WriteCHR);
DECLFW(VRC1_Write);
DECLFW(VRC1_WriteMode);

void VRC1_Reset(void);
void VRC1_Power(void);
void VRC1_StateRestore(int version);
void VRC1_Init(CartInfo *info);

void VRC1_SetConfig(uint8_t clear);

extern void (*VRC1_SyncPRG)(void);
extern void (*VRC1_SyncCHR)(void);
extern void (*VRC1_SyncMirror)(void);

extern void (*VRC1_pwrap)(uint16_t A, uint16_t V);
extern void (*VRC1_cwrap)(uint16_t A, uint16_t V);

#endif /* _VRC1_H */
