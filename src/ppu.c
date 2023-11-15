/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2003 Xodnizel
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

/*
* negExp - 24.02.26 - New PPU added from FCEUX
*/

#include        <string.h>
#include        <stdlib.h>

#include        "fceu-types.h"
#include        "x6502.h"
#include        "fceu.h"
#include        "ppu.h"
#include        "nsf.h"
#include        "sound.h"
#include        "general.h"
#include        "fceu-endian.h"
#include        "fceu-memory.h"

#include        "cart.h"
#include        "ines.h"
#include        "palette.h"
#include        "state.h"
#include        "video.h"
#include        "input.h"
#include        "gamegenie.h"

static void FetchSpriteData(void);
static void RefreshLine(int lastpixel);
static void RefreshSprites(void);
static void CopySprites(uint8 *target);

static void Fixit1(void);
static uint32 ppulut1[256];
static uint32 ppulut2[256];
static uint32 ppulut3[128];

static bool new_ppu_reset = FALSE;

static uint8 bitrevtbl[256];
static uint8 bitrev_initialized = FALSE;

static uint8 bitrevlut(int index) {
	if (!bitrev_initialized) {
		int bits = 8;
		int n = 1 << 8;

		int m = 1;
		int a = n >> 1;
		int j = 2;

		int i;

		bitrevtbl[0] = 0;
		bitrevtbl[1] = a;

		while (--bits) {
			m <<= 1;
			a >>= 1;
			for (i = 0; i < m; i++) {
				bitrevtbl[j++] = bitrevtbl[i] + a;
			}
		}
		bitrev_initialized = TRUE;
	}

	return bitrevtbl[index];
}

typedef struct PPUSTATUS {
	int32 sl;
	int32 cycle, end_cycle;
} PPUSTATUS;

typedef struct SPRITE_READ {
	int32 num;
	int32 count;
	int32 fetch;
	int32 found;
	int32 found_pos[8];
	int32 ret;
	int32 last;
	int32 mode;
} SPRITE_READ;

void spr_read_reset(SPRITE_READ *spr) {
	spr->num = spr->count = spr->fetch = spr->found = spr->ret = spr->last = spr->mode = 0;
	spr->found_pos[0] = spr->found_pos[1] = spr->found_pos[2] = spr->found_pos[3] = 0;
	spr->found_pos[4] = spr->found_pos[5] = spr->found_pos[6] = spr->found_pos[7] = 0;
}

void spr_read_start_scanline(SPRITE_READ *spr) {
	spr->num = 1;
	spr->found = 0;
	spr->fetch = 1;
	spr->count = 0;
	spr->last = 64;
	spr->mode = 0;
	spr->found_pos[0] = spr->found_pos[1] = spr->found_pos[2] = spr->found_pos[3] = 0;
	spr->found_pos[4] = spr->found_pos[5] = spr->found_pos[6] = spr->found_pos[7] = 0;
}

/* doesn't need to be savestated as it is just a reflection of the current position in the ppu loop */
PPUPHASE ppuphase;

/* this needs to be savestated since a game may be trying to read from this across vblanks */
SPRITE_READ spr_read;

/* definitely needs to be savestated */
uint8 idleSynch = 1;

/* uses the internal counters concept at http://nesdev.icequake.net/PPU%20addressing.txt */
typedef struct PPUREGS {
	/* normal clocked regs. as the game can interfere with these at any time, they need to be savestated */
	uint32 fv;	/* 3 */
	uint32 v;	/* 1 */
	uint32 h;	/* 1 */
	uint32 vt;	/* 5 */
	uint32 ht;	/* 5 */

	/* temp unlatched regs (need savestating, can be written to at any time) */
	uint32 _fv, _v, _h, _vt, _ht;

	/* other regs that need savestating */
	uint32 fh;	/* 3 (horz scroll) */
	uint32 s;	/* 1 ($2000 bit 4: "Background pattern table address (0: $0000; 1: $1000)") */

	/* other regs that don't need saving */
	uint32 par;	/* 8 (sort of a hack, just stored in here, but not managed by this system) */

	/* cached state data. these are always reset at the beginning of a frame and don't need saving */
	/* but just to be safe, we're gonna save it */
	PPUSTATUS status;
} PPUREGS;

PPUREGS ppur;

void newppu_regs_reset(PPUREGS *reg) {
	reg->fv = reg->v = reg->h = reg->vt = reg->ht = 0;
	reg->fh = reg->par = reg->s = 0;
	reg->_fv = reg->_v = reg->_h = reg->_vt = reg->_ht = 0;
	reg->status.cycle = 0;
	reg->status.end_cycle = 341;
	reg->status.sl = 241;
}

void newppu_regs_install_latches(PPUREGS *reg) {
	reg->fv = reg->_fv;
	reg->v = reg->_v;
	reg->h = reg->_h;
	reg->vt = reg->_vt;
	reg->ht = reg->_ht;
}

void newppu_regs_install_h_latches(PPUREGS *reg) {
	reg->ht = reg->_ht;
	reg->h = reg->_h;
}

void clear_latches(PPUREGS *reg) {
	reg->_fv = reg->_v = reg->_h = reg->_vt = reg->_ht = 0;
	reg->fh = 0;
}

void newppu_regs_increment_hsc(PPUREGS *reg) {
	/* The first one, the horizontal scroll counter, consists of 6 bits, and is
	   made up by daisy-chaining the HT counter to the H counter. The HT counter
	   is then clocked every 8 pixel dot clocks (or every 8/3 CPU clock cycles). */
	reg->ht++;
	reg->h += (reg->ht >> 5);
	reg->ht &= 31;
	reg->h &= 1;
}

void newppu_regs_increment_vs(PPUREGS *reg) {
	int fv_overflow;

	reg->fv++;
	fv_overflow = (reg->fv >> 3);
	reg->vt += fv_overflow;
	reg->vt &= 31; /* fixed tecmo super bowl */
	if (reg->vt == 30 && fv_overflow == 1) { /* caution here (only do it at the exact instant of overflow) fixes p'radikus conflict */
		reg->v++;
		reg->vt = 0;
	}
	reg->fv &= 7;
	reg->v &= 1;
}

uint32 newppu_regs_get_ntread(PPUREGS *reg) {
	return 0x2000 | (reg->v << 0xB) | (reg->h << 0xA) | (reg->vt << 5) | reg->ht;
}

uint32 newppu_regs_get_2007access(PPUREGS *reg) {
	return ((reg->fv & 3) << 0xC) | (reg->v << 0xB) | (reg->h << 0xA) | (reg->vt << 5) | reg->ht;
}

/* The PPU has an internal 4-position, 2-bit shifter, which it uses for
 * obtaining the 2-bit palette select data during an attribute table byte
 * fetch. To represent how this data is shifted in the diagram, letters a..c
 * are used in the diagram to represent the right-shift position amount to
 * apply to the data read from the attribute data (a is always 0). This is why
 * you only see bits 0 and 1 used off the read attribute data in the diagram.
 */
uint32 newppu_regs_get_atread(PPUREGS *reg) {
	return 0x2000 | (reg->v << 0xB) | (reg->h << 0xA) | 0x3C0 | ((reg->vt & 0x1C) << 1) |
	       ((reg->ht & 0x1C) >> 2);
}

/* address line 3 relates to the pattern table fetch occuring (the PPU always makes them in pairs). */
uint32 newppu_regs_get_ptread(PPUREGS *reg) {
	return (reg->s << 0xC) | (reg->par << 0x4) | reg->fv;
}

void newppu_regs_increment2007(PPUREGS *reg, bool rendering, bool by32) {
	if (rendering) {
		/* don't do this:
		 * if (by32) increment_vs();
		 * else increment_hsc();
		 * do this instead:
		 */
		newppu_regs_increment_vs(reg); /* yes, even if we're moving by 32 */
		return;
	}

	/* If the VRAM address increment bit (2000.2) is clear (inc. amt. = 1), all
	 * the scroll counters are daisy-chained (in the order of HT, VT, H, V, FV)
	 * so that the carry out of each counter controls the next counter's clock
	 * rate. The result is that all 5 counters function as a single 15-bit one.
	 * Any access to 2007 clocks the HT counter here.
	 *
	 * If the VRAM address increment bit is set (inc. amt. = 32), the only
	 * difference is that the HT counter is no longer being clocked, and the VT
	 * counter is now being clocked by access to 2007.
	 */
	if (by32) {
		reg->vt++;
	} else {
		reg->ht++;
		reg->vt += (reg->ht >> 5) & 1;
	}
	reg->h += (reg->vt >> 5);
	reg->v += (reg->h >> 1);
	reg->fv += (reg->v >> 1);
	reg->ht &= 31;
	reg->vt &= 31;
	reg->h &= 1;
	reg->v &= 1;
	reg->fv &= 7;
}

#if 0
void newppu_regs_debug_log(PPUREGS *reg) {
	FCEU_printf(
	    "ppur: fv(%d), v(%d), h(%d), vt(%d), ht(%d)\n", fv, v, h, vt, ht);
	FCEU_printf("      _fv(%d), _v(%d), _h(%d), _vt(%d), _ht(%d)\n", _fv, _v,
	    _h, _vt, _ht);
	FCEU_printf("      fh(%d), s(%d), par(%d)\n", fh, s, par);
	FCEU_printf("      .status cycle(%d), end_cycle(%d), sl(%d)\n",
	    status.cycle, status.end_cycle, status.sl);
}
#endif

int newppu_get_scanline(void) {
	return ppur.status.sl;
}

int newppu_get_dot(void) {
	return ppur.status.cycle;
}

void newppu_hacky_emergency_reset(void) {
	if (ppur.status.end_cycle == 0) {
		newppu_regs_reset(&ppur);
	}
}

static void makeppulut(void) {
	int x;
	int y;
	int cc, xo, pixel;


	for (x = 0; x < 256; x++) {
		ppulut1[x] = 0;
		for (y = 0; y < 8; y++)
			ppulut1[x] |= ((x >> (7 - y)) & 1) << (y * 4);
		ppulut2[x] = ppulut1[x] << 1;
	}

	for (cc = 0; cc < 16; cc++) {
		for (xo = 0; xo < 8; xo++) {
			ppulut3[xo | (cc << 3)] = 0;
			for (pixel = 0; pixel < 8; pixel++) {
				int shiftr;
				shiftr = (pixel + xo) / 8;
				shiftr *= 2;
				ppulut3[xo | (cc << 3)] |= ((cc >> shiftr) & 3) << (2 + pixel * 4);
			}
		}
	}
}

