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

#ifndef _TXC_H
#define _TXC_H

typedef struct __TXC {
	uint8_t accumulator;
	uint8_t inverter;
	uint8_t staging;
	uint8_t output;
	uint8_t increase;
	uint8_t invert;
	uint8_t A;
	uint8_t B;
	uint8_t X;
	uint8_t Y;
} TXC;

extern TXC txc;

DECLFR(TXC_Read);
DECLFW(TXC_Write);

void TXC_Power(void);

void TXC_Init(CartInfo *info, void (*proc)(void));

#endif /* _TXC_H */
