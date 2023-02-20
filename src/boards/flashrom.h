/*
 *  Copyright (C) 2010-2023 Fabio Cavallo (aka FHorse)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef FLASHROM_H_
#define FLASHROM_H_

void Flash_Init(uint8 *data, uint32 size, uint8 manufacter_id, uint8 model_id, uint32 sector_size, uint32 adr1, uint32 adr2);
void FlashWrite(uint32 address, uint8 value);
uint8 FlashRead(uint32 address);
void FlashCpuCycle(void);

#endif /* FLASHROM_H_ */