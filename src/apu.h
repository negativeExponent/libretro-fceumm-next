#ifndef _FCEU_APU_H
#define _FCEU_APU_H

enum FrameSeqMode {
	FrameFourStepMode = 0,
	FrameFiveStepMode = 1
};

enum FrameType {
	FrameNone = 0,
	FrameHalf = 1,
	FrameQuarter = 2
};

enum WavePositionShift {
	SQ_SHIFT		= 24,
	TRINPCM_SHIFT	= 16
};

typedef struct Timer {
	uint16      period;        /* Frequency period for square wave (controls pitch) */

	/* Internal */
	int32       counter;       /* Counter tracking the square wave period (timing of cycles) */
	int32       count2;        /* Shifted period timer for  low-quality mode */

	uint32      cvbc;          /* base counter */
} Timer;

typedef struct LengthCount {
	/* Registers */
	uint8       enabled;        /* set by channel enable flag in 4015 write */
	uint8       halt;           /* Halt flag: if set, disables further counting */

	uint8       counter;        /* Current count value for length counter (counts down) */

	uint8       delayHalt;
	uint8       delayCounter;
	uint8       nextHalt;
	uint8       nextCounter;
} LengthCount;

typedef struct Envelope {
	/* Register */
	uint8       loop;           /* Loop mode flag: if set, envelope restarts after reaching a certain state */
	uint8       constant;       /* Constant mode flag: if set, volume remains constant */
	uint8       speed;          /* Speed of volume decay, affecting the rate of decrease */

	/* internal */
	uint8       decay_volume;   /* Current volume level during the decay phase */
	uint8       counter;        /* Counter tracking the current state of decay process */
	uint8       reload;         /* Flag to reload decay counter (restarts volume decrease) */
} Envelope;

typedef struct Sweep {
	/* Register */
	uint8       enabled;         /* Enable flag for sweep operation (if set, sweep occurs) */
	uint8       period;         /* Sweep period: determines the rate at which frequency is adjusted */
	uint8       negate;         /* Negate flag: if set, subtracts from the frequency during sweep */
	uint8       shift;          /* Number of bits to shift for frequency adjustment in the sweep */

	uint16      pulsePeriod;    /* Period of the pulse being swept (frequency of waveform) */

	/* Internal */
	uint8       id;
	uint8       counter;        /* Counter for sweep timing (decrements until reload) */
	uint8       reload;         /* Reload flag: when set, resets sweep counter to initial value */
} Sweep;

typedef struct SquareUnit {
	/* Registers*/
	uint8       duty;           /* Duty cycle defines the waveform's high-to-low ratio */

	/* Internal */
	uint8       step;           /* Counter for tracking the current phase of the duty cycle */

	LengthCount length;         /* Length counter to manage note duration for square wave */
	Envelope    envelope;       /* Envelope to control volume for square wave */
	Sweep       sweep;          /* Sweep to modify frequency of square wave */
	Timer       timer;          /* Cycle counter and period to reload */
} SquareUnit;

typedef struct TriangleUnit {
	uint8       linearPeriod;   /* Linear length control value for the triangle wave */

	/* Internal */
	uint8       linearCounter;  /* Counter to track linear length during envelope decay */
	uint8       linearReload;   /* Flag to reload the linear length counter */
	uint8       stepCounter;    /* Counter for tracking the steps in the triangle wave's length */

	LengthCount length;         /* Length counter for triangle wave channel (note duration) */
	Timer       timer;          /* Cycle counter and period to reload */
} TriangleUnit;

typedef struct NoiseUnit {
	/* Register */
	uint8       shortMode;      /* Short mode flag for noise wave: alters frequency generation behavior */
	uint8       periodIndex;    /* The period determines how many CPU cycles happen between shift register clocks. */

	/* Internal */
	uint16      shiftRegister;  /* Shift register used to generate noise waveform */

	LengthCount length;         /* Length counter for noise wave channel (note duration) */
	Envelope    envelope;       /* Envelope to control volume for noise wave */
	Timer       timer;          /* Cycle counter and period to reload */
} NoiseUnit;

