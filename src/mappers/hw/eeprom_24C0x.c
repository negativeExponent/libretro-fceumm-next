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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include "mapinc.h"
#include "eeprom_24C0x.h"

static INLINE void eeprom_24C01_Start(X24C0X *e) {
	e->mode = MODE_ADDRESS;
	e->latch.bit = 0;
	e->latch.address = 0;
	e->output = 0x10;
}

static INLINE void eeprom_24C02_Start(X24C0X *e) {
	e->mode = MODE_DATA;
	e->latch.bit = 0;
	e->output = 0x10;
}

static INLINE void eeprom_24C0X_Stop(X24C0X *e) {
	e->mode = MODE_IDLE;
	e->output = 0x10;
}

static INLINE void eeprom_24C01_Rise(X24C0X *e, uint8_t bit) {
	if (e->mode == MODE_ADDRESS) {
		if (e->latch.bit < 7) {
			e->latch.address &= ~(1U << e->latch.bit);
			e->latch.address |= bit << e->latch.bit++;
		} else if (e->latch.bit < 8) {
			e->latch.bit = 8;
			if (bit) {
				e->next = MODE_READ;
				e->latch.data = e->mem[e->latch.address];
			} else {
				e->next = MODE_WRITE;
			}
		}
	} else if (e->mode == MODE_ACK) {
		e->output = 0x00;
	} else if (e->mode == MODE_READ && (e->latch.bit < 8)) {
		e->output = (e->latch.data & 1U << e->latch.bit++) ? 0x10 : 0x00;
	} else if (e->mode == MODE_WRITE && (e->latch.bit < 8)) {
		e->latch.data &= ~(1U << e->latch.bit);
		e->latch.data |= bit << e->latch.bit++;
	} else if (e->mode == MODE_ACK_WAIT && (bit == 0)) {
		e->next = MODE_IDLE;
	}
}

static INLINE void eeprom_24C01_Fall(X24C0X *e) {
	if (e->mode == MODE_ADDRESS && (e->latch.bit == 8)) {
		e->mode = MODE_ACK;
		e->output = 0x10;
	} else if (e->mode == MODE_ACK) {
		e->mode = e->next;
		e->latch.bit = 0;
		e->output = 0x10;
	} else if (e->mode == MODE_READ && (e->latch.bit == 8)) {
		e->mode = MODE_ACK_WAIT;
		e->latch.address = (e->latch.address + 1) & 0x7F;
	} else if (e->mode == MODE_WRITE && (e->latch.bit == 8)) {
		e->mode = MODE_ACK;
		e->next = MODE_IDLE;
		e->mem[e->latch.address] = e->latch.data;
		e->latch.address = (e->latch.address + 1) & 0x7F;
	}
}

static INLINE void eeprom_24C02_Rise(X24C0X *e, uint8_t bit) {
	if (e->mode == MODE_DATA && (e->latch.bit < 8)) {
		e->latch.data &= ~(1U << (7 - e->latch.bit));
		e->latch.data |= bit << (7 - e->latch.bit++);
	} else if (e->mode == MODE_WRITE && (e->latch.bit < 8)) {
		e->latch.data &= ~(1U << (7 - e->latch.bit));
		e->latch.data |= bit << (7 - e->latch.bit++);
	} else if (e->mode == MODE_ADDRESS && (e->latch.bit < 8)) {
		e->latch.address &= ~(1U << (7 - e->latch.bit));
		e->latch.address |= bit << (7 - e->latch.bit++);
	} else if (e->mode == MODE_READ && (e->latch.bit < 8)) {
		e->output = (e->latch.data & (1U << (7 - e->latch.bit++))) ? 0x10 : 0x00;
	} else if (e->mode == MODE_NOT_ACK) {
		e->output = 0x10;
	} else if (e->mode == MODE_ACK) {
		e->output = 0x00;
	} else if (e->mode == MODE_ACK_WAIT && (bit == 0)) {
		e->next = MODE_READ;
		e->latch.data = e->mem[e->latch.address];
	}
}

