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

/* NES 2.0 Mapper 384
 *
 * NES 2.0 Mapper 384 denotes the L1A16 circuit board, used on a 4-in-1 multicart
 * containing Crisis Force, among other games. It uses a VRC4 clone in the VRC4e
 * configuration (VRC4 A0=CPU A2, VRC4 A1=CPU A3) and has 2 KiB of WRAM, mirrored
 * once, in the CPU $6000-$6FFF area. The outer bank register overlays this area
 * and contains a Lock bit to prevent Crisis Force's WRAM writes from changing the
 * outer bank.
 */

#include "mapinc.h"
#include "vrc24.h"

static struct {
	uint8_t reg;
} m384;

static SFORMAT StateRegs[] = {
	{ &m384.reg, 1, "REGS" },
	{ 0 }
};

static void SetPRG(uint16_t A, uint16_t V) {
	setprg8(A, (m384.reg << 4) | (V & 0x0F));
}

static void SetCHR(uint16_t A, uint16_t V) {
	setchr1(A, (m384.reg << 7) | (V & 0x7F));
}

static DECLFW(WriteReg) {
	CartBW(A, V);
	if ((A & 0x800) && !(m384.reg & 0x08)) {
		m384.reg = V;
		VRC24_SyncPRG();
		VRC24_SyncCHR();
	}
}

static void Reset(void) {
	memset(&m384, 0, sizeof(m384));
	VRC24_Reset();
}

static void Power(void) {
	memset(&m384, 0, sizeof(m384));
	VRC24_Power();
	SetWriteHandler(0x6000, 0x7FFF, WriteReg);
}

void Mapper384_Init(CartInfo *info) {
	VRC24_Init(info, VRC24_VRC4, 0x04, 0x08, TRUE, TRUE);
	info->Power = Power;
	info->Reset = Reset;
	VRC24_pwrap = SetPRG;
	VRC24_cwrap = SetCHR;
	AddExState(StateRegs, ~0, 0, NULL);
}