static uint8 ppudead = 1;
static uint8 kook = 0;
int fceuindbg = 0;
int paldeemphswap;

uint8 gNoBGFillColor = 0xFF;

uint8 MMC5Hack = FALSE;
uint32 MMC5HackVROMMask = 0;
uint8 *MMC5HackExNTARAMPtr = 0;
uint8 *MMC5HackVROMPTR = 0;
uint8 MMC5HackCHRMode = 0;
uint8 MMC5HackSPMode = 0;
uint8 MMC50x5130 = 0;
uint8 MMC5HackSPScroll = 0;
uint8 MMC5HackSPPage = 0;

uint8 PEC586Hack = 0;

uint8 QTAIHack = FALSE;
uint8 qtaintramreg = 0;
uint8 QTAINTRAM[0x800];

uint8 VRAMBuffer = 0, PPUGenLatch = 0;
uint8 *vnapage[4];
uint8 PPUNTARAM = 0;
uint8 PPUCHRRAM = 0;

/* Color deemphasis emulation.  Joy... */
static uint8 deemp = 0;
static int deempcnt[8];

void (*GameHBIRQHook)(void), (*GameHBIRQHook2)(void);
void (*PPU_hook)(uint32 A);

uint8 vtoggle = 0;
uint8 XOffset = 0;

uint32 TempAddr = 0, RefreshAddr = 0, NTRefreshAddr = 0;

static int maxsprites = 8;

/* scanline is equal to the current visible scanline we're on. */
int scanline;
int g_rasterpos;
static uint32 scanlines_per_frame;
PPU_T ppu;

uint8 PPU[4];
uint8 PPUSPL;
uint8 NTARAM[0x1000], PALRAM[0x20], SPRAM[0x100], SPRBUF[0x100];
uint8 UPALRAM[0x03];/* for 0x4/0x8/0xC addresses in palette, the ones in
					 * 0x20 are 0 to not break fceu rendering.
					 */
uint8 READPAL_MOTHEROFALL(uint32 A)
{
	if(!(A & 3)) {
		if(!(A & 0xC))
			return READPAL(0x00);
		else
			return READUPAL(((A & 0xC) >> 2) - 1);
	}
	else
		return READPAL(A & 0x1F);
}

/* this duplicates logic which is embedded in the ppu rendering code
 * which figures out where to get CHR data from depending on various hack modes
 * mostly involving mmc5.
 * this might be incomplete.
 */
uint8* FCEUPPU_GetCHR(uint32 vadr, uint32 refreshaddr) {
	if (MMC5Hack) {
		if (MMC5HackCHRMode == 1) {
			uint8 *C = MMC5HackVROMPTR;
			C += (((MMC5HackExNTARAMPtr[refreshaddr & 0x3ff]) & 0x3f & MMC5HackVROMMask) << 12) + (vadr & 0xfff);
			C += (MMC50x5130 & 0x3) << 18;	/* 11-jun-2009 for kuja_killer */
			return C;
		} else {
			return MMC5BGVRAMADR(vadr);
		}
	} else return VRAMADR(vadr);
}

#define RENDER_LOG(tmp)
#define RENDER_LOGP(tmp)

/* likewise for ATTR */
int FCEUPPU_GetAttr(int ntnum, int xt, int yt) {
	int attraddr = 0x3C0 + ((yt >> 2) << 3) + (xt >> 2);
	int temp = (((yt & 2) << 1) + (xt & 2));
	int refreshaddr = xt + yt * 32;
	if (MMC5Hack && MMC5HackCHRMode == 1)
		return (MMC5HackExNTARAMPtr[refreshaddr & 0x3ff] & 0xC0) >> 6;
	else
		return (vnapage[ntnum][attraddr] & (3 << temp)) >> temp;
}

/* new ppu----- */
INLINE void FFCEUX_PPUWrite_Default(uint32 A, uint8 V) {
	uint32 tmp = A;

	if (PPU_hook) PPU_hook(A);

	if (tmp < 0x2000) {
		if (PPUCHRRAM & (1 << (tmp >> 10)))
			VPage[tmp >> 10][tmp] = V;
	} else if (tmp < 0x3F00) {
		if (QTAIHack && (qtaintramreg & 1)) {
			QTAINTRAM[((((tmp & 0xF00) >> 10) >> ((qtaintramreg >> 1)) & 1) << 10) | (tmp & 0x3FF)] = V;
		} else {
			if (PPUNTARAM & (1 << ((tmp & 0xF00) >> 10)))
				vnapage[((tmp & 0xF00) >> 10)][tmp & 0x3FF] = V;
		}
	} else {
		if (!(tmp & 3)) {
			if (!(tmp & 0xC)) {
				PALRAM[0x00] = PALRAM[0x04] = PALRAM[0x08] = PALRAM[0x0C] = V & 0x3F;
				PALRAM[0x10] = PALRAM[0x14] = PALRAM[0x18] = PALRAM[0x1C] = V & 0x3F;
			}
			else
				UPALRAM[((tmp & 0xC) >> 2) - 1] = V & 0x3F;
		} else
			PALRAM[tmp & 0x1F] = V & 0x3F;
	}
}

uint8 FFCEUX_PPURead_Default(uint32 A) {
	uint32 tmp = A;

	if (PPU_hook) PPU_hook(A);

	if (tmp < 0x2000) {
		return VPage[tmp >> 10][tmp];
	} else if (tmp < 0x3F00) {
		return vnapage[(tmp >> 10) & 0x3][tmp & 0x3FF];
	} else {
		uint8 ret;
		if (!(tmp & 3)) {
			if (!(tmp & 0xC))
				ret = READPAL(0x00);
			else
				ret = READUPAL(((tmp & 0xC) >> 2) - 1);
		} else
			ret = READPAL(tmp & 0x1F);
		return ret;
	}
}

uint8 (*FFCEUX_PPURead)(uint32 A) = 0;
void (*FFCEUX_PPUWrite)(uint32 A, uint8 V) = 0;

#define CALL_PPUREAD(A) (FFCEUX_PPURead(A))

#define CALL_PPUWRITE(A, V) (FFCEUX_PPUWrite ? FFCEUX_PPUWrite(A, V) : FFCEUX_PPUWrite_Default(A, V))

int newppu = 0;

/* --------------- */

static DECLFR(A2002) {
	uint8 ret;

	if (newppu) {
		/* once we thought we clear latches here, but that caused midframe glitches.
		 * i think we should only reset the state machine for 2005/2006
		 */
		/* ppur.clear_latches(); */
	}

	FCEUPPU_LineUpdate();
	ret = PPU_status;
	ret |= PPUGenLatch & 0x1F;

#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
#endif
	{
		vtoggle = 0;
		PPU_status &= 0x7F;
		PPUGenLatch = ret;
	}

	return ret;
}

#define GETLASTPIXEL (isPAL ? ((timestamp * 48 - linestartts) / 15) : ((timestamp * 48 - linestartts) >> 4))

static uint8 *Pline, *Plinef;
static int firsttile;
static int linestartts;

static DECLFR(A2004) {
	int i;
	if (newppu) {
		if ((ppur.status.sl < 241) && PPUON) {
			/* from cycles 0 to 63, the
			 * 32 byte OAM buffer gets init
			 * to 0xFF
			 */
			if (ppur.status.cycle < 64)
				return spr_read.ret = 0xFF;
			else {
				for (i = spr_read.last;
					 i != ppur.status.cycle; ++i) {
					if (i < 256) {
						switch (spr_read.mode) {
						case 0:
							if (spr_read.count < 2)
								spr_read.ret = (PPU[3] & 0xF8) + (spr_read.count << 2);
							else
								spr_read.ret = spr_read.count << 2;

							spr_read.found_pos[spr_read.found] = spr_read.ret;
							spr_read.ret = SPRAM[spr_read.ret];

							if (i & 1) {
								/* odd cycle, see if in range */
								if (!((ppur.status.sl - 1 - spr_read.ret) & ~(Sprite16 ? 0xF : 0x7))) {
									++spr_read.found;
									spr_read.fetch = 1;
									spr_read.mode = 1;
								} else {
									if (++spr_read.count == 64) {
										spr_read.mode = 4;
										spr_read.count = 0;
									} else if (spr_read.found == 8) {
										spr_read.fetch = 0;
										spr_read.mode = 2;
									}
								}
							}
							break;
						case 1: /* sprite is in range fetch next 3 bytes */
							if (i & 1) {
								++spr_read.fetch;
								if (spr_read.fetch == 4) {
									spr_read.fetch = 1;
									if (++spr_read.count == 64) {
										spr_read.count = 0;
										spr_read.mode = 4;
									} else if (spr_read.found == 8) {
										spr_read.fetch = 0;
										spr_read.mode = 2;
									} else
										spr_read.mode = 0;
								}
							}

							if (spr_read.count < 2)
								spr_read.ret = (PPU[3] & 0xF8) + (spr_read.count << 2);
							else
								spr_read.ret = spr_read.count << 2;

							spr_read.ret = SPRAM[spr_read.ret | spr_read.fetch];
							break;
						case 2: /* 8th sprite fetched */
							spr_read.ret = SPRAM[(spr_read.count << 2) | spr_read.fetch];
							if (i & 1) {
								if (!((ppur.status.sl - 1 - SPRAM[((spr_read.count << 2) | spr_read.fetch)]) & ~((Sprite16) ? 0xF : 0x7))) {
									spr_read.fetch = 1;
									spr_read.mode = 3;
								} else {
									if (++spr_read.count == 64) {
										spr_read.count = 0;
										spr_read.mode = 4;
									}
									spr_read.fetch =
										(spr_read.fetch + 1) & 3;
								}
							}
							spr_read.ret = spr_read.count;
							break;
						case 3:	/* 9th sprite overflow detected */
							spr_read.ret = SPRAM[spr_read.count | spr_read.fetch];
							if (i & 1) {
								if (++spr_read.fetch == 4) {
									spr_read.count = (spr_read.count + 1) & 63;
									spr_read.mode = 4;
								}
							}
							break;
						case 4:	/* read OAM[n][0] until hblank */
							if (i & 1)
								spr_read.count = (spr_read.count + 1) & 63;
							spr_read.fetch = 0;
							spr_read.ret = SPRAM[spr_read.count << 2];
							break;
						}
					} else if (i < 320) {
						spr_read.ret = (i & 0x38) >> 3;
						if (spr_read.found < (spr_read.ret + 1)) {
							if (spr_read.num) {
								spr_read.ret = SPRAM[252];
								spr_read.num = 0;
							} else
								spr_read.ret = 0xFF;
						} else if ((i & 7) < 4) {
							spr_read.ret =
								SPRAM[spr_read.found_pos[spr_read.ret] | spr_read.fetch++];
							if (spr_read.fetch == 4)
								spr_read.fetch = 0;
						} else
							spr_read.ret = SPRAM[spr_read.found_pos [spr_read.ret | 3]];
					} else {
						if (!spr_read.found)
							spr_read.ret = SPRAM[252];
						else
							spr_read.ret = SPRAM[spr_read.found_pos[0]];
						break;
					}
				}
				spr_read.last = ppur.status.cycle;
				return spr_read.ret;
			}
		} else
			return SPRAM[PPU[3]];
	} else {
		if (Pline) {
			int l = GETLASTPIXEL;

			if (l > 320 && l < 340) {
				return 0;
			}
			return 0xFF;
		} else {
			return SPRAM[PPU[3]];
		}
	}
}