static INLINE void eeprom_24C02_Fall(X24C0X *e) {
	if (e->mode == MODE_DATA && (e->latch.bit == 8)) {
		if ((e->latch.data & 0xA0) == 0xA0) {
			e->latch.bit = 0;
			e->mode = MODE_ACK;
			e->rw = e->latch.data & 0x01;
			e->output = 0x10;

			if (e->rw) {
				e->next = MODE_READ;
				e->latch.data = e->mem[e->latch.address];
			} else {
				e->next = MODE_ADDRESS;
			}
		} else {
			e->mode = MODE_NOT_ACK;
			e->next = MODE_IDLE;
			e->output = 0x10;
		}
	} else if (e->mode == MODE_ADDRESS && (e->latch.bit == 8)) {
		e->latch.bit = 0;
		e->mode = MODE_ACK;
		e->next = (e->rw ? MODE_IDLE : MODE_WRITE);
		e->output = 0x10;
	} else if (e->mode == MODE_READ && (e->latch.bit == 8)) {
		e->mode = MODE_ACK_WAIT;
		e->latch.address = (e->latch.address + 1) & 0xFF;
	} else if (e->mode == MODE_WRITE && (e->latch.bit == 8)) {
		e->latch.bit = 0;
		e->mode = MODE_ACK;
		e->next = MODE_WRITE;
		e->mem[e->latch.address] = e->latch.data;
		e->latch.address = (e->latch.address + 1) & 0xFF;
	} else if (e->mode == MODE_NOT_ACK) {
		e->mode = MODE_IDLE;
		e->latch.bit = 0;
		e->output = 0x10;
	} else if (e->mode == MODE_ACK || e->mode == MODE_ACK_WAIT) {
		e->mode = e->next;
		e->latch.bit = 0;
		e->output = 0x10;
	}
}

static INLINE void eeprom_24C01_step(X24C0X *e, uint8_t scl, uint8_t sda) {
	if (e->line.scl && sda < e->line.sda) {
		eeprom_24C01_Start(e);
	} else if (e->line.scl && sda > e->line.sda) {
		eeprom_24C0X_Stop(e);
	} else if (scl > e->line.scl) {
		eeprom_24C01_Rise(e, sda);
	} else if (scl < e->line.scl) {
		eeprom_24C01_Fall(e);
	}

	e->line.scl = scl;
	e->line.sda = sda;
}

static INLINE void eeprom_24C02_step(X24C0X *e, uint8_t scl, uint8_t sda) {
	if (e->line.scl && sda < e->line.sda) {
		eeprom_24C02_Start(e);
	} else if (e->line.scl && sda > e->line.sda) {
		eeprom_24C0X_Stop(e);
	} else if (scl > e->line.scl) {
		eeprom_24C02_Rise(e, sda);
	} else if (scl < e->line.scl) {
		eeprom_24C02_Fall(e);
	}

	e->line.scl = scl;
	e->line.sda = sda;
}

void eeprom_i2c_step(X24C0X *e, uint8_t scl, uint8_t sda) {
	if (e->model == EEPROM_24C01) {
		eeprom_24C01_step(e, scl, sda);
	} else {
		eeprom_24C02_step(e, scl, sda);
	}
}

uint8_t eeprom_read(X24C0X *e) {
	return e->output;
}

/* Init EEPROM */
void eeprom_init(X24C0X *e, uint8_t model, uint8_t *_rom) {
	memset(e, 0, sizeof(*e));
	e->mem = _rom;
	e->model = model;
	e->line.scl = 0;
	e->line.sda = 0;
	e->mode = MODE_IDLE;
	e->next = MODE_IDLE;
	e->latch.bit = 0;
	e->latch.address = 0;
	e->latch.data = 0;
	e->rw = FALSE;
	e->output = 0x10;
}

void eeprom_AddStateInfo(X24C0X *e) {
	uint8_t is24C01 = (e->model == EEPROM_24C01);

	AddExState(&e->model,         1, 0, is24C01 ? "MDL0" : "MDL1");
	AddExState(&e->mode,          1, 0, is24C01 ? "MOD0" : "MOD1");
	AddExState(&e->next,          1, 0, is24C01 ? "NXT0" : "NXT1");

	AddExState(&e->line.sda,      1, 0, is24C01 ? "SDA0" : "SDA1");
	AddExState(&e->line.scl,      1, 0, is24C01 ? "SCL0" : "SCL1");

	AddExState(&e->output,        1, 0, is24C01 ? "OUT0" : "OUT1");
	AddExState(&e->latch.bit,     1, 0, is24C01 ? "BIT0" : "BIT1");
	AddExState(&e->latch.address, 1, 0, is24C01 ? "ADR0" : "ADR1");
	AddExState(&e->latch.data,    1, 0, is24C01 ? "DAT0" : "DAT1");

	AddExState(&e->rw,            1, 0, is24C01 ? "RW_0" : "RW_1");
}

void eeprom_24C01_init(X24C0X *e, uint8_t *_rom) {
	eeprom_init(e, EEPROM_24C01, _rom);
}

void eeprom_24C02_init(X24C0X *e, uint8_t *_rom) {
	eeprom_init(e, EEPROM_24C02, _rom);
}
