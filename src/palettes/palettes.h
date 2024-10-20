pal rp2c04_0001[64] = {
	#include "rp2c04-0001.h"
};

pal rp2c04_0002[64] = {
	#include "rp2c04-0002.h"
};

pal rp2c04_0003[64] = {
	#include "rp2c04-0003.h"
};

pal rp2c04_0004[64] = {
	#include "rp2c04-0004.h"
};

pal rp2c03[64] = {
	#include "rp2c03.h"
};

pal unvpalette[7] = {
	{ 0x00, 0x00, 0x00 }, /*  0 = Black */
	{ 0xFF, 0xFF, 0xD3 }, /*  1 = White */
	{ 0x00, 0x00, 0x00 }, /*  2 = Black */
	{ 0x75, 0x75, 0x92 }, /*  3 = Greyish */
	{ 0xBE, 0x00, 0x00 }, /*  4 = Reddish */
	{ 0x33, 0xFF, 0x33 }, /*  5 = Bright green */
	{ 0x31, 0x0E, 0xC8 }  /*  6 = Ultramarine Blue */
};

#define P64(x) (((x) << 2) | ((x >> 4) & 3))

/* Default palette */
pal palette[64] = {
#if 0
	{ P64(0x1D), P64(0x1D), P64(0x1D) }, /* Value 0 */
	{ P64(0x09), P64(0x06), P64(0x23) }, /* Value 1 */
	{ P64(0x00), P64(0x00), P64(0x2A) }, /* Value 2 */
	{ P64(0x11), P64(0x00), P64(0x27) }, /* Value 3 */
	{ P64(0x23), P64(0x00), P64(0x1D) }, /* Value 4 */
	{ P64(0x2A), P64(0x00), P64(0x04) }, /* Value 5 */
	{ P64(0x29), P64(0x00), P64(0x00) }, /* Value 6 */
	{ P64(0x1F), P64(0x02), P64(0x00) }, /* Value 7 */
	{ P64(0x10), P64(0x0B), P64(0x00) }, /* Value 8 */
	{ P64(0x00), P64(0x11), P64(0x00) }, /* Value 9 */
	{ P64(0x00), P64(0x14), P64(0x00) }, /* Value 10 */
	{ P64(0x00), P64(0x0F), P64(0x05) }, /* Value 11 */
	{ P64(0x06), P64(0x0F), P64(0x17) }, /* Value 12 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 13 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 14 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 15 */
	{ P64(0x2F), P64(0x2F), P64(0x2F) }, /* Value 16 */
	{ P64(0x00), P64(0x1C), P64(0x3B) }, /* Value 17 */
	{ P64(0x08), P64(0x0E), P64(0x3B) }, /* Value 18 */
	{ P64(0x20), P64(0x00), P64(0x3C) }, /* Value 19 */
	{ P64(0x2F), P64(0x00), P64(0x2F) }, /* Value 20 */
	{ P64(0x39), P64(0x00), P64(0x16) }, /* Value 21 */
	{ P64(0x36), P64(0x0A), P64(0x00) }, /* Value 22 */
	{ P64(0x32), P64(0x13), P64(0x03) }, /* Value 23 */
	{ P64(0x22), P64(0x1C), P64(0x00) }, /* Value 24 */
	{ P64(0x00), P64(0x25), P64(0x00) }, /* Value 25 */
	{ P64(0x00), P64(0x2A), P64(0x00) }, /* Value 26 */
	{ P64(0x00), P64(0x24), P64(0x0E) }, /* Value 27 */
	{ P64(0x00), P64(0x20), P64(0x22) }, /* Value 28 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 29 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 30 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 31 */
	{ P64(0x3F), P64(0x3F), P64(0x3F) }, /* Value 32 */
	{ P64(0x0F), P64(0x2F), P64(0x3F) }, /* Value 33 */
	{ P64(0x17), P64(0x25), P64(0x3F) }, /* Value 34 */
	{ P64(0x33), P64(0x22), P64(0x3F) }, /* Value 35 */
	{ P64(0x3D), P64(0x1E), P64(0x3F) }, /* Value 36 */
	{ P64(0x3F), P64(0x1D), P64(0x2D) }, /* Value 37 */
	{ P64(0x3F), P64(0x1D), P64(0x18) }, /* Value 38 */
	{ P64(0x3F), P64(0x26), P64(0x0E) }, /* Value 39 */
	{ P64(0x3C), P64(0x2F), P64(0x0F) }, /* Value 40 */
	{ P64(0x20), P64(0x34), P64(0x04) }, /* Value 41 */
	{ P64(0x13), P64(0x37), P64(0x12) }, /* Value 42 */
	{ P64(0x16), P64(0x3E), P64(0x26) }, /* Value 43 */
	{ P64(0x00), P64(0x3A), P64(0x36) }, /* Value 44 */
	{ P64(0x1E), P64(0x1E), P64(0x1E) }, /* Value 45 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 46 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 47 */
	{ P64(0x3F), P64(0x3F), P64(0x3F) }, /* Value 48 */
	{ P64(0x2A), P64(0x39), P64(0x3F) }, /* Value 49 */
	{ P64(0x31), P64(0x35), P64(0x3F) }, /* Value 50 */
	{ P64(0x35), P64(0x32), P64(0x3F) }, /* Value 51 */
	{ P64(0x3F), P64(0x31), P64(0x3F) }, /* Value 52 */
	{ P64(0x3F), P64(0x31), P64(0x36) }, /* Value 53 */
	{ P64(0x3F), P64(0x2F), P64(0x2C) }, /* Value 54 */
	{ P64(0x3F), P64(0x36), P64(0x2A) }, /* Value 55 */
	{ P64(0x3F), P64(0x39), P64(0x28) }, /* Value 56 */
	{ P64(0x38), P64(0x3F), P64(0x28) }, /* Value 57 */
	{ P64(0x2A), P64(0x3C), P64(0x2F) }, /* Value 58 */
	{ P64(0x2C), P64(0x3F), P64(0x33) }, /* Value 59 */
	{ P64(0x27), P64(0x3F), P64(0x3C) }, /* Value 60 */
	{ P64(0x31), P64(0x31), P64(0x31) }, /* Value 61 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 62 */
	{ P64(0x00), P64(0x00), P64(0x00) }, /* Value 63 */
	#undef P64
#endif
	{ 0x66, 0x66, 0x66 },
	{ 0x00, 0x2a, 0x88 },
	{ 0x14, 0x12, 0xa7 },
	{ 0x3b, 0x00, 0xa4 },
	{ 0x5c, 0x00, 0x7e },
	{ 0x6e, 0x00, 0x40 },
	{ 0x6c, 0x06, 0x00 },
	{ 0x56, 0x1d, 0x00 },
	{ 0x33, 0x35, 0x00 },
	{ 0x0b, 0x48, 0x00 },
	{ 0x00, 0x52, 0x00 },
	{ 0x00, 0x4f, 0x08 },
	{ 0x00, 0x40, 0x4d },
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00 },
	{ 0xad, 0xad, 0xad },
	{ 0x15, 0x5f, 0xd9 },
	{ 0x42, 0x40, 0xff },
	{ 0x75, 0x27, 0xfe },
	{ 0xa0, 0x1a, 0xcc },
	{ 0xb7, 0x1e, 0x7b },
	{ 0xb5, 0x31, 0x20 },
	{ 0x99, 0x4e, 0x00 },
	{ 0x6b, 0x6d, 0x00 },
	{ 0x38, 0x87, 0x00 },
	{ 0x0c, 0x93, 0x00 },
	{ 0x00, 0x8f, 0x32 },
	{ 0x00, 0x7c, 0x8d },
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00 },
	{ 0xff, 0xfe, 0xff },
	{ 0x64, 0xb0, 0xff },
	{ 0x92, 0x90, 0xff },
	{ 0xc6, 0x76, 0xff },
	{ 0xf3, 0x6a, 0xff },
	{ 0xfe, 0x6e, 0xcc },
	{ 0xfe, 0x81, 0x70 },
	{ 0xea, 0x9e, 0x22 },
	{ 0xbc, 0xbe, 0x00 },
	{ 0x88, 0xd8, 0x00 },
	{ 0x5c, 0xe4, 0x30 },
	{ 0x45, 0xe0, 0x82 },
	{ 0x48, 0xcd, 0xde },
	{ 0x4f, 0x4f, 0x4f },
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00 },
	{ 0xff, 0xfe, 0xff },
	{ 0xc0, 0xdf, 0xff },
	{ 0xd3, 0xd2, 0xff },
	{ 0xe8, 0xc8, 0xff },
	{ 0xfb, 0xc2, 0xff },
	{ 0xfe, 0xc4, 0xea },
	{ 0xfe, 0xcc, 0xc5 },
	{ 0xf7, 0xd8, 0xa5 },
	{ 0xe4, 0xe5, 0x94 },
	{ 0xcf, 0xef, 0x96 },
	{ 0xbd, 0xf4, 0xab },
	{ 0xb3, 0xf3, 0xcc },
	{ 0xb5, 0xeb, 0xf2 },
	{ 0xb8, 0xb8, 0xb8 },
	{ 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x00 },
};