static DECLFR(A200x) {	/* Not correct for $2004 reads. */
	FCEUPPU_LineUpdate();
	return PPUGenLatch;
}

static DECLFR(A2007) {
	uint8 ret;
	uint32 tmp = RefreshAddr & 0x3FFF;

	if (newppu) {
		ret = VRAMBuffer;
		RefreshAddr = newppu_regs_get_2007access(&ppur) & 0x3FFF;
		if ((RefreshAddr & 0x3F00) == 0x3F00) {
			/* if it is in the palette range bypass the
			 * delayed read, and what gets filled in the temp
			 * buffer is the address - 0x1000, also
			 * if grayscale is set then the return is AND with 0x30
			 * to get a gray color reading
			 */
			if (!(tmp & 3)) {
				if (!(tmp & 0xC))
					ret = READPAL(0x00);
				else
					ret = READUPAL(((tmp & 0xC) >> 2) - 1);
			} else
				ret = READPAL(tmp & 0x1F);
			VRAMBuffer = CALL_PPUREAD(RefreshAddr - 0x1000);
		} else {
			/* if (debug_loggingCD && (RefreshAddr < 0x2000))
				LogAddress = GetCHRAddress(RefreshAddr); */
			VRAMBuffer = CALL_PPUREAD(RefreshAddr);
		}
		newppu_regs_increment2007(&ppur, ppur.status.sl >= 0 && ppur.status.sl < 241 && PPUON, INC32 != 0);
		RefreshAddr = newppu_regs_get_2007access(&ppur);
		return ret;
	}

	/* OLD PPU */
	FCEUPPU_LineUpdate();

	if (tmp >= 0x3F00) {	/* Palette RAM tied directly to the output data, without VRAM buffer */
		if (!(tmp & 3)) {
			if (!(tmp & 0xC))
				ret = READPAL(0x00);
			else
				ret = READUPAL(((tmp & 0xC) >> 2) - 1);
		} else
			ret = READPAL(tmp & 0x1F);
		#ifdef FCEUDEF_DEBUGGER
		if (!fceuindbg)
		#endif
		{
			if ((tmp - 0x1000) < 0x2000)
				VRAMBuffer = VPage[(tmp - 0x1000) >> 10][tmp - 0x1000];
			else
				VRAMBuffer = vnapage[((tmp - 0x1000) >> 10) & 0x3][(tmp - 0x1000) & 0x3FF];
			if (PPU_hook) PPU_hook(tmp);
		}
	} else {
		ret = VRAMBuffer;
		#ifdef FCEUDEF_DEBUGGER
		if (!fceuindbg)
		#endif
		{
			if (PPU_hook) PPU_hook(tmp);
			PPUGenLatch = VRAMBuffer;
			if (tmp < 0x2000) {
				if (MMC5Hack && newppu)
					VRAMBuffer = *MMC5BGVRAMADR(tmp);
				else
					VRAMBuffer = VPage[tmp >> 10][tmp];
			} else if (tmp < 0x3F00)
				VRAMBuffer = vnapage[(tmp >> 10) & 0x3][tmp & 0x3FF];
		}
	}

	#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
	#endif
	{
		if ((ScreenON || SpriteON) && (scanline < 240)) {
			uint32 rad = RefreshAddr;
			if ((rad & 0x7000) == 0x7000) {
				rad ^= 0x7000;
				if ((rad & 0x3E0) == 0x3A0)
					rad ^= 0xBA0;
				else if ((rad & 0x3E0) == 0x3e0)
					rad ^= 0x3e0;
				else
					rad += 0x20;
			} else
				rad += 0x1000;
			RefreshAddr = rad;
		} else {
			if (INC32)
				RefreshAddr += 32;
			else
				RefreshAddr++;
		}
		if (PPU_hook) PPU_hook(RefreshAddr & 0x3fff);
	}
	return ret;
}

static DECLFW(B2000) {
	FCEUPPU_LineUpdate();
	PPUGenLatch = V;

	if (!(PPU[0] & 0x80) && (V & 0x80) && (PPU_status & 0x80))
		TriggerNMI2();

	PPU[0] = V;
	TempAddr &= 0xF3FF;
	TempAddr |= (V & 3) << 10;

	ppur._h = V & 1;
	ppur._v = (V >> 1) & 1;
	ppur.s = (V >> 4) & 1;
}

static DECLFW(B2001) {
	FCEUPPU_LineUpdate();
	if (paldeemphswap)
		V = (V&0x9F)|((V&0x40)>>1)|((V&0x20)<<1);
	if (FSettings.DisableEmphasis)
		V &= ~0xE0;
	PPUGenLatch = V;
	PPU[1] = V;
	if (V & 0xE0)
		deemp = V >> 5;
}

static DECLFW(B2002) {
	PPUGenLatch = V;
}

static DECLFW(B2003) {
	PPUGenLatch = V;
	PPU[3] = V;
	PPUSPL = V & 0x7;
}

static DECLFW(B2004) {
	PPUGenLatch = V;
	if (newppu) {
		/* the attribute upper bits are not connected
		 * so AND them out on write, since reading them
		 * should return 0 in those bits.
		 */
		if ((PPU[3] & 3) == 2)
			V &= 0xE3;
		SPRAM[PPU[3]] = V;
		PPU[3] = (PPU[3] + 1) & 0xFF;
	} else {
		if (PPUSPL >= 8) {
			if (PPU[3] >= 8)
				SPRAM[PPU[3]] = V;
		} else {
			SPRAM[PPUSPL] = V;
		}
		PPU[3]++;
		PPUSPL++;
	}
}

static DECLFW(B2005) {
	uint32 tmp = TempAddr;
	FCEUPPU_LineUpdate();
	PPUGenLatch = V;
	if (!vtoggle) {
		tmp &= 0xFFE0;
		tmp |= V >> 3;
		XOffset = V & 7;

		ppur._ht = V >> 3;
		ppur.fh = V & 7;
	} else {
		tmp &= 0x8C1F;
		tmp |= ((V & ~0x7) << 2);
		tmp |= (V & 7) << 12;

		ppur._vt = V >> 3;
		ppur._fv = V & 7;
	}
	TempAddr = tmp;
	vtoggle ^= 1;
}


static DECLFW(B2006) {
	FCEUPPU_LineUpdate();

	PPUGenLatch = V;
	if (!vtoggle) {
		TempAddr &= 0x00FF;
		TempAddr |= (V & 0x3f) << 8;

		ppur._vt &= 0x07;
		ppur._vt |= (V & 0x3) << 3;
		ppur._h = (V >> 2) & 1;
		ppur._v = (V >> 3) & 1;
		ppur._fv = (V >> 4) & 3;
	} else {
		TempAddr &= 0xFF00;
		TempAddr |= V;

		RefreshAddr = TempAddr;
		if (PPU_hook)
			PPU_hook(RefreshAddr);

		ppur._vt &= 0x18;
		ppur._vt |= (V >> 5);
		ppur._ht = V & 31;

		newppu_regs_install_latches(&ppur);
	}
	vtoggle ^= 1;
}

static DECLFW(B2007) {
	uint32 tmp = RefreshAddr & 0x3FFF;

	if (newppu) {
		PPUGenLatch = V;
		RefreshAddr = newppu_regs_get_2007access(&ppur) & 0x3FFF;
		CALL_PPUWRITE(RefreshAddr, V);
		newppu_regs_increment2007(&ppur, ppur.status.sl >= 0 && ppur.status.sl < 241 && PPUON, INC32 != 0);
		RefreshAddr = newppu_regs_get_2007access(&ppur);
		return;
	}

	/* OLD PPU */
	PPUGenLatch = V;
	if (tmp < 0x2000) {
		if (PPUCHRRAM & (1 << (tmp >> 10)))
			VPage[tmp >> 10][tmp] = V;
	} else if (tmp < 0x3F00) {
		if (QTAIHack && (qtaintramreg & 1)) {
			QTAINTRAM[((((tmp & 0xF00) >> 10) >> ((qtaintramreg >> 1)) & 1) << 10) | (tmp & 0x3FF)] = V;
		} else {
			if (PPUNTARAM & (1 << ((tmp & 0xF00) >> 10)))
				vnapage[((tmp & 0xF00) >> 10)][tmp & 0x3FF] = V;
		}
	} else {
		if (!(tmp & 3)) {
			if (!(tmp & 0xC))
				PALRAM[0x00] = PALRAM[0x04] = PALRAM[0x08] = PALRAM[0x0C] = V & 0x3F;
			else
				UPALRAM[((tmp & 0xC) >> 2) - 1] = V & 0x3F;
		} else
			PALRAM[tmp & 0x1F] = V & 0x3F;
	}
	if (INC32)
		RefreshAddr += 32;
	else
		RefreshAddr++;
	if (PPU_hook)
		PPU_hook(RefreshAddr & 0x3fff);
}

static DECLFW(B4014) {
	uint32 t = V << 8;
	int x;

	for (x = 0; x < 256; x++)
		X6502_DMW(0x2004, X6502_DMR(t + x));
}

#define GETLASTPIXEL (isPAL ? ((timestamp * 48 - linestartts) / 15) : ((timestamp * 48 - linestartts) >> 4))

static uint8 *Pline, *Plinef;
static int firsttile;
static int linestartts;
static int tofix = 0;

static void ResetRL(uint8 *target) {
	memset(target, 0xFF, 256);
	InputScanlineHook(0, 0, 0, 0);
	Plinef = target;
	Pline = target;
	firsttile = 0;
	linestartts = timestamp * 48 + cpu.count;
	tofix = 0;
	FCEUPPU_LineUpdate();
	tofix = 1;
}

