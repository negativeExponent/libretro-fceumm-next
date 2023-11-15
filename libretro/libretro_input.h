#ifndef __LIBRETRO_INPUT_H
#define __LIBRETRO_INPUT_H

#include <libretro.h>

#define MAX_CONTROLLERS 4 /* max supported players */
#define MAX_PORTS       2 /* max controller ports. port 0 for player 1/3, port 1 for player 2/4 */

enum RetroZapperInputModes {
	RetroLightgun,
	RetroSTLightgun,
	RetroMouse,
	RetroPointer
};

enum RetroArkanoidInputModes {
	RetroArkanoidMouse,
	RetroArkanoidPointer,
	RetroArkanoidAbsMouse,
	RetroArkanoidStelladaptor
};

typedef struct {
	bool enable_4player;  /* four-score / 4-player adapter used */
	bool up_down_allowed; /* disabled simultaneous up+down and left+right dpad combinations */
	bool needs_update;

	/* turbo related */
	bool turbo_enabler[MAX_CONTROLLERS];
	int turbo_delay;

	int type[MAX_CONTROLLERS + 1]; /* 4-players + famicom expansion */

	/* input data */
	uint32_t JSReturn; /* player input data, 1 byte per player (1-4) */
	uint32_t JoyButtons[2];
	uint32_t MouseData[MAX_PORTS][4]; /* nes mouse data */
	uint32_t FamicomData[3];          /* Famicom expansion port data */
	uint32_t PowerPadData[2];         /* NES Power Pad data */
	uint32_t FTrainerData;            /* Expansion: Family Trainer Data */
	uint8_t QuizKingData;             /* Expansion: Quiz King Data */

	enum RetroZapperInputModes zapperMode;
	enum RetroArkanoidInputModes arkanoidMode;
	int mouseSensitivity;
} NES_INPUT_T;

extern NES_INPUT_T nes_input;

void input_init_env(retro_environment_t *_environ_cb);
void set_controller_port_device(unsigned port, unsigned device);
void update_input_descriptors(void);
void input_update(retro_input_state_t *input_cb);

extern bool libretro_supports_bitmasks;
extern unsigned palette_switch_counter;
extern bool palette_switch_enabled;

void input_palette_switch(bool, bool);

#endif /* __LIBRETRO_INPUT_H */