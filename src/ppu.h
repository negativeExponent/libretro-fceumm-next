#ifndef _FCEU_PPU_H
#define _FCEU_PPU_H

#define VBlankON          (PPU[0] & 0x80) /* Generate VBlank NMI */
#define Sprite16          (PPU[0] & 0x20) /* Sprites 8x16/8x8 */
#define BGAdrHI           (PPU[0] & 0x10) /* BG pattern adr $0000/$1000 */
#define SpAdrHI           (PPU[0] & 0x08) /* Sprite pattern adr $0000/$1000 */
#define INC32             (PPU[0] & 0x04) /* auto increment 1/32 */

#define GRAYSCALE         (PPU[1] & 0x01) /* Grayscale (AND palette entries with 0x30) */
#define BGLeft8ON         (PPU[1] & 0x02) /* show background in leftmost 8 pixels of screen. 0: hides */
#define SpriteLeft8ON     (PPU[1] & 0x04) /* show sprites in leftmost 8 pixels of screen. 0: hides */
#define ScreenON          (PPU[1] & 0x08) /* Show screen */
#define SpriteON          (PPU[1] & 0x10) /* Show Sprite */
#define PPUON             (PPU[1] & 0x18) /* PPU should operate */

#define PPU_status        (PPU[2])
#define Sprite0Hit        (PPU_status & 0x40)

#define READPALNOGS(ofs)  (PALRAM[(ofs)])
#define READPAL(ofs)      (PALRAM[(ofs)] & (GRAYSCALE ? 0x30 : 0xFF))
#define READUPAL(ofs)     (UPALRAM[(ofs)] & (GRAYSCALE ? 0x30 : 0xFF))

typedef struct {
	/* overclock the console by adding dummy scanlines to PPU loop
	 * disables DMC DMA and WaveHi filling for these dummies
	 * doesn't work with new PPU */
	struct {
		uint8_t overclocked_state; /* ppu in overclock state? stops sound from running in overclocked state */
		uint8_t DMC_7bit_in_use; /* DMC in use */
		uint16_t vblank_scanlines;
		uint16_t postrender_scanlines;
	} overclock;
	uint16_t totalscanlines;
	uint16_t normal_scanlines;
} PPU_T;

extern PPU_T ppu;

void FCEUPPU_Init(void);
void FCEUPPU_Reset(void);
void FCEUPPU_Power(void);
int FCEUPPU_Loop(int skip);

void FCEUPPU_LineUpdate(void);
void FCEUPPU_SetVideoSystem(int w);

extern void (*GameHBIRQHook)(void), (*GameHBIRQHook2)(void);
extern void (*PPU_hook)(uint32_t A);

int newppu_get_scanline(void);
int newppu_get_dot(void);
void newppu_hacky_emergency_reset(void);

/* For cart.c and banksw.h, mostly */
extern uint8_t NTARAM[0x1000], *vnapage[4];
extern uint8_t PPUNTARAM;
extern uint8_t PPUCHRRAM;

void FCEUPPU_SaveState(void);
void FCEUPPU_LoadState(int version);

extern int g_rasterpos;
extern int scanline;
extern uint8_t PPU[4];

void PPU_ResetHooks(void);
extern uint8_t (*FFCEUX_PPURead)(uint32_t A);
extern void (*FFCEUX_PPUWrite)(uint32_t A, uint8_t V);
extern uint8_t FFCEUX_PPURead_Default(uint32_t A);
void FFCEUX_PPUWrite_Default(uint32_t A, uint8_t V);

uint8_t* FCEUPPU_GetCHR(uint32_t vadr, uint32_t refreshaddr);
int FCEUPPU_GetAttr(int ntnum, int xt, int yt);
void ppu_getScroll(int *xpos, int *ypos);

typedef enum PPUPHASE {
	PPUPHASE_VBL, PPUPHASE_BG, PPUPHASE_OBJ
} PPUPHASE;

extern PPUPHASE ppuphase;

#define ABANKS            MMC5SPRVPage
#define BBANKS            MMC5BGVPage
#define MMC5SPRVRAMADR(V) &MMC5SPRVPage[(V) >> 10][(V)]
#define VRAMADR(V)        &VPage[(V) >> 10][(V)]
uint8_t *MMC5BGVRAMADR(uint32_t A);

#endif