typedef struct DMCUnit {
	uint8       bitCounter;     /* Bit counter for reading sample data bits (tracks position in current sample) */

	uint8       addressLatch;   /* Address latch for DMC sample address (mapped to $4012) */
	uint8       lengthLatch;    /* Length latch for DMC sample length (mapped to $4013) */
	uint8       periodIndex;    /* The rate determines for how many CPU cycles happen between changes in the output level during automatic delta-encoded sample playback */
	uint8       loop;           /* Loop flag for DMC sample (1 = loop, 0 = no loop) */
	uint8       irqEnabled;     /* IRQ enabled flag: if set, triggers IRQ when sample finishes loading */
	uint8       irqPending;     /* Flag indicating if an IRQ is pending */

	uint16      readAddress;    /* Address to read data from in memory for DMC sample */
	uint16      lengthCounter;  /* Counter for the length of the sample data to play */
	uint8       sampleShiftReg; /* Holds the current sample data during DMA shift processing */

	uint8       dmaBufferValid; /* Flag indicating whether the DMA buffer contains valid data */
	uint8       dmaBuffer;      /* DMA buffer for transferring sample data to audio output */
	uint8       sampleValid;    /* Sample validity flag: indicates if sample data is properly loaded */
	uint8       rawDataLatch;   /* Raw data latch for DMC control (mapped to $4011 0xxxxxxx) */

	Timer       timer;
} DMCUnit;

typedef struct FrameCounter {
	uint8       mode;            /* Mode of the frame counter operation, 1=5-step, 0=4-step */
	uint8       irqInhibit;      /* Flag to inhibit IRQ generation */
	uint8       irqPending;      /* Flag indicating if an IRQ is pending */
	uint8       step;            /* Current step in frame counter operation */

	/* Timers */
	int32       counter;         /* frame cycle counter */

	uint8       delay;           /* Delay after 4017 write */
	uint8       newMode;
} FrameCounter;

static const uint8 lengthtable[0x20] =
{
	0x0A, 0xFE, 0x14, 0x02, 0x28, 0x04, 0x50, 0x06,
	0xa0, 0x08, 0x3c, 0x0a, 0x0e, 0x0c, 0x1a, 0x0e,
	0x0c, 0x10, 0x18, 0x12, 0x30, 0x14, 0x60, 0x16,
	0xc0, 0x18, 0x48, 0x1a, 0x10, 0x1c, 0x20, 0x1E
};

static const uint8 SquareWaveTable[2][4][8] = {
	{
		/* table for normal mode */
		{ 0, 0, 0, 0, 0, 0, 0, 1 }, /* 12.5% */
		{ 0, 0, 0, 0, 0, 0, 1, 1 }, /* 25.0% */
		{ 0, 0, 0, 0, 1, 1, 1, 1 }, /* 50.0% */
		{ 1, 1, 1, 1, 1, 1, 0, 0 }, /* 25.0% (negated) */
	},
	{
		/* table for swapped-duty mode */
		{ 0, 0, 0, 0, 0, 0, 0, 1 }, /* 12.5% */
		{ 0, 0, 0, 0, 1, 1, 1, 1 }, /* 25.0% */
		{ 0, 0, 0, 0, 0, 0, 1, 1 }, /* 50.0% */
		{ 1, 1, 1, 1, 1, 1, 0, 0 }, /* 25.0% (negated) */
	},
};

/**
 * MMC5SoundFrameTick
 * ------------------
 * Step the MMC5 audio unit’s frame-sequencer events. 
 * This should be called once per APU frame tick (240 Hz NTSC / 192 Hz PAL).
 * 
 * Actions performed:
 *  - Clock length counters for both MMC5 pulse channels
 *  - Clock envelopes for both MMC5 pulse channels
 * 
 * Note: This function does NOT generate PCM samples; it only updates counters.
 */
void MMC5SoundFrameTick(void);

extern uint8 isMMC5Audio;

#endif /* _FCEU_APU_H */