static uint8 sprlinebuf[256 + 8];

void FCEUPPU_LineUpdate(void) {
	if (newppu) {
		return;
	}
#ifdef FCEUDEF_DEBUGGER
	if (!fceuindbg)
#endif
	if (Pline) {
		int l = GETLASTPIXEL;
		RefreshLine(l);
	}
}

static int show_sprites = TRUE;
static int show_background = TRUE;

void FCEUI_SetRenderPlanes(int sprites, int bg) {
	show_sprites = sprites;
	show_background = bg;
}

void FCEUI_GetRenderPlanes(int *sprites, int *bg) {
	*sprites = show_sprites;
	*bg = show_background;
}

static void CheckSpriteHit(int p);

static void EndRL(void) {
	RefreshLine(272);
	if (tofix)
		Fixit1();
	CheckSpriteHit(272);
	Pline = 0;
}

static int32 sphitx;
static uint8 sphitdata;

static void CheckSpriteHit(int p) {
	int l = p - 16;
	int x;

	if (sphitx == 0x100) return;

	for (x = sphitx; x < (sphitx + 8) && x < l; x++) {
		if ((sphitdata & (0x80 >> (x - sphitx))) && !(Plinef[x] & 64) && x < 255) {
			PPU_status |= 0x40;
			sphitx = 0x100;
			break;
		}
	}
}

/* spork the world.  Any sprites on this line? Then this will be set to 1.
 * Needed for zapper emulation and *gasp* sprite emulation.
 */
static int spork = 0;

/* lasttile is really "second to last tile." */
static void RefreshLine(int lastpixel) {
	static uint32 pshift[2];
	static uint32 atlatch;
	uint32 smorkus = RefreshAddr;

	#define RefreshAddr smorkus
	uint32 vofs;
	int X1;

	uint8 *P = Pline;
	int lasttile = lastpixel >> 3;
	int numtiles;
	static int norecurse = 0;	/* Yeah, recursion would be bad.
								 * PPU_hook() functions can call
								 * mirroring/chr bank switching functions,
								 * which call FCEUPPU_LineUpdate, which call this
								 * function.
								 */
	if (norecurse) return;

	if (sphitx != 0x100 && !Sprite0Hit) {
		if ((sphitx < (lastpixel - 16)) && !(sphitx < ((lasttile - 2) * 8)))
			lasttile++;
	}

	if (lasttile > 34) lasttile = 34;
	numtiles = lasttile - firsttile;

	if (numtiles <= 0) return;

	P = Pline;

	vofs = 0;

	if(PEC586Hack)
		vofs = ((RefreshAddr & 0x200) << 3) | ((RefreshAddr >> 12) & 7);
	else
		vofs = ((PPU[0] & 0x10) << 8) | ((RefreshAddr >> 12) & 7);

	if (!ScreenON && !SpriteON) {
		uint32 tem;
		tem = READPAL(0) | (READPAL(0) << 8) | (READPAL(0) << 16) | (READPAL(0) << 24);
		tem |= 0x40404040;
		FCEU_dwmemset(Pline, tem, numtiles * 8);
		P += numtiles * 8;
		Pline = P;

		firsttile = lasttile;

		#define TOFIXNUM (272 - 0x4)
		if (lastpixel >= TOFIXNUM && tofix) {
			Fixit1();
			tofix = 0;
		}

		if ((lastpixel - 16) >= 0) {
			InputScanlineHook(Plinef, spork ? sprlinebuf : 0, linestartts, lasttile * 8 - 16);
		}
		return;
	}

	/* Priority bits, needed for sprite emulation. */
	PALRAM[0]   |= 64;
	PALRAM[4]   |= 64;
	PALRAM[8]   |= 64;
	PALRAM[0xC] |= 64;

	/* This high-level graphics MMC5 emulation code was written for MMC5 carts in "CL" mode.
	 * It's probably not totally correct for carts in "SL" mode.
	 */

#define PPUT_MMC5
	if (MMC5Hack && geniestage != 1) {
		if (MMC5HackCHRMode == 0 && (MMC5HackSPMode & 0x80)) {
			int tochange = MMC5HackSPMode & 0x1F;
			tochange -= firsttile;
			for (X1 = firsttile; X1 < lasttile; X1++) {
				if ((tochange <= 0 && MMC5HackSPMode & 0x40) || (tochange > 0 && !(MMC5HackSPMode & 0x40))) {
					#define PPUT_MMC5SP
					#include "pputile.h"
					#undef PPUT_MMC5SP
				} else {
					#include "pputile.h"
				}
				tochange--;
			}
		} else if (MMC5HackCHRMode == 1 && (MMC5HackSPMode & 0x80)) {
			#define PPUT_MMC5SP
			#define PPUT_MMC5CHR1
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPUT_MMC5CHR1
			#undef PPUT_MMC5SP
		} else if (MMC5HackCHRMode == 1) {
			#define PPUT_MMC5CHR1
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPUT_MMC5CHR1
		} else {
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
		}
	}
	#undef PPUT_MMC5
	else if (PPU_hook) {
		norecurse = 1;
		#define PPUT_HOOK
		if (PEC586Hack) {
			#define PPU_BGFETCH
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPU_BGFETCH
		} else {
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
		}
		#undef PPUT_HOOK
		norecurse = 0;
	} else {
		if (PEC586Hack) {
			#define PPU_BGFETCH
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPU_BGFETCH
		} else if (QTAIHack) {
			#define PPU_VRC5FETCH
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
			#undef PPU_VRC5FETCH
		} else {
			for (X1 = firsttile; X1 < lasttile; X1++) {
				#include "pputile.h"
			}
		}
	}

#undef vofs
#undef RefreshAddr

	/* Reverse changes made before. */
	PALRAM[0]   &= 63;
	PALRAM[4]   &= 63;
	PALRAM[8]   &= 63;
	PALRAM[0xC] &= 63;

	RefreshAddr = smorkus;
	if (firsttile <= 2 && 2 < lasttile && !BGLeft8ON) {
		uint32 tem;
		tem = READPAL(0) | (READPAL(0) << 8) | (READPAL(0) << 16) | (READPAL(0) << 24);
		tem |= 0x40404040;
		*(uint32*)Plinef = *(uint32*)(Plinef + 4) = tem;
	}

	if (!ScreenON) {
		uint32 tem;
		int tstart, tcount;
		tem = READPAL(0) | (READPAL(0) << 8) | (READPAL(0) << 16) | (READPAL(0) << 24);
		tem |= 0x40404040;

		tcount = lasttile - firsttile;
		tstart = firsttile - 2;
		if (tstart < 0) {
			tcount += tstart;
			tstart = 0;
		}
		if (tcount > 0)
			FCEU_dwmemset(Plinef + tstart * 8, tem, tcount * 8);
	}

	if (lastpixel >= TOFIXNUM && tofix) {
		Fixit1();
		tofix = 0;
	}

	/* This only works right because of a hack earlier in this function. */
	CheckSpriteHit(lastpixel);

	if ((lastpixel - 16) >= 0) {
		InputScanlineHook(Plinef, spork ? sprlinebuf : 0, linestartts, lasttile * 8 - 16);
	}
	Pline = P;
	firsttile = lasttile;
}

static INLINE void Fixit2(void) {
	if (ScreenON || SpriteON) {
		uint32 rad = RefreshAddr;
		rad &= 0xFBE0;
		rad |= TempAddr & 0x041f;
		RefreshAddr = rad;
	}
}

static void Fixit1(void) {
	if (ScreenON || SpriteON) {
		uint32 rad = RefreshAddr;

		if ((rad & 0x7000) == 0x7000) {
			rad ^= 0x7000;
			if ((rad & 0x3E0) == 0x3A0)
				rad ^= 0xBA0;
			else if ((rad & 0x3E0) == 0x3e0)
				rad ^= 0x3e0;
			else
				rad += 0x20;
		} else
			rad += 0x1000;
		RefreshAddr = rad;
	}
}

void MMC5_hb(int);		/* Ugh ugh ugh. */
static void DoLine(void)
{
	int x, colour_emphasis;
	uint8 *target = NULL;
	uint8 *dtarget = NULL;

	if (scanline >= 240 && scanline != ppu.totalscanlines)
	{
		X6502_Run(256 + 69);
		scanline++;
		X6502_Run(16);
		return;
	}

	target = XBuf + ((scanline < 240 ? scanline : 240) << 8);
	dtarget = XDBuf + ((scanline < 240 ? scanline : 240) << 8);

	if (MMC5Hack /* && (ScreenON || SpriteON) */) MMC5_hb(scanline);

	X6502_Run(256);
	EndRL();

	if (!show_background) {/* User asked to not display background data. */
		uint32 tem;
		tem = READPAL(0) | (READPAL(0) << 8) | (READPAL(0) << 16) | (READPAL(0) << 24);
		tem |= 0x40404040;
		FCEU_dwmemset(target, tem, 256);
	}

	if (SpriteON)
		CopySprites(target);

	if (ScreenON || SpriteON) {	/* Yes, very el-cheapo. */
		if (PPU[1] & 0x01) {
			for (x = 63; x >= 0; x--)
				*(uint32*)&target[x << 2] = (*(uint32*)&target[x << 2]) & 0x30303030;
		}
	}
	if ((PPU[1] >> 5) == 0x7) {
		for (x = 63; x >= 0; x--)
			*(uint32*)&target[x << 2] = ((*(uint32*)&target[x << 2]) & 0x3f3f3f3f) | 0xc0c0c0c0;
	} else if (PPU[1] & 0xE0)
		for (x = 63; x >= 0; x--)
			*(uint32*)&target[x << 2] = (*(uint32*)&target[x << 2]) | 0x40404040;
	else
		for (x = 63; x >= 0; x--)
			*(uint32*)&target[x << 2] = ((*(uint32*)&target[x << 2]) & 0x3f3f3f3f) | 0x80808080;

	/* write the actual colour emphasis */
	colour_emphasis = ((PPU[1] >> 5) << 24) | ((PPU[1] >> 5) << 16) | ((PPU[1] >> 5) << 8) | ((PPU[1] >> 5) << 0);
	for (x = 63; x >= 0; x--)
		*(uint32*)&dtarget[x << 2] = colour_emphasis;

    sphitx = 0x100;

	if (ScreenON || SpriteON)
		FetchSpriteData();

	if (GameHBIRQHook && (ScreenON || SpriteON) && ((PPU[0] & 0x38) != 0x18)) {
		X6502_Run(6);
		Fixit2();
		X6502_Run(4);
		GameHBIRQHook();
		X6502_Run(85 - 16 - 10);
	} else {
		X6502_Run(6);	/* Tried 65, caused problems with Slalom(maybe others) */
		Fixit2();
		X6502_Run(85 - 6 - 16);

		/* A semi-hack for Star Trek: 25th Anniversary */
		if (GameHBIRQHook && (ScreenON || SpriteON) && ((PPU[0] & 0x38) != 0x18))
			GameHBIRQHook();
	}

	if (SpriteON)
		RefreshSprites();
	if (GameHBIRQHook2 && (ScreenON || SpriteON))
		GameHBIRQHook2();
	scanline++;
	if (scanline < 240) {
		ResetRL(XBuf + (scanline << 8));
	}
	X6502_Run(16);
}

