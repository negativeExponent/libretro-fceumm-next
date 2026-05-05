#ifndef _ZAPPER_H_
#define _ZAPPER_H_

#include "../fceu-types.h"

typedef struct ZAPPER {
	uint32_t mzx, mzy, mzb, mzs;
	int zap_readbit;
	uint8_t bogo;
	int zappo;
	uint64_t zaphit;
} ZAPPER;

extern ZAPPER ZD[2];

#endif
