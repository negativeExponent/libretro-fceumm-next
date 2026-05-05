#include "mapinc.h"
#include "fme7.h"
#include "latch.h"
#include "mmc1.h"
#include "mmc2.h"
#include "mmc4.h"
#include "mmc3.h"
#include "vrc1.h"
#include "vrc24.h"
#include "vrc3.h"
#include "vrc6.h"
#include "vrc7.h"

static void (*mapperSync)(void) = NULL;
static void SetMode(uint8_t);
static DECLFR(ReadReg);
static DECLFW(WriteReg);

static struct {
	uint8_t reg[4];        /* Supervisor registers */
	uint8_t Custom_reg[4]; /* Registers for custom mappers */
	uint8_t eeprom[16], eep_clock, state, command, output; /* Serial EEPROM */
} m468;

static SFORMAT StateRegs[] = {
	{ &m468.reg, 4, "EXPR" },
	{ &m468.Custom_reg, 4, "CURG" },
	{ m468.eeprom, 16, "EEPR" },
	{ &m468.eep_clock, 1, "EEP0" },
	{ &m468.state, 1, "EEP1" },
	{ &m468.command, 1, "EEP2" },
	{ &m468.output, 1, "EEP3" },
	{ 0 }
};

/* Serial EEPROM */
static const uint16_t lut509[512] = { /* Look-up table, used only by Legendary Games of NES 509-in-1 */
	  7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,
	 47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,   0,   1,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
	 85,  86,  87,  88,  89,  90,   4,  91,  92,  93,  94,  95,  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
	124, 125, 126, 127, 128, 129,   2,   3, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139,   5, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
	161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200,
	201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 238, 239, 240, 241,
	242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 256,   6, 257, 258, 259, 260, 261, 262, 263, 264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276, 277, 278, 279, 280, 281,
	282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293, 294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310, 311, 312, 313, 314, 315, 316, 317, 318, 319, 320, 321,
	322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335, 336, 337, 338, 339, 340, 341, 342, 343, 344, 345, 346, 347, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362,
	363, 364, 365, 366, 367, 368, 369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385, 386, 387, 388, 389, 390, 391, 392, 393, 394, 395, 396, 397, 398, 399, 400, 401, 402,
	403, 404, 405, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418, 419, 420, 421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439, 440, 441, 442,
	443, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 454, 455, 456, 457, 458, 459, 460, 461, 462, 463, 464, 465, 466, 467, 468, 469, 470, 471, 472, 473, 474, 475, 476, 477, 478, 479, 480, 481, 482,
	483, 484, 485, 486, 487, 488, 489, 490, 491, 492, 493, 494, 495, 496, 497, 498, 499, 500, 501, 502, 503, 504, 505, 506, 507, 508, 512, 513, 514, 515, 516, 517
};

static void setPins(uint8_t select, uint8_t newClock, uint8_t newData) { /* Serial EEPROM */
	if (select) {
		m468.state = 0;
	} else if (!m468.eep_clock && !!newClock) {
		if (m468.state < 8) {
			m468.command = m468.command << 1 | !!(newData) * 1;
			if (++m468.state == 8 && (m468.command & 0xF0) != 0x50 && (m468.command & 0xF0) != 0xA0) {
				m468.state = 0;
			}
		} else {
			uint32_t mask = 1 << (15 - m468.state);
			uint32_t address = m468.command & 0x0F;
			if ((m468.command & 0xF0) == 0xA0) {
				m468.eeprom[address] = m468.eeprom[address] & ~mask | !!(newData)*mask;
				/* The "write" m468.command also silently returns the content of a
				 * lookup table */
				m468.output = !!(lut509[m468.eeprom[0] | m468.eeprom[1] | m468.eeprom[2] << 8 & 0x1FF] >> (address & 1 ? 0 : 8) & mask);
			} else if ((m468.command & 0xF0) == 0x50) {
				m468.output = !!(m468.eeprom[address] & mask);
			}

			if (++m468.state == 16) {
				m468.state = 0;
			}
		}
	}
	m468.eep_clock = newClock;
}

static INLINE uint16_t Mapper_GetPRGBase(void) {
	return ((m468.reg[(iNESCart.submapper == 1) ? 2 : 3] << 9) & 0x2000) | ((m468.reg[1] << 5) & 0x1FE0) | ((m468.reg[0] << 4) & 0x0010);
}