#define V_FLIP  0x80
#define H_FLIP  0x40
#define SP_BACK 0x20

typedef struct {
	uint8 y, no, atr, x;
} SPR;

typedef struct {
	uint8 ca[2], atr, x;
} SPRB;

void FCEUI_DisableSpriteLimitation(int a) {
	maxsprites = a ? 64 : 8;
}

static uint8 numsprites, SpriteBlurp;
static void FetchSpriteData(void) {
	uint8 ns, sb;
	SPR *spr;
	uint8 H;
	int n;
	int vofs;
	uint8 P0 = PPU[0];

	spr = (SPR*)SPRAM;
	H = 8;

	ns = sb = 0;

	vofs = (uint32)(P0 & 0x8 & (((P0 & 0x20) ^ 0x20) >> 2)) << 9;
	H += (P0 & 0x20) >> 2;

	if (!PPU_hook)
		for (n = 63; n >= 0; n--, spr++) {
			if ((uint32)(scanline - spr->y) >= H) continue;
			if (ns < maxsprites) {
				if (n == 63) sb = 1;

				{
					SPRB dst;
					uint8 *C;
					int t;
					uint32 vadr;

					t = (int)scanline - (spr->y);

					if (Sprite16)
						vadr = ((spr->no & 1) << 12) + ((spr->no & 0xFE) << 4);
					else
						vadr = (spr->no << 4) + vofs;

					if (spr->atr & V_FLIP) {
						vadr += 7;
						vadr -= t;
						vadr += (P0 & 0x20) >> 1;
						vadr -= t & 8;
					} else {
						vadr += t;
						vadr += t & 8;
					}

					/* Fix this geniestage hack */
					if (MMC5Hack && geniestage != 1)
						C = MMC5SPRVRAMADR(vadr);
					else
						C = VRAMADR(vadr);

					dst.ca[0] = C[0];
					dst.ca[1] = C[8];
					dst.x = spr->x;
					dst.atr = spr->atr;

					*(uint32*)&SPRBUF[ns << 2] = *(uint32*)&dst;
				}

				ns++;
			} else {
				PPU_status |= 0x20;
				break;
			}
		}
	else
		for (n = 63; n >= 0; n--, spr++) {
			if ((uint32)(scanline - spr->y) >= H) continue;

			if (ns < maxsprites) {
				if (n == 63) sb = 1;

				{
					SPRB dst;
					uint8 *C;
					int t;
					uint32 vadr;

					t = (int)scanline - (spr->y);

					if (Sprite16)
						vadr = ((spr->no & 1) << 12) + ((spr->no & 0xFE) << 4);
					else
						vadr = (spr->no << 4) + vofs;

					if (spr->atr & V_FLIP) {
						vadr += 7;
						vadr -= t;
						vadr += (P0 & 0x20) >> 1;
						vadr -= t & 8;
					} else {
						vadr += t;
						vadr += t & 8;
					}

					if (MMC5Hack)
						C = MMC5SPRVRAMADR(vadr);
					else
						C = VRAMADR(vadr);
					dst.ca[0] = C[0];
					if (ns < 8) {
						PPU_hook(0x2000);
						PPU_hook(vadr);
					}
					dst.ca[1] = C[8];
					dst.x = spr->x;
					dst.atr = spr->atr;


					*(uint32*)&SPRBUF[ns << 2] = *(uint32*)&dst;
				}

				ns++;
			} else {
				PPU_status |= 0x20;
				break;
			}
		}

	/* Handle case when >8 sprites per scanline option is enabled. */
	if (ns > 8) PPU_status |= 0x20;
	else if (PPU_hook) {
		for (n = 0; n < (8 - ns); n++) {
			PPU_hook(0x2000);
			PPU_hook(vofs);
		}
	}
	numsprites = ns;
	SpriteBlurp = sb;
}

static void RefreshSprites(void) {
	int n;
	SPRB *spr;

	spork = 0;
	if (!numsprites) return;

	FCEU_dwmemset(sprlinebuf, 0x80808080, 256);
	numsprites--;
	spr = (SPRB*)SPRBUF + numsprites;

	for (n = numsprites; n >= 0; n--, spr--) {
		uint32 pixdata;
		uint8 J, atr;

		int x = spr->x;
		uint8 *C;
		int VB;

		pixdata = ppulut1[spr->ca[0]] | ppulut2[spr->ca[1]];
		J = spr->ca[0] | spr->ca[1];
		atr = spr->atr;

		if (J) {
			if (n == 0 && SpriteBlurp && !Sprite0Hit) {
				sphitx = x;
				sphitdata = J;
				if (atr & H_FLIP)
					sphitdata = ((J << 7) & 0x80) |
								((J << 5) & 0x40) |
								((J << 3) & 0x20) |
								((J << 1) & 0x10) |
								((J >> 1) & 0x08) |
								((J >> 3) & 0x04) |
								((J >> 5) & 0x02) |
								((J >> 7) & 0x01);
			}

			C = sprlinebuf + x;
			VB = 0x10 + ((atr & 3) << 2);

			if (atr & SP_BACK) {
				if (atr & H_FLIP) {
					if (J & 0x80) C[7] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x40) C[6] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x20) C[5] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x10) C[4] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x08) C[3] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x04) C[2] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x02) C[1] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x01) C[0] = READPAL(VB | pixdata) | 0x40;
				} else {
					if (J & 0x80) C[0] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x40) C[1] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x20) C[2] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x10) C[3] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x08) C[4] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x04) C[5] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x02) C[6] = READPAL(VB | (pixdata & 3)) | 0x40;
					pixdata >>= 4;
					if (J & 0x01) C[7] = READPAL(VB | pixdata) | 0x40;
				}
			} else {
				if (atr & H_FLIP) {
					if (J & 0x80) C[7] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x40) C[6] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x20) C[5] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x10) C[4] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x08) C[3] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x04) C[2] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x02) C[1] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x01) C[0] = READPAL(VB | pixdata);
				} else {
					if (J & 0x80) C[0] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x40) C[1] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x20) C[2] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x10) C[3] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x08) C[4] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x04) C[5] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x02) C[6] = READPAL(VB | (pixdata & 3));
					pixdata >>= 4;
					if (J & 0x01) C[7] = READPAL(VB | pixdata);
				}
			}
		}
	}
	SpriteBlurp = 0;
	spork = 1;
}

static void CopySprites(uint8 *target) {
	uint8 n = SpriteLeft8ON ? 0 : 8;
	uint8 *P = target;

	if (!spork) return;
	spork = 0;

	if (!show_sprites) return;	/* User asked to not display sprites. */

	if(!SpriteON) return;

   do
	{
		uint32 t = *(uint32*)(sprlinebuf + n);

		if (t != 0x80808080) {
			#ifdef MSB_FIRST
			if (!(t & 0x80000000)) {
				if (!(t & 0x40000000) || (P[n] & 64))	/* Normal sprite || behind bg sprite */
					P[n] = sprlinebuf[n];
			}

			if (!(t & 0x800000)) {
				if (!(t & 0x400000) || (P[n + 1] & 64))	/* Normal sprite || behind bg sprite */
					P[n + 1] = (sprlinebuf + 1)[n];
			}

			if (!(t & 0x8000)) {
				if (!(t & 0x4000) || (P[n + 2] & 64))		/* Normal sprite || behind bg sprite */
					P[n + 2] = (sprlinebuf + 2)[n];
			}

			if (!(t & 0x80)) {
				if (!(t & 0x40) || (P[n + 3] & 64))		/* Normal sprite || behind bg sprite */
					P[n + 3] = (sprlinebuf + 3)[n];
			}
			#else

			if (!(t & 0x80)) {
				if (!(t & 0x40) || (P[n] & 0x40))		/* Normal sprite || behind bg sprite */
					P[n] = sprlinebuf[n];
			}

			if (!(t & 0x8000)) {
				if (!(t & 0x4000) || (P[n + 1] & 0x40))		/* Normal sprite || behind bg sprite */
					P[n + 1] = (sprlinebuf + 1)[n];
			}

			if (!(t & 0x800000)) {
				if (!(t & 0x400000) || (P[n + 2] & 0x40))	/* Normal sprite || behind bg sprite */
					P[n + 2] = (sprlinebuf + 2)[n];
			}

			if (!(t & 0x80000000)) {
				if (!(t & 0x40000000) || (P[n + 3] & 0x40))	/* Normal sprite || behind bg sprite */
					P[n + 3] = (sprlinebuf + 3)[n];
			}
			#endif
		}
      n += 4;
	} while(n);
}

void FCEUPPU_SetVideoSystem(int w) {
	if (w) {
		scanlines_per_frame = isDendy ? 262 : 312;
		FSettings.FirstSLine = FSettings.UsrFirstSLine[1];
		FSettings.LastSLine = FSettings.UsrLastSLine[1];
	} else {
		scanlines_per_frame = 262;
		FSettings.FirstSLine = FSettings.UsrFirstSLine[0];
		FSettings.LastSLine = FSettings.UsrLastSLine[0];
	}
}

void FCEUPPU_Init(void) {
	makeppulut();
}

void PPU_ResetHooks(void) {
	FFCEUX_PPURead = FFCEUX_PPURead_Default;
}

void FCEUPPU_Reset(void) {
	VRAMBuffer = PPU[0] = PPU[1] = PPU_status = PPU[3] = 0;
	PPUSPL = 0;
	PPUGenLatch = 0;
	RefreshAddr = TempAddr = 0;
	vtoggle = 0;
	ppudead = 2;
	kook = 0;
	idleSynch = 1;

	new_ppu_reset = TRUE; /* delay reset of ppur/spr_read until it's ready to start a new frame */
}

