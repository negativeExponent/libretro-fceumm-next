#ifndef _MMC5_AUDIO_H
#define _MMC5_AUDIO_H

typedef struct MMC5APU {
	uint16 wl[2];
	uint8 env[2];
	uint8 enable;
	uint8 running;
	uint8 raw;
	uint8 rawcontrol;
	int32 dcount[2];
	int32 BC[3];
	int32 vcount[2];
} MMC5APU;

extern MMC5APU MMC5Sound;

DECLFW(Mapper5_SW);
void Mapper5_ESI(void);

#endif /* _MMC5_AUDIO_H */