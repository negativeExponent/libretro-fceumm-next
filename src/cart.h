#ifndef _FCEU_CART_H
#define _FCEU_CART_H

#define PRG_PAGE_SIZE 0x4000
#define CHR_PAGE_SIZE 0x2000

#define PRG_BANK_COUNT(x) (ROM.prg.size / ((x) * 1024))
#define CHR_BANK_COUNT(x) (ROM.chr.size / ((x) * 1024))

enum DefaultValues { DEFAULT = -1, NOEXTRA = -1 };
enum ConsoleSystem { NES_NTSC = 0, NES_PAL = 1, MULTI = 2, DENDY = 3 };

typedef struct {
	/* Set by mapper/board code: */
	void (*Power)(void);
	void (*Reset)(void);
	void (*Close)(void);

	uint8_t *SaveGame[4];    /* Pointers to memory to save/load. */
	uint32_t SaveGameLen[4]; /* How much memory to save/load. */

	/* Set by iNES/UNIF loading code. */
	int format;
	int iNES2;   /* iNES version */
	int mapper; /* mapper used */
	int submapper;
	int mirror;      /* As set in the header or chunk.
	                    * iNES/UNIF specific.  Intended
	                    * to help support games like "Karnov"
	                    * that are not really MMC3 but are
	                    * set to mapper 4.
	                    */
	int mirror2bits; /* a 2bit representation for mirroring.
	                    * For use in nonstandard way like apply one-screen mirroring on
	                    * mapper 30.
	                    */
	int battery;     /* Presence of an actual battery. */
	int trainer;     /* Presense of trainer data */
	int MiscRoms;    /* Presense of misc roms */
	int region;      /* video system timing (NTSC, PAL, Dendy */
	int ConsoleType;
	int InputTypes;
	int VS_PPUTypes;
	int VS_HWType;

	int PRGRamSize;     /* prg ram size in bytes (volatile) */
	int CHRRamSize;     /* chr ram size in bytes (volatile) */
	int PRGRamSaveSize; /* prg ram size in bytes (non-volatile or battery backed) */
	int CHRRamSaveSize; /* chr ram size in bytes (non-volatile or battery backed) */

	uint8_t MD5[16];
	uint32_t PRGCRC32;
	uint32_t CHRCRC32;
	uint32_t CRC32; /* Should be set by the iNES/UNIF loading
	               * code, used by mapper/board code, maybe
	               * other code in the future.
	               */
} CartInfo;

typedef struct mem_t {
	uint8_t *data;
	uint32_t size;
} mem_t;

typedef struct romData_t {
	mem_t prg;
	mem_t chr;
	mem_t misc;
	mem_t disk;
	mem_t disko;
} romData_t;

extern uint8_t *Page[32], *VPage[8], *MMC5SPRVPage[8], *MMC5BGVPage[8];

void ResetCartMapping(void);
void SetupCartPRGMapping(int chip, uint8_t *p, uint32_t size, uint8_t ram);
void SetupCartCHRMapping(int chip, uint8_t *p, uint32_t size, uint8_t ram);
void SetupCartMirroring(int m, int hard, uint8_t *extra);

DECLFR(CartBROB);
DECLFR(CartBR);
DECLFW(CartBW);

extern uint8_t *PRGptr[32];
extern uint8_t *CHRptr[32];

extern uint32_t PRGsize[32];
extern uint32_t CHRsize[32];

extern uint32_t PRGmask2[32];
extern uint32_t PRGmask4[32];
extern uint32_t PRGmask8[32];
extern uint32_t PRGmask16[32];
extern uint32_t PRGmask32[32];

extern uint32_t CHRmask1[32];
extern uint32_t CHRmask2[32];
extern uint32_t CHRmask4[32];
extern uint32_t CHRmask8[32];

void setprg2(uint16_t A, uint16_t V);
void setprg4(uint16_t A, uint16_t V);
void setprg8(uint16_t A, uint16_t V);
void setprg16(uint16_t A, uint16_t V);
void setprg32(uint16_t A, uint16_t V);

void setprg2r(int r, uint16_t A, uint16_t V);
void setprg4r(int r, uint16_t A, uint16_t V);
void setprg8r(int r, uint16_t A, uint16_t V);
void setprg16r(int r, uint16_t A, uint16_t V);
void setprg32r(int r, uint16_t A, uint16_t V);

void setprg2_access(uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);
void setprg4_access(uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);
void setprg8_access(uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);
void setprg16_access(uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);
void setprg32_access(uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);

void setprg2r_access(int r, uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);
void setprg4r_access(int r, uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);
void setprg8r_access(int r, uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);
void setprg16r_access(int r, uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);
void setprg32r_access(int r, uint16_t A, uint16_t V, uint8_t rd, uint8_t wr);

void unsetcpu2(uint16_t A);
void unsetcpu4(uint16_t A);
void unsetcpu8(uint16_t A);
void unsetcpu16(uint16_t A);
void unsetcpu32(uint16_t A);

void setchr1r(int r, uint16_t A, uint16_t V);
void setchr2r(int r, uint16_t A, uint16_t V);
void setchr4r(int r, uint16_t A, uint16_t V);
void setchr8r(int r, uint16_t V);

void setchr1(uint16_t A, uint16_t V);
void setchr2(uint16_t A, uint16_t V);
void setchr4(uint16_t A, uint16_t V);
void setchr8(uint16_t V);

void setmirror(int t);
void setmirrorw(int a, int b, int c, int d);
void setntamem(uint8_t *p, int ram, int b);

enum MirroringType {
	MI_H = 0, /* horizontal */
	MI_V = 1, /* vertical */
	MI_0 = 2, /* single-screen 0 */
	MI_1 = 3, /* single-screen 1 */
	MI_4 = 4  /* four-screen */
};

extern CartInfo iNESCart;

extern uint8_t *CHRRAM;
extern uint32_t CHRRAMSIZE;

extern uint8_t *WRAM;
extern uint32_t WRAMSIZE;

extern romData_t ROM;

/* */
uint32_t GetWRAMSize(const CartInfo *info, uint32_t default_size);
uint32_t GetCHRRAMSize(const CartInfo *info, uint32_t default_size);
#endif
