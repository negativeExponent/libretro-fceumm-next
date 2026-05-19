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

#ifndef FLASHROM_H
#define FLASHROM_H

DECLFW(FlashROM_Write);
DECLFR(FlashROM_Read);

void FlashROM_Init(uint8_t *data, uint32_t size, uint8_t manufacter_id, uint8_t model_id, uint32_t sector_size, uint32_t adr1, uint32_t adr2);
void FlashROM_CPUCyle(int a);

#endif /* FLASHROM_H */