/* Mapper syncs */
static void Sync(void) {
	if (mapperSync) {
		mapperSync();
	}
}

static void Mapper_SyncWRAM(void) {
	if (PRGsize[0]) {
		setprg8r(0x10, 0x6000, 0);
	}
}

static void Sync_AxROM(void) {
	uint16_t prgMask = ((m468.reg[0] & 0x20) ? 0x0F : ((m468.reg[0] & 0x02) ? 0x03 : 0x07));
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg32(0x8000, (latch.data & prgMask) | ((prgBase >> 2) & ~prgMask));
	setchr8(0);
	setmirror((latch.data & 0x10) ? MI_1 : MI_0);
}

static void Sync_BxROM(void) {
	uint16_t prgMask = ((m468.reg[0] & 0x20) ? 0x0F : ((m468.reg[0] & 0x02) ? 0x03 : 0x07));
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg32(0x8000, (latch.data & prgMask) | (prgBase >> 2 & ~prgMask));
	setchr8(0);
	setmirror((m468.reg[0] & 0x04) ? MI_H : MI_V);
}

static void SetPRG_FME7(uint16_t A, uint16_t V) {
	uint16_t prgMask = (m468.reg[0] & 0x02) ? 0x0F : 0x1F;
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_FME7(uint16_t A, uint16_t V) {
	setchr1(A, V & 0xFF);
}

static void Sync_FME7(void) {
	FME7_pwrap = SetPRG_FME7;
	FME7_cwrap = SetCHR_FME7;
	FME7_SyncPRG();
	FME7_SyncCHR();
	FME7_SyncMirror();
	FME7_SyncWRAM();
}

static void SetPRG_FxROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = (m468.reg[0] & 0x02) ? 0x07 : 0x0F;
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg16(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_FxROM(uint16_t A, uint16_t V) {
	setchr4(A, V & 0xFF);
}

static void Sync_FxROM(void) {
	MMC4_pwrap = SetPRG_FxROM;
	MMC4_cwrap = SetCHR_FxROM;
	MMC4_SyncPRG();
	MMC4_SyncCHR();
	MMC4_SyncMirror();
	Mapper_SyncWRAM();
}

static void Sync_GNROM(void) {
	uint16_t prgMask = (m468.reg[0] & 0x08) ? 0x01 : 0x03;
	uint16_t prgBase = Mapper_GetPRGBase();
	uint8_t value = latch.data;
	if ((iNESCart.submapper == 1) && (~m468.reg[0] & 0x08) || (iNESCart.submapper != 1) && (prgBase & 0x2000)) {
		value = ((latch.data >> 4) & 0x0F) | ((latch.data << 4) & 0xF0);
	}
	prgBase = (prgBase >> 2) | ((m468.reg[0] >> 1) & 0x02);
	setprg32(0x8000, (value & prgMask) | (prgBase & ~prgMask));
	setchr8(value >> 4);
	setmirror((m468.reg[0] & 0x10) ? MI_H : MI_V);
}

static void Sync_IF12(void) {
	uint16_t prgMask = (m468.reg[0] & 0x02) ? 0x07 : 0x0F;
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg16(0x8000, (m468.Custom_reg[1] & prgMask) | (prgBase >> 1 & ~prgMask));
	setprg16(0xC000, (prgBase >> 1) | prgMask);
	setchr8(m468.Custom_reg[0] >> 1 & 0x0F);
	setmirror(m468.Custom_reg[0] & 0x01 ? MI_H : MI_V);
}

static DECLFW(IF12_WriteReg) {
	m468.Custom_reg[((A >> 14) & 1)] = V;
	Sync();
}

static void Sync_LF36(void) {
	uint16_t prgBase = Mapper_GetPRGBase();
	prgBase |= m468.reg[0] & 0x08;
	setprg8(0x8000, 0x04 | prgBase);
	setprg8(0xA000, 0x05 | prgBase);
	setprg8(0xC000, m468.Custom_reg[0] & 0x07 | prgBase);
	setprg8(0xE000, 0x07 | prgBase);
	setchr8(0);
	setmirror(m468.reg[0] & 0x04 ? MI_H : MI_V);
}

static void LF36_cpuCycle(int a) {
	while (a--) {
		if (m468.Custom_reg[1] & 1) {
			if (!++m468.Custom_reg[2])
				++m468.Custom_reg[3];
			if (m468.Custom_reg[3] & 0x10)
				X6502_IRQBegin(FCEU_IQEXT);
			else
				X6502_IRQEnd(FCEU_IQEXT);
		} else {
			m468.Custom_reg[2] = m468.Custom_reg[3] = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
	}
}

static DECLFW(LF36_WriteReg) {
	switch (A >> 13 & 3) {
	case 0:
	case 1:
		m468.Custom_reg[1] = A >> 13 & 1;
		break;
	case 3:
		m468.Custom_reg[0] = V;
		Sync();
	}
}

static void Sync_Misc(void) {
	uint16_t prgBase = Mapper_GetPRGBase();
	if (m468.reg[0] & 0x02) {
		setprg16(0x8000, m468.Custom_reg[2] << 1 & 0x0E | m468.reg[0] & 0x01 | prgBase >> 1 & ~0x0F);
		setprg16(0xC000, m468.Custom_reg[2] << 1 & 0x0E | m468.reg[0] & 0x01 | prgBase >> 1 & ~0x0F);
		setchr8(m468.Custom_reg[0] & 0x03);
	} else {
		setprg32(0x8000, m468.Custom_reg[2] & 0x07 | prgBase >> 2 & ~0x07);
		setchr8(m468.Custom_reg[0] & 0x0F);
	}
	if (m468.reg[0] & 0x08) {
		setmirror(m468.reg[0] & 0x04 ? MI_H : MI_V);
	} else {
		setmirror(m468.Custom_reg[1] & 0x10 ? MI_1 : MI_0);
	}
}

static DECLFW(Misc_WriteReg) {
	switch (A >> 12 & 7) {
	case 0:
	case 2:
	case 3:
		m468.Custom_reg[0] = V;
		Sync();
		break;
	case 1:
		m468.Custom_reg[m468.reg[0] & 0x08 ? 0 : 1] = V;
		Sync();
		break;
	case 6:
	case 7:
		m468.Custom_reg[2] = V;
		Sync();
		break;
	}
}

static void Sync_Nanjing(void) {
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg32(0x8000, m468.Custom_reg[2] << 4 & 0x30 | m468.Custom_reg[0] & 0x0F | (m468.Custom_reg[3] & 0x04 ? 0x00 : 0x03) | prgBase >> 2);
	setchr8(0);
	setmirror(m468.reg[0] & 0x04 ? MI_H : MI_V);
}

static void Nanjing_scanline(void) {
	if (m468.Custom_reg[0] & 0x80 && scanline < 239) {
		setchr4(0x0000, scanline >= 127 ? 1 : 0);
		setchr4(0x1000, scanline >= 127 ? 1 : 0);
	} else {
		setchr8(0);
	}
}

static DECLFW(Nanjing_WriteReg) {
	m468.Custom_reg[A >> 8 & 3] = V;
	Sync();
}

static void SetPRG_PNROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = 0x0F;
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_PNROM(uint16_t A, uint16_t V) {
	setchr1(A, V & 0xFF);
}

static void Sync_PNROM(void) {
	MMC2_pwrap = SetPRG_PNROM;
	MMC2_cwrap = SetCHR_PNROM;
	MMC2_SyncPRG();
	MMC2_SyncCHR();
	MMC2_SyncMirror();
	Mapper_SyncWRAM();
}

static void SetPRG_SxROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = ((m468.reg[0] & 0x02) ? ((m468.reg[0] & 0x08) ? 0x03 : 0x07) : 0x0F);
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg16(A, (((prgBase >> 1) | (m468.reg[0] & 0x06)) & ~prgMask) | (V & prgMask));
}

static void SetCHR_SxROM(uint16_t A, uint16_t V) {
	setchr4(A, V & 0xFF);
}

static void Sync_SxROM(void) {
	MMC1_pwrap = SetPRG_SxROM;
	MMC1_cwrap = SetPRG_SxROM;
	MMC1_SyncPRG();
	MMC1_SyncCHR();
	MMC1_SyncMirror();
	MMC1_SyncWRAM();
}

static void SetPRG_SUROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = 0x1F;
	uint16_t prgBase = Mapper_GetPRGBase() >> 1;
	setprg16(A, (prgBase & ~prgMask) | ((V | MMC1_GetCHRBank(0) & 0x10) & prgMask));
}

static void SetCHR_SUROM(uint16_t A, uint16_t V) {
	setchr4(A, V & 0xFF);
}

static void Sync_SUROM(void) {
	MMC1_pwrap = SetPRG_SUROM;
	MMC1_cwrap = SetCHR_SUROM;
	MMC1_SyncPRG();
	MMC1_SyncCHR();
	MMC1_SyncMirror();
	MMC1_SyncWRAM();
}

static void SetPRG_TxROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = (m468.reg[0] & 0x08) ? ((m468.reg[0] & 0x04) ? (m468.reg[0] & 0x02 ? ((m468.reg[2] & 0x02) ? 0x07 : 0x0F) : 0x1F) : 0x3F) : 0x7F;
	uint16_t prgBase = Mapper_GetPRGBase();
	prgBase |= m468.reg[2] & 0x01 ? 0x0C : 0x00;
	setprg8(A, (prgBase & ~prgMask) | ((V | MMC1_GetCHRBank(0) & 0x10) & prgMask));
}

