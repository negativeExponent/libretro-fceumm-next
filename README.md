[![Build Status](https://travis-ci.org/libretro/libretro-fceumm.svg?branch=master)](https://travis-ci.org/libretro/libretro-fceumm)
[![Build status](https://ci.appveyor.com/api/projects/status/etk1vcouybahdbkt/branch/master?svg=true)](https://ci.appveyor.com/project/bparker06/libretro-fceumm/branch/master)

# FCE Ultra mappers modified (FCEUmm)
FCEU "mappers modified" is an unofficial build of FCEU Ultra by CaH4e3, which supports a lot of new mappers including some obscure mappers such as one for unlicensed NES ROM's.

##### Some notable changes and updates (in no particular order):
- additional mappers support
- mapper fixes and updates
- additional input options (SNES Mouse, SNES Gamepad, PowerPad A/B, VirtualBoy Controller etc)
- variable overscan cropping options, including separate top, bottom, left, right overscan cropping
- replaced on/off audio options to volume controls
- add volume controls for expansion audio (FDS, MMC5, VRC6, VRC7, Namco163, Sunsoft5B)
- fix to audio controls not muting some channels when in low quality mode
- add missing state variables, fixing runahead compatibility
- Assign F12 as Hard Reset (PowerNES) hotkey
- Write instructions now update the databus (backport https://github.com/TASEmulators/fceux/pull/659)
- Apply bisqwit's deemphasis method
- support for 512-palettes and apply bisqwit's deemphasis (fceux)
- fix possible buffer overflow caused by emphasis buffer not initialized to zero
- Vs. System rework, fix input issues and missing palettes, reworked inputs, update preexisting database and other updates.
- Fix low volume issue
- Use NES 2.0 info for supported input types (when available)
- Propagate FCEU_MemoryRand() to initialize mapper WRAM,CHRRAM and other cartridge memory types.
- FCEU_gmalloc default init state is based on FCEU_MemoryRand() settings.
- misc changes under the hood (libretro, sound, etc)

##### Button/Key Changes/Additions:
* F12 - Hard-Reset
* L1 - Change Disk Side in FDS
* R1 - Insert/Eject Disk in FDS
* L2/R2 - Insert Coins for slot 1/2 respectively in Vs. Unisystem and some coin-operated mappers.
* holding L2 and pressing LEFT or RIGHT will change internal palette

The core can be compiled similarly to libretro-fceumm. and be a drop-in replacement.

##### NOTE:
This core is a work-in-progress! Expect breakage or regressions.
With the massive changes, savestates will not be compatible between versions or commits. SRAM or battery saves are mostly compatible though unless on eeprom-based saves.
NES 2.0 headered rom is prefered.

PS2/PSP targets are special cases and may not include some enhancements.