void FCEUPPU_Power(void) {
	int x;

	/* initialize PPU memory regions according to settings */
	FCEU_MemoryRand(NTARAM, 0x1000);
	FCEU_MemoryRand(PALRAM, 0x20);
	FCEU_MemoryRand(SPRAM, 0x100);

	/* palettes can only store values up to $3F, and PALRAM X4/X8/XC are mirrors of X0 for rendering purposes (UPALRAM is used for $2007 readback)*/
	for (x = 0; x < 0x20; ++x) PALRAM[x] &= 0x3F;
	UPALRAM[0] = PALRAM[0x04];
	UPALRAM[1] = PALRAM[0x08];
	UPALRAM[2] = PALRAM[0x0C];
	PALRAM[0x0C] = PALRAM[0x08] = PALRAM[0x04] = PALRAM[0x00];
	PALRAM[0x1C] = PALRAM[0x18] = PALRAM[0x14] = PALRAM[0x10];

	FCEUPPU_Reset();

	for (x = 0x2000; x < 0x4000; x += 8) {
		SetReadHandler(x + 0, x + 0, A200x);
		SetReadHandler(x + 1, x + 1, A200x);
		SetReadHandler(x + 2, x + 2, A2002);
		SetReadHandler(x + 3, x + 3, A200x);
		SetReadHandler(x + 4, x + 4, A2004); /* A2004; */
		SetReadHandler(x + 5, x + 5, A200x);
		SetReadHandler(x + 6, x + 6, A200x);
		SetReadHandler(x + 7, x + 7, A2007);

		SetWriteHandler(x + 0, x + 0, B2000);
		SetWriteHandler(x + 1, x + 1, B2001);
		SetWriteHandler(x + 2, x + 2, B2002);
		SetWriteHandler(x + 3, x + 3, B2003);
		SetWriteHandler(x + 4, x + 4, B2004);
		SetWriteHandler(x + 5, x + 5, B2005);
		SetWriteHandler(x + 6, x + 6, B2006);
		SetWriteHandler(x + 7, x + 7, B2007);
	}
	SetWriteHandler(0x4014, 0x4014, B4014); /* PPU OAM DMA */
}

int FCEUX_PPU_Loop(int skip);

int FCEUPPU_Loop(int skip) {
	if (newppu && (GameInfo->type != GIT_NSF)) {
		return FCEUX_PPU_Loop(skip);
	}

	/* Needed for Knight Rider, possibly others. */
	if (ppudead) {
		memset(XBuf, 0x80, 256 * 240);
		X6502_Run(scanlines_per_frame * (256 + 85));
		ppudead--;
	} else {
		X6502_Run(256 + 85);
		PPU_status |= 0x80;

		/* Not sure if this is correct.  According to Matt Conte and my own tests, it is.
		 * Timing is probably off, though.
		 * NOTE:  Not having this here breaks a Super Donkey Kong game.
		 */
		PPU[3] = PPUSPL = 0;

		/* I need to figure out the true nature and length of this delay. */
		X6502_Run(12);
		if (GameInfo->type == GIT_NSF)
			DoNSFFrame();
		else {
			if (VBlankON)
				TriggerNMI();
		}
		X6502_Run((scanlines_per_frame - 242) * (256 + 85) - 12);
		if (FSettings.PPUOverclockEnabled && ppu.overclock.vblank_scanlines) {
			if (!ppu.overclock.DMC_7bit_in_use || !FSettings.SkipDMC7BitOverclock) {
				ppu.overclock.overclocked_state = 1;
				X6502_Run(ppu.overclock.vblank_scanlines * (256 + 85) - 12);
				ppu.overclock.overclocked_state = 0;
			}
		}
		PPU_status &= 0x1f;
		X6502_Run(256);

		{
			int x;

			if (ScreenON || SpriteON) {
				if (GameHBIRQHook && ((PPU[0] & 0x38) != 0x18))
					GameHBIRQHook();
				if (PPU_hook) {
					for (x = 0; x < 42; x++) {
						PPU_hook(0x2000); PPU_hook(0);
					}
				}
				if (GameHBIRQHook2)
					GameHBIRQHook2();
			}
			X6502_Run(85 - 16);
			if (ScreenON || SpriteON) {
				RefreshAddr = TempAddr;
				if (PPU_hook) PPU_hook(RefreshAddr & 0x3fff);
			}

			/* Clean this stuff up later. */
			spork = numsprites = 0;
			ResetRL(XBuf);

			X6502_Run(16 - kook);
			kook ^= 1;
		}
		if (GameInfo->type == GIT_NSF)
			X6502_Run((256 + 85) * ppu.normal_scanlines);
		#ifdef FRAMESKIP
		else if (skip) {
			int y;

			y = SPRAM[0];
			y++;

			PPU_status |= 0x20;	/* Fixes "Bee 52".  Does it break anything? */
			if (GameHBIRQHook) {
				X6502_Run(256);
				for (scanline = 0; scanline < 240; scanline++) {
					if (ScreenON || SpriteON)
						GameHBIRQHook();
					if (scanline == y && SpriteON) PPU_status |= 0x40;
					X6502_Run((scanline == 239) ? 85 : (256 + 85));
				}
			} else if (y < 240) {
				X6502_Run((256 + 85) * y);
				if (SpriteON) PPU_status |= 0x40;	/* Quick and very dirty hack. */
				X6502_Run((256 + 85) * (240 - y));
			} else
				X6502_Run((256 + 85) * 240);
		}
		#endif
		else {
			int x, max, maxref;

			deemp = PPU[1] >> 5;

         /* manual samples can't play correctly with overclocking */
			if (ppu.overclock.DMC_7bit_in_use && FSettings.SkipDMC7BitOverclock)
				ppu.totalscanlines = ppu.normal_scanlines;
			else
				ppu.totalscanlines = ppu.normal_scanlines + (FSettings.PPUOverclockEnabled ? ppu.overclock.postrender_scanlines : 0);

			for (scanline = 0; scanline < ppu.totalscanlines; ) {	/* scanline is incremented in  DoLine.  Evil. :/ */
				deempcnt[deemp]++;
				DoLine();
				if (scanline < ppu.normal_scanlines || scanline == ppu.totalscanlines)
					ppu.overclock.overclocked_state = 0;
				else {
					if (ppu.overclock.DMC_7bit_in_use && FSettings.SkipDMC7BitOverclock) /* 7bit sample started after 240th line */
						break;
					ppu.overclock.overclocked_state = 1;
				}
			}

			ppu.overclock.DMC_7bit_in_use = 0;
			if (MMC5Hack /*&& (ScreenON || SpriteON)*/) MMC5_hb(scanline);
			for (x = 1, max = 0, maxref = 0; x < 7; x++) {
				if (deempcnt[x] > max) {
					max = deempcnt[x];
					maxref = x;
				}
				deempcnt[x] = 0;
			}
			SetNESDeemph_OldHacky(maxref, FALSE);
		}
	}

	#ifdef FRAMESKIP
	if (skip) {
		FCEU_PutImageDummy();
		return(0);
	} else
	#endif
	{
		return(1);
	}
}

static uint16 TempAddrT, RefreshAddrT;

void FCEUPPU_LoadState(int version) {
	TempAddr = TempAddrT;
	RefreshAddr = RefreshAddrT;
}

SFORMAT FCEUPPU_STATEINFO[] = {
	{ NTARAM, 0x1000, "NTAR" },
	{ PALRAM, 0x20, "PRAM" },
	{ SPRAM, 0x100, "SPRA" },
	{ PPU, 0x4, "PPUR" },
	{ &kook, 1, "KOOK" },
	{ &ppudead, 1, "DEAD" },
	{ &PPUSPL, 1, "PSPL" },
	{ &XOffset, 1, "XOFF" },
	{ &vtoggle, 1, "VTGL" },
	{ &RefreshAddrT, 2 | FCEUSTATE_RLSB, "RADD" },
	{ &TempAddrT, 2 | FCEUSTATE_RLSB, "TADD" },
	{ &VRAMBuffer, 1, "VBUF" },
	{ &PPUGenLatch, 1, "PGEN" },
	{ 0 }
};

SFORMAT FCEU_NEWPPU_STATEINFO[] = {
	{ &idleSynch, 1, "IDLS" },
	{ &spr_read.num, 4 | FCEUSTATE_RLSB, "SR_0" },
	{ &spr_read.count, 4 | FCEUSTATE_RLSB, "SR_1" },
	{ &spr_read.fetch, 4 | FCEUSTATE_RLSB, "SR_2" },
	{ &spr_read.found, 4 | FCEUSTATE_RLSB, "SR_3" },
	{ &spr_read.found_pos[0], 4 | FCEUSTATE_RLSB, "SRx0" },
	{ &spr_read.found_pos[1], 4 | FCEUSTATE_RLSB, "SRx1" },
	{ &spr_read.found_pos[2], 4 | FCEUSTATE_RLSB, "SRx2" },
	{ &spr_read.found_pos[3], 4 | FCEUSTATE_RLSB, "SRx3" },
	{ &spr_read.found_pos[4], 4 | FCEUSTATE_RLSB, "SRx4" },
	{ &spr_read.found_pos[5], 4 | FCEUSTATE_RLSB, "SRx5" },
	{ &spr_read.found_pos[6], 4 | FCEUSTATE_RLSB, "SRx6" },
	{ &spr_read.found_pos[7], 4 | FCEUSTATE_RLSB, "SRx7" },
	{ &spr_read.ret, 4 | FCEUSTATE_RLSB, "SR_4" },
	{ &spr_read.last, 4 | FCEUSTATE_RLSB, "SR_5" },
	{ &spr_read.mode, 4 | FCEUSTATE_RLSB, "SR_6" },
	{ &ppur.fv, 4 | FCEUSTATE_RLSB, "PFVx" },
	{ &ppur.v, 4 | FCEUSTATE_RLSB, "PVxx" },
	{ &ppur.h, 4 | FCEUSTATE_RLSB, "PHxx" },
	{ &ppur.vt, 4 | FCEUSTATE_RLSB, "PVTx" },
	{ &ppur.ht, 4 | FCEUSTATE_RLSB, "PHTx" },
	{ &ppur._fv, 4 | FCEUSTATE_RLSB, "P_FV" },
	{ &ppur._v, 4 | FCEUSTATE_RLSB, "P_Vx" },
	{ &ppur._h, 4 | FCEUSTATE_RLSB, "P_Hx" },
	{ &ppur._vt, 4 | FCEUSTATE_RLSB, "P_VT" },
	{ &ppur._ht, 4 | FCEUSTATE_RLSB, "P_HT" },
	{ &ppur.fh, 4 | FCEUSTATE_RLSB, "PFHx" },
	{ &ppur.s, 4 | FCEUSTATE_RLSB, "PSxx" },
	{ &ppur.status.sl, 4 | FCEUSTATE_RLSB, "PST0" },
	{ &ppur.status.cycle, 4 | FCEUSTATE_RLSB, "PST1" },
	{ &ppur.status.end_cycle, 4 | FCEUSTATE_RLSB, "PST2" },
	{ 0 }
};