static void SetCHR_TxROM(uint16_t A, uint16_t V) {
	uint16_t chrMask = m468.reg[0] & 0x10 ? 0xFF : 0x7F;
	setchr1(A, V & chrMask);
}

static void Sync_TxROM(void) {
	MMC3_pwrap = SetPRG_TxROM;
	MMC3_cwrap = SetCHR_TxROM;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
	MMC3_SyncMirror();
	Mapper_SyncWRAM();
}

static void SetPRG_TxSROM(uint16_t A, uint16_t V) {
	uint16_t prgMask = m468.reg[0] & 0x08 ? (m468.reg[0] & 0x04 ? (m468.reg[0] & 0x02 ? (m468.reg[2] & 0x02 ? 0x07 : 0x0F) : 0x1F) : 0x3F) : 0x7F;
	uint16_t prgBase = Mapper_GetPRGBase();
	prgBase |= (m468.reg[2] & 0x01) ? 0x0C : 0x00;
	setprg8(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_TxSROM(uint16_t A, uint16_t V) {
	uint16_t chrMask = 0x7F;
	setchr1(A, V & chrMask);
}

static void SyncMirror_TxSROM(void) {
	switch (mmc3.mirr & 0x03) { /* Only A000=02 is TxSROM. H/V mirroring is necessary for Ys 1, modified for MMC3. */
	case 0: setmirror(MI_V); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror((MMC3_GetCHRBank(0) & 0x80) ? MI_1 : MI_0); break;
	case 3: setmirror(MI_1); break;
	}
}

static void Sync_TxSROM(void) {
	MMC3_pwrap = SetPRG_TxSROM;
	MMC3_cwrap = SetCHR_TxSROM;
	MMC3_SyncMirror = SyncMirror_TxSROM;
	MMC3_SyncPRG();
	MMC3_SyncCHR();
	MMC3_SyncMirror();
	Mapper_SyncWRAM();
}

static void Sync_UxROM(void) {
	uint16_t prgMask = (m468.reg[0] & 0x02) ? 0x07 : (((iNESCart.submapper == 1) && (~m468.reg[0] & 0x04)) ? 0x1F : 0x0F);
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg16(0x8000, latch.data & prgMask | prgBase >> 1 & ~prgMask);
	setprg16(0xC000, prgBase >> 1 | prgMask);
	setchr8(0);
	setmirror(m468.reg[0] & 0x04 ? MI_H : MI_V);
}

static void Sync_UNROM512(void) {
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg16(0x8000, (latch.data & 0x1F) | ((prgBase >> 1) & ~0x1F));
	setprg16(0xC000, (prgBase >> 1) | 0x1F);
	setchr8(latch.data >> 5);
	setmirror((m468.reg[0] & 0x04) ? MI_H : MI_V);
}

static void SetPRG_VRC1(uint16_t A, uint16_t V) {
	uint16_t prgMask = m468.reg[0] & 0x08 ? (m468.reg[0] & 0x04 ? (m468.reg[0] & 0x02 ? 0x0F : 0x1F) : 0x3F) : 0x7F;
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC1(uint16_t A, uint16_t V) {
	setchr4(A, V & 0x1F);
}

static void Sync_VRC1(void) {
	VRC1_SyncPRG();
	VRC1_SyncCHR();
	VRC1_SyncMirror();
}

static void SetPRG_VRC24(uint16_t A, uint16_t V) {
	uint16_t prgMask = m468.reg[0] & 0x02 ? 0x0F : 0x1F;
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC24(uint16_t A, uint16_t V) {
	setchr1(A, V & 0xFF);
}

static void Sync_VRC24(void) {
	VRC24_pwrap = SetPRG_VRC24;
	VRC24_cwrap = SetCHR_VRC24;
	VRC24_SyncPRG();
	VRC24_SyncCHR();
	VRC24_SyncMirror();
	Mapper_SyncWRAM();
}

static void SetPRG_VRC3(uint16_t A, uint16_t V) {
	uint16_t prgMask = 0x07;
	uint16_t prgBase = Mapper_GetPRGBase() >> 1;
	setprg16(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC3(uint16_t V) {
	setchr8(V & 0x01);
}

static void Sync_VRC3(void) {
	VRC3_pwrap = SetPRG_VRC3;
	VRC3_cwrap = SetCHR_VRC3;
	VRC3_SyncPRG();
	VRC3_SyncCHR();
	setmirror((m468.reg[0] & 0x04) ? MI_H : MI_V);
	Mapper_SyncWRAM();
}

static void SetPRG_VRC6(uint16_t A, uint16_t V) {
	uint16_t prgMask = (m468.reg[0] & 0x02) ? 0x0F : 0x1F;
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC6(uint16_t A, uint16_t V) {
	setchr1(A, V & 0xFF);
}

static void Sync_VRC6(void) {
	VRC6_pwrap = SetPRG_VRC6;
	VRC6_cwrap = SetCHR_VRC6;
	Mapper_SyncWRAM();
	VRC6_SyncPRG();
	VRC6_SyncCHR();
	VRC6_SyncMirror();
}

static void SetPRG_VRC7(uint16_t A, uint16_t V) {
	uint16_t prgMask = m468.reg[0] & 0x08 ? (m468.reg[0] & 0x04 ? (m468.reg[0] & 0x02 ? 0x0F : 0x1F) : 0x3F) : 0x7F;
	uint16_t prgBase = Mapper_GetPRGBase();
	setprg8(A, (prgBase & ~prgMask) | (V & prgMask));
}

static void SetCHR_VRC7(uint16_t A, uint16_t V) {
	uint16_t chrMask = (m468.reg[0] & 0x10) ? 0xFF : 0x7F;
	setchr1(A, V & chrMask);
}

static void Sync_VRC7(void) {
	VRC7_pwrap = SetPRG_VRC7;
	VRC7_cwrap = SetCHR_VRC7;
	VRC7_SyncPRG();
	VRC7_SyncCHR();
	VRC7_mwrap(vrc7.mirr);
	Mapper_SyncWRAM();
}

static void SetMode(uint8_t clear) {
	uint8_t previousMirroring;
	MapIRQHook = NULL;
	PPU_hook = NULL;
	GameHBIRQHook = NULL;
	SetReadHandler(0x5000, 0x5FFF, ReadReg);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x5000, 0x5FFF, WriteReg);
	SetWriteHandler(0x6000, 0xFFFF, CartBW);
	switch (iNESCart.submapper << 8 | m468.reg[0] >> 4) {
	case 0x000:
	case 0x302:
		mapperSync = Sync_SxROM;
		MMC1_pwrap = SetPRG_SxROM;
		MMC1_cwrap = SetCHR_SxROM;
		MMC1_SetConfig(clear, MMC1B);
		Mapper_SyncWRAM();
		break;
	case 0x001:
		mapperSync = Sync_SUROM;
		MMC1_pwrap = SetPRG_SUROM;
		MMC1_cwrap = SetCHR_SUROM;
		MMC1_SetConfig(clear, MMC1B);
		Mapper_SyncWRAM();
		break;
	case 0x004:
	case 0x006:
	case 0x104:
	case 0x106:
		if (m468.reg[0] & 0x08) {
			mapperSync = Sync_BxROM;
		} else {
			mapperSync = Sync_AxROM;
		}
		Latch_SetConfig(clear, Sync);
		break;
	case 0x005:
	case 0x105:
		mapperSync = Sync_Misc; /* NROM, CNROM, Fire Hawk */
		SetWriteHandler(0x8000, 0xFFFF, Misc_WriteReg);
		if (clear) {
			m468.Custom_reg[0] = m468.Custom_reg[1] = m468.Custom_reg[2] = m468.Custom_reg[3] = 0;
		}
		Sync();
		break;
	case 0x007:
		mapperSync = Sync_LF36; /* SMB2J */
		MapIRQHook = LF36_cpuCycle;
		SetWriteHandler(0x8000, 0xFFFF, LF36_WriteReg);
		if (clear) {
			m468.Custom_reg[0] = m468.Custom_reg[1] = m468.Custom_reg[2] = m468.Custom_reg[3] = 0;
		}
		Sync();
		break;
	case 0x008:
		mapperSync = Sync_FxROM;
		MMC4_pwrap = SetPRG_FxROM;
		MMC4_cwrap = SetCHR_FxROM;
		MMC4_SetConfig(clear);
		Mapper_SyncWRAM();
		break;
	case 0x009:
	case 0x107:
	case 0x307:
		if (m468.reg[0] & 0x08) {
			mapperSync = Sync_UxROM;
			Latch_SetConfig(clear, Sync);
		} else {
			mapperSync = Sync_IF12; /* Not Irem's actual IF-12 mapper, but something custom by BlazePro */
			SetWriteHandler(0x8000, 0xFFFF, IF12_WriteReg);
			if (clear) {
				m468.Custom_reg[0] = m468.Custom_reg[1] = m468.Custom_reg[2] = m468.Custom_reg[3] = 0;
			}
			Sync();
		}
		break;
	case 0x00A:
		mapperSync = Sync_PNROM;
		MMC2_pwrap = SetPRG_PNROM;
		MMC2_cwrap = SetCHR_PNROM;
		MMC2_SetConfig(clear);
		break;
	case 0x00B:
		mapperSync = Sync_UNROM512;
		Latch_SetConfig(clear, Sync);
		break;
	case 0x00C:
	case 0x00D:
	case 0x10C:
	case 0x10D:
		mapperSync = Sync_GNROM;
		Latch_SetConfig(clear, Sync);
		break;
	case 0x00E:
	case 0x10E:
		mapperSync = Sync_Nanjing;
		GameHBIRQHook = Nanjing_scanline;
		SetWriteHandler(0x5000, 0x53FF, Nanjing_WriteReg);
		if (clear) {
			m468.Custom_reg[0] = m468.Custom_reg[1] = m468.Custom_reg[2] = m468.Custom_reg[3] = 0;
		}
		Sync();
		break;
	case 0x100:
	case 0x101:
		mapperSync = Sync_TxROM;
		previousMirroring = mmc3.mirr;
		MMC3_pwrap = SetPRG_TxROM;
		MMC3_cwrap = SetCHR_TxROM;
		MMC3_SetConfig(clear, MMC3B);
		MMC3_Write(0xA000, previousMirroring);
		Mapper_SyncWRAM();
		break;
	case 0x102:
		mapperSync = Sync_TxSROM;
		MMC3_pwrap = SetPRG_TxSROM;
		MMC3_cwrap = SetCHR_TxSROM;
		MMC3_SyncMirror = SyncMirror_TxSROM;
		MMC3_SetConfig(clear, MMC3B);
		Mapper_SyncWRAM();
		break;
	case 0x200:
		mapperSync = Sync_VRC24;
		VRC24_pwrap = SetPRG_VRC24;
		VRC24_cwrap = SetCHR_VRC24;
		VRC2_SetConfig(clear, 0x05, 0x0A);
		Mapper_SyncWRAM();
		break;
	case 0x201:
		mapperSync = Sync_VRC24;
		VRC24_pwrap = SetPRG_VRC24;
		VRC24_cwrap = SetCHR_VRC24;
		VRC4_SetConfig(clear, 0x05, 0x0A, TRUE);
		Mapper_SyncWRAM();
		break;
	case 0x202:
		mapperSync = Sync_VRC24;
		VRC24_pwrap = SetPRG_VRC24;
		VRC24_cwrap = SetCHR_VRC24;
		VRC2_SetConfig(clear, 0X0A, 0x05);
		Mapper_SyncWRAM();
		break;
	case 0x203:
		mapperSync = Sync_VRC24;
		VRC24_pwrap = SetPRG_VRC24;
		VRC24_cwrap = SetCHR_VRC24;
		VRC4_SetConfig(clear, 0x0A, 0x05, TRUE);
		Mapper_SyncWRAM();
		break;
	case 0x300:
	case 0x301:
		mapperSync = Sync_VRC6;
		VRC6_pwrap = SetPRG_VRC6;
		VRC6_cwrap = SetCHR_VRC6;
		VRC6_SetConfig(clear, 0x01, 0x02);
		Mapper_SyncWRAM();
		break;
	case 0x400:
		mapperSync = Sync_VRC1;
		VRC1_pwrap = SetPRG_VRC1;
		VRC1_cwrap = SetCHR_VRC1;
		VRC1_SetConfig(clear);
		Mapper_SyncWRAM();
		break;
	case 0x401:
		mapperSync = Sync_VRC7;
		VRC7_pwrap = SetPRG_VRC7;
		VRC7_cwrap = SetCHR_VRC7;
		VRC7_SetConfig(clear, 0x18);
		Mapper_SyncWRAM();
		break;
	case 0x404:
		mapperSync = Sync_VRC3;
		VRC3_pwrap = SetPRG_VRC3;
		VRC3_cwrap = SetCHR_VRC3;
		VRC3_SetConfig(clear);
		Mapper_SyncWRAM();
		break;
	case 0x500:
		mapperSync = Sync_FME7;
		FME7_pwrap = SetPRG_FME7;
		FME7_cwrap = SetCHR_FME7;
		FME7_SetConfig(clear);
		Mapper_SyncWRAM();
		break;
	default:
		break;
	}
}

/* Supervisor */
static DECLFR(ReadReg) {
	switch (A) {
	case 0x5301:
	case 0x5601: return m468.output ? 0x80 : 0x00;
	default: return 0xFF;
	}
}

static DECLFW(WriteReg) {
	switch (A) {
	case 0x5301:
		if (iNESCart.submapper == 0) setPins(!!(V & 0x04), !!(V & 0x02), !!(V & 0x01));
		break;
	case 0x5601:
		if (iNESCart.submapper == 1) setPins(!!(V & 0x10), !!(V & 0x02), !!(V & 0x01));
		if (~m468.reg[3] & 0x80) {
			m468.reg[3] = V;
			Sync();
		}
		break;
	case 0x5700:
		m468.reg[A & 0x03] = V;
		SetMode(1);
		break;
	case 0x5701:
	case 0x5702:
		m468.reg[A & 0x03] = V;
		Sync();
		break;
	}
}

static void Power(void) {
	m468.reg[0] = 0x0F;
	m468.reg[1] = 0xFF;
	m468.reg[2] = (iNESCart.submapper == 1) ? 0x10 : 0x00;
	m468.reg[3] = 0x00;
	m468.eep_clock = m468.command = m468.output = 1;
	m468.command = m468.state = 0;
	SetMode(1);
}

static void Close(void) {
}

static void StateRestore(int version) {
	SetMode(0);
}

void Mapper468_Init(CartInfo *info) {
	size_t ws = info->iNES2 ? (info->PRGRamSize + info->PRGRamSaveSize) : 8192;
	FME7_Init(info, FALSE, FALSE);
	Latch_Init(info, Sync, NULL, FALSE, FALSE);
	MMC1_Init(info, MMC1B, FALSE, FALSE);
	MMC2_Init(info, FALSE, FALSE);
	MMC4_Init(info, FALSE, FALSE);
	MMC3_Init(info, MMC1B, FALSE, FALSE);
	VRC1_Init(info);
	VRC24_Init(info, VRC24_VRC4, 0x01, 0x02, FALSE, TRUE);
	VRC3_Init(info);
	VRC6_Init(info, 0x01, 0x02, FALSE);
	VRC7_Init(info, 0x18);
	info->Reset = Power;
	info->Power = Power;
	info->Close = Close;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, 0);
	WRAMSIZE = ws;
	if (WRAMSIZE) {
		WRAM = (uint8_t *)FCEU_malloc(WRAMSIZE);
		SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, TRUE);
		AddExState(WRAM, WRAMSIZE, 0, "WRAM");
		if (info->battery) {
			info->SaveGame[0] = WRAM;
			info->SaveGameLen[0] = WRAMSIZE;
		}
	}
}
