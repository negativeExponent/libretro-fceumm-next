#ifndef _FCEU_VIDEO_H
#define _FCEU_VIDEO_H

extern uint8_t *XBuf;
extern uint8_t *XDBuf;

int FCEU_InitVirtualVideo(void);
void FCEU_KillVirtualVideo(void);
void FCEU_DrawNumberRow(uint8_t *target, int *nstatus, int cur);

#endif