void FCEUPPU_SaveState(void) {
	TempAddrT = TempAddr;
	RefreshAddrT = RefreshAddr;
}

/* --------------------- */
int pputime = 0;
int totpputime = 0;
const int kLineTime = 341;
const int kFetchTime = 2;

void runppu(int x) {
	ppur.status.cycle = (ppur.status.cycle + x) % ppur.status.end_cycle;
	if (!new_ppu_reset) { /* if resetting, suspend CPU until the first frame */
		X6502_Run(x);
	}
}

/* todo - consider making this a 3 or 4 slot fifo to keep from touching so much memory */
typedef struct BGData {
	uint8 nt, pecnt, at, pt[2], qtnt;
	uint8 ppu1[8];
} BGData;

BGData bgmain[34]; /* one at the end is junk, it can never be rendered */

static INLINE void BGData_Read(BGData *bg) {
	NTRefreshAddr = RefreshAddr = newppu_regs_get_ntread(&ppur);
	if (PEC586Hack)
		ppur.s = (RefreshAddr & 0x200) >> 9;
	else if (QTAIHack) {
		bg->qtnt = QTAINTRAM[((((RefreshAddr >> 10) & 3) >> ((qtaintramreg >> 1)) & 1) << 10) | (RefreshAddr & 0x3FF)];
		ppur.s = bg->qtnt & 0x3F;
	}
	bg->pecnt = (RefreshAddr & 1) << 3;
	bg->nt = CALL_PPUREAD(RefreshAddr);
	bg->ppu1[0] = PPU[1];
	runppu(1);
	bg->ppu1[1] = PPU[1];
	runppu(1);

	RefreshAddr = newppu_regs_get_atread(&ppur);
	bg->at = CALL_PPUREAD(RefreshAddr);

	/* modify at to get appropriate palette shift */
	if (ppur.vt & 2)
		bg->at >>= 4;
	if (ppur.ht & 2)
		bg->at >>= 2;
	bg->at &= 0x03;
	bg->at <<= 2;
	/* horizontal scroll clocked at cycle 3 and then
	 * vertical scroll at 251
	 */
	bg->ppu1[2] = PPU[1];
	runppu(1);
	if (PPUON) {
		newppu_regs_increment_hsc(&ppur);
		if (ppur.status.cycle == 251)
			newppu_regs_increment_vs(&ppur);
	}
	bg->ppu1[3] = PPU[1];
	runppu(1);

	ppur.par = bg->nt;
	RefreshAddr = newppu_regs_get_ptread(&ppur);
	if (PEC586Hack) {
		bg->pt[0] = CALL_PPUREAD(RefreshAddr | bg->pecnt);
		bg->ppu1[4] = PPU[1];
		runppu(1);
		bg->ppu1[5] = PPU[1];
		runppu(1);
		bg->pt[1] = CALL_PPUREAD(RefreshAddr | bg->pecnt);
		bg->ppu1[6] = PPU[1];
		runppu(1);
		bg->ppu1[7] = PPU[1];
		runppu(1);
	} else if (QTAIHack && (bg->qtnt & 0x40)) {
		uint32 vadr = (ROM.chr.size == (128 * 1024)) ? ((RefreshAddr & 0x00007) << 1) | ((RefreshAddr & 0x00010) >> 4) | ((RefreshAddr & 0x3FFE0) >> 1) : RefreshAddr;
		bg->pt[0] = ROM.chr.data[vadr];
		bg->ppu1[4] = PPU[1];
		runppu(1);
		bg->ppu1[5] = PPU[1];
		runppu(1);
		RefreshAddr |= 8;
		bg->pt[1] = ROM.chr.data[vadr];
		bg->ppu1[6] = PPU[1];
		runppu(1);
		bg->ppu1[7] = PPU[1];
		runppu(1);
	} else {
		if (ScreenON)
			RENDER_LOG(RefreshAddr);
		bg->pt[0] = CALL_PPUREAD(RefreshAddr);
		bg->ppu1[4] = PPU[1];
		runppu(1);
		bg->ppu1[5] = PPU[1];
		runppu(1);
		RefreshAddr |= 8;
		if (ScreenON)
			RENDER_LOG(RefreshAddr);
		bg->pt[1] = CALL_PPUREAD(RefreshAddr);
		bg->ppu1[6] = PPU[1];
		runppu(1);
		bg->ppu1[7] = PPU[1];
		runppu(1);
	}
}

static INLINE int PaletteAdjustPixel(int pixel) {
	if ((PPU[1] >> 5) == 0x7)
		return (pixel & 0x3f) | 0xc0;
	else if (PPU[1] & 0xE0)
		return pixel | 0x40;
	else
		return (pixel & 0x3F) | 0x80;
}

