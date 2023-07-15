#ifndef _MMC5_AUDIO_H
#define _MMC5_AUDIO_H

typedef struct __MMC5SOUND {
	uint16 wl[2];
	uint8 env[2];
	uint8 enable;
	uint8 running;
	uint8 raw;
	uint8 rawcontrol;
	int32 dcount[2];
	int32 BC[3];
	int32 vcount[2];
} MMC5SOUND;

extern MMC5SOUND MMC5Sound;

DECLFW(MMC5Sound_Write);

void MMC5Sound_ESI(void);

#endif /* _MMC5_AUDIO_H */