int framectr = 0;
int FCEUX_PPU_Loop(int skip) {
	int dot;

	if (new_ppu_reset) /* first frame since reset, time to initialize */
	{
		newppu_regs_reset(&ppur);
		spr_read_reset(&spr_read);
		new_ppu_reset = FALSE;
	}

	/* 262 scanlines */
	if (ppudead) {
		/* not quite emulating all the NES power up behavior
		 * since it is known that the NES ignores writes to some
		 * register before around a full frame, but no games
		 * should write to those regs during that time, it needs
		 * to wait for vblank
		 */
		ppur.status.sl = 241;
		if (isPAL)
			runppu(70 * kLineTime);
		else
			runppu(20 * kLineTime);
		ppur.status.sl = 0;
		runppu(242 * kLineTime);
		--ppudead;
		goto finish;
	}

	{
		const int delay = 20;	/* fceu used 12 here but I couldnt get it to work in marble madness and pirates. */

		int dot, S, sl;
		int sltodo = isPAL ? 70 : 20;

		static uint8 oams[2][64][8];/* [7] turned to [8] for faster indexing */
		static int oamcounts[2] = { 0, 0 };
		static int oamslot = 0;
		static int oamcount;

		PPU_status |= 0x80;

		ppuphase = PPUPHASE_VBL;

		/* Not sure if this is correct.  According to Matt Conte and my own tests, it is.
		 * Timing is probably off, though.
		 *NOTE:  Not having this here breaks a Super Donkey Kong game.
		 */
		PPU[3] = PPUSPL = 0;

		ppur.status.sl = 241;	/* for sprite reads */

		/* formerly: runppu(delay); */
		for (dot = 0; dot < delay; dot++)
			runppu(1);

		if (VBlankON) TriggerNMI();

		/* formerly: runppu(20 * (kLineTime) - delay); */
		for (S = 0; S < sltodo; S++) {
			for (dot = (S == 0 ? delay : 0); dot < kLineTime; dot++)
				runppu(1);
			ppur.status.sl++;
		}

		/* this seems to run just before the dummy scanline begins */
		PPU_status = 0;
		/* this early out caused metroid to fail to boot. I am leaving it here as a reminder of what not to do */
		/* if(!PPUON) { runppu(kLineTime*242); goto finish; } */

		/* There are 2 conditions that update all 5 PPU scroll counters with the
		 * contents of the latches adjacent to them. The first is after a write to
		 * 2006/2. The second, is at the beginning of scanline 20, when the PPU starts
		 * rendering data for the first time in a frame (this update won't happen if
		 * all rendering is disabled via 2001.3 and 2001.4).
		 */

		/* if(PPUON)
			ppur.install_latches(); */

		/* capture the initial xscroll
		 * int xscroll = ppur.fh;
		 * render 241/291 scanlines (1 dummy at beginning, dendy's 50 at the end)
		 * ignore overclocking!
		 */

		for (sl = 0; sl < ppu.normal_scanlines; sl++) {
			const int yp = sl - 1;
			int scanslot, renderslot;
			int spriteHeight;
			int i, s, xt; /* loop counters */

			spr_read_start_scanline(&spr_read);

			g_rasterpos = 0;
			ppur.status.sl = sl;

			linestartts = timestamp * 48 + cpu.count; /* pixel timestamp for debugger */

			ppuphase = PPUPHASE_BG;

			if (sl != 0 && sl < 241)  /* ignore the invisible */
			{
				/* DEBUG(FCEUD_UpdatePPUView(scanline = yp, 1));
				DEBUG(FCEUD_UpdateNTView(scanline = yp, 1)); */
			}

			/* hack to fix SDF ship intro screen with split. is it right?
			 * well, if we didnt do this, we'd be passing in a negative scanline, so that's a sign something is fishy..
			 */
			if(sl != 0)
				if (MMC5Hack) MMC5_hb(yp);


			/* twiddle the oam buffers */
			scanslot = oamslot ^ 1;
			renderslot = oamslot;
			oamslot ^= 1;

			oamcount = oamcounts[renderslot];

			/* the main scanline rendering loop:
			 * 32 times, we will fetch a tile and then render 8 pixels.
			 * two of those tiles were read in the last scanline.
			 */
			for (xt = 0; xt < 32; xt++) {
				const uint8 blank = (gNoBGFillColor == 0xFF) ? READPAL(0) : gNoBGFillColor;
				
				BGData_Read(&bgmain[xt + 2]);

				/* ok, we're also going to draw here.
				 * unless we're on the first dummy scanline
				 */
				if (sl != 0 && sl < 241) { /* cape at 240 for dendy, its PPU does nothing afterwards */
					int xp;
					int xstart = xt << 3;

					uint8 *const target = XBuf + (yp << 8) + xstart;
					uint8 *const dtarget = XDBuf + (yp << 8) + xstart;
					uint8 *ptr = target;
					uint8 *dptr = dtarget;
					int rasterpos = xstart;

					/* check all the conditions that can cause things to render in these 8px */
					const bool renderspritenow = SpriteON && (xt > 0 || SpriteLeft8ON);
					const bool renderbgnow = ScreenON && (xt > 0 || BGLeft8ON);

					oamcount = oamcounts[renderslot];

					for (xp = 0; xp < 8; xp++, rasterpos++, g_rasterpos++) {
						/* bg pos is different from raster pos due to its offsetability.
						 * so adjust for that here
						 */
						const int bgpos = rasterpos + ppur.fh;
						const int bgpx = bgpos & 7;
						const int bgtile = bgpos >> 3;

						uint8 pixel = 0;
						uint8 pixelcolor = blank;

						bool havepixel = FALSE;
						int s1;

						/* according to qeed's doc, use palette 0 or $2006's value if it is & 0x3Fxx */
						if (!ScreenON && !SpriteON)
						{
							/* if there's anything wrong with how we're doing this, someone please chime in */
							int addr = newppu_regs_get_2007access(&ppur);
							if ((addr & 0x3F00) == 0x3F00)
							{
								pixel = addr & 0x1F;
							}
							pixelcolor = READPAL_MOTHEROFALL(pixel);
						}

						/* generate the BG data */
						if (renderbgnow) {
							uint8* pt = bgmain[bgtile].pt;
							pixel = ((pt[0] >> (7 - bgpx)) & 1) | (((pt[1] >> (7 - bgpx)) & 1) << 1) | bgmain[bgtile].at;
						}
						if (show_background)
							pixelcolor = READPALNOGS(pixel);

						/* look for a sprite to be drawn */
						havepixel = FALSE;
						for (s1 = 0; s1 < oamcount; s1++) {
							uint8* oam = oams[renderslot][s1];
							int x = oam[3];
							if (rasterpos >= x && rasterpos < x + 8) {
								/* build the pixel.
								 * fetch the LSB of the patterns
								 */
								uint8 spixel = oam[4] & 1;
								spixel |= (oam[5] & 1) << 1;

								/* shift down the patterns so the next pixel is in the LSB */
								oam[4] >>= 1;
								oam[5] >>= 1;

								if (!renderspritenow) continue;

								/* bail out if we already have a pixel from a higher priority sprite */
								if (havepixel) continue;

								/* transparent pixel bailout */
								if (spixel == 0) continue;

								/* spritehit:
								 * 1. is it sprite#0?
								 * 2. is the bg pixel nonzero?
								 * then, it is spritehit.
								 */
								if (oam[6] == 0 && (pixel & 3) != 0 &&
									rasterpos < 255) {
									PPU_status |= 0x40;
								}
								havepixel = TRUE;

								/* priority handling */
								if (oam[2] & 0x20) {
									/* behind background: */
									if ((pixel & 3) != 0) continue;
								}

								/* bring in the palette bits and palettize */
								spixel |= (oam[2] & 3) << 2;

								if (show_sprites)
									pixelcolor = READPALNOGS(0x10 + spixel);
							}
						}

						/* apply grayscale.. kind of clunky
						 * really we need to read the entire palette instead of just ppu1
						 * this will be needed for special color effects probably (very fine rainbows and whatnot?)
						 * are you allowed to chang the palette mid-line anyway? well you can definitely change the grayscale flag as we know from the FF1 "polygon" effect
						 */
						if(bgmain[xt+2].ppu1[xp]&1)
							pixelcolor &= 0x30;

						/* this does deemph stuff inside it.. which is probably wrong... */
						*ptr = PaletteAdjustPixel(pixelcolor);

						ptr++;

						/* grab deemph.. */
						/* I guess this works the same way as the grayscale, ideally? */
						*dptr++ = bgmain[xt+2].ppu1[xp]>>5;
					}
				}
			}

			/* look for sprites (was supposed to run concurrent with bg rendering) */
			oamcounts[scanslot] = 0;
			oamcount = 0;
			spriteHeight = Sprite16 ? 16 : 8;
			for (i = 0; i < 64; i++) {
				uint8* spr = SPRAM + i * 4;

				oams[scanslot][oamcount][7] = 0;
				if (yp >= spr[0] && yp < spr[0] + spriteHeight) {
					int j;
					/* if we already have maxsprites, then this new one causes
					 * an overflow, set the flag and bail out.
					 */
					if (oamcount >= 8 && PPUON) {
						PPU_status |= 0x20;
						if (maxsprites == 8)
							break;
					}

					/* just copy some bytes into the internal sprite buffer */
					for (j = 0; j < 4; j++)
						oams[scanslot][oamcount][j] = spr[j];
					oams[scanslot][oamcount][7] = 1;

					/* note that we stuff the oam index into [6].
					 * i need to turn this into a struct so we can have fewer magic numbers
					 */
					oams[scanslot][oamcount][6] = (uint8)i;
					oamcount++;
				}
			}
			oamcounts[scanslot] = oamcount;

			/* FV is clocked by the PPU's horizontal blanking impulse, and therefore will increment every scanline.
			 * well, according to (which?) tests, maybe at the end of hblank.
			 * but, according to what it took to get crystalis working, it is at the beginning of hblank.

			 * this is done at cycle 251
			 * rendering scanline, it doesn't need to be scanline 0,
			 * because on the first scanline when the increment is 0, the vs_scroll is reloaded.
			 */
			/* if(PPUON && sl != 0)
				ppur.increment_vs(); */

			/* todo - think about clearing oams to a predefined value to force deterministic behavior */

			ppuphase = PPUPHASE_OBJ;

			/* fetch sprite patterns */
			for (s = 0; s < maxsprites; s++) {
				/* if this is a real sprite sprite, then it is not above the 8 sprite limit.
				 * this is how we support the no 8 sprite limit feature.
				 * not that at some point we may need a virtual CALL_PPUREAD which just peeks and doesnt increment any counters
				 * this could be handy for the debugging tools also
				 */
				const bool realSprite = (s < 8);

				uint8* const oam = oams[scanslot][s];
				uint32 line = yp - oam[0];

				uint32 patternNumber = oam[1];
				uint32 patternAddress;

				int garbage_todo;

				/* if we have hit our eight sprite pattern and we dont have any more sprites, then bail */
				if (s == oamcount && s >= 8)
					break;

				
				if (oam[2] & 0x80)	/* vflip */
					line = spriteHeight - line - 1;

				/* create deterministic dummy fetch pattern */
				if (!oam[7]) {
					patternNumber = 0;
					line = 0;
				}

				/* 8x16 sprite handling: */
				if (Sprite16) {
					uint32 bank = (patternNumber & 1) << 12;
					patternNumber = patternNumber & ~1;
					patternNumber |= (line >> 3);
					patternAddress = (patternNumber << 4) | bank;
				} else {
					patternAddress = (patternNumber << 4) | (SpAdrHI << 9);
				}

				/* offset into the pattern for the current line.
				 * tricky: tall sprites have already had lines>8 taken care of by getting a new pattern number above.
				 * so we just need the line offset for the second pattern
				 */
				patternAddress += line & 7;

				/* garbage nametable fetches */
				garbage_todo = 2;
				if (PPUON) {
					if (sl == 0 && ppur.status.cycle == 304) {
						runppu(1);
						if (PPUON) newppu_regs_install_latches(&ppur);
						runppu(1);
						garbage_todo = 0;
					}
					if ((sl != 0 && sl < 241) && ppur.status.cycle == 256) {
						runppu(1);
						/* at 257: 3d world runner is ugly if we do this at 256
						 */
						if (PPUON) newppu_regs_install_h_latches(&ppur);
						runppu(1);
						garbage_todo = 0;
					}
				}
				if (realSprite) runppu(garbage_todo);

				/* Dragon's Lair (Europe version mapper 4)
				 * does not set SpriteON in the beginning but it does
				 * set the bg on so if using the conditional SpriteON the MMC3 counter
				 * the counter will never count and no IRQs will be fired so use PPUON
				 */
				if (((PPU[0] & 0x38) != 0x18) && s == 2 && PPUON) {
					/* (The MMC3 scanline counter is based entirely on PPU A12, triggered on rising edges (after the line remains low for a sufficiently long period of time))
					 * http://nesdevwiki.org/wiki/index.php/Nintendo_MMC3
					 * test cases for timing: SMB3, Crystalis
					 * crystalis requires deferring this til somewhere in sprite [1,3]
					 * kirby requires deferring this til somewhere in sprite [2,5..
					 */
					/* if (PPUON && GameHBIRQHook) { */
					if (GameHBIRQHook) {
						GameHBIRQHook();
					}
				}

				/* blind attempt to replicate old ppu functionality */
				if (s == 2 && PPUON) {
					if (GameHBIRQHook2) {
						GameHBIRQHook2();
					}
				}

				if (realSprite) runppu(kFetchTime);


				/* pattern table fetches */
				RefreshAddr = patternAddress;
				/*if (SpriteON)
					RENDER_LOG(RefreshAddr);*/
				oam[4] = CALL_PPUREAD(RefreshAddr);
				if (realSprite) runppu(kFetchTime);

				RefreshAddr += 8;
				/*if (SpriteON)
					RENDER_LOG(RefreshAddr);*/
				oam[5] = CALL_PPUREAD(RefreshAddr);
				if (realSprite) runppu(kFetchTime);

				/* hflip */
				if (!(oam[2] & 0x40)) {
					oam[4] = bitrevlut(oam[4]);
					oam[5] = bitrevlut(oam[5]);
				}
			}

			ppuphase = PPUPHASE_BG;

			/* fetch BG: two tiles for next line */
			for (xt = 0; xt < 2; xt++)
				BGData_Read(&bgmain[xt]);

			/* I'm unclear of the reason why this particular access to memory is made.
			 * The nametable address that is accessed 2 times in a row here, is also the
			 * same nametable address that points to the 3rd tile to be rendered on the
			 * screen (or basically, the first nametable address that will be accessed when
			 * the PPU is fetching background data on the next scanline).
			 * (not implemented yet)
			 */
			runppu(kFetchTime);
			if (sl == 0) {
				if (idleSynch && PPUON && !isPAL)
					ppur.status.end_cycle = 340;
				else
					ppur.status.end_cycle = 341;
				idleSynch ^= 1;
			} else
				ppur.status.end_cycle = 341;
			runppu(kFetchTime);

			/* After memory access 170, the PPU simply rests for 4 cycles (or the
			 * equivelant of half a memory access cycle) before repeating the whole
			 * pixel/scanline rendering process. If the scanline being rendered is the very
			 * first one on every second frame, then this delay simply doesn't exist.
			 */
			if (ppur.status.end_cycle == 341)
				runppu(1);
		}	/* scanline loop */

		ppu.overclock.DMC_7bit_in_use = 0;

		if (MMC5Hack) MMC5_hb(240);

		/* idle for one line */
		runppu(kLineTime);
		framectr++;
	}

finish:
	return 0;
}
