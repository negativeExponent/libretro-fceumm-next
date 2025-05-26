# FCEUmm Next
`FCEUmm Next` is a **modified libretro core** originally based on [libretro/fceumm](https://github.com/libretro/libretro-fceumm), now evolved with significant custom changes and enhancements. It’s designed to be used with RetroArch or any libretro-compatible frontend.

FCEUmm is a Nintendo Entertainment System / Famicom emulator. It currently supports a wide number of carts, including support for Family Computer Disk System and Vs. Unisystem.

It is a fork of FCEU "mappers modified", an unofficial build of FCEU Ultra by CaH4e3, which supports a lot of new mappers including some obscure mappers such as one for unlicensed NES ROM's using the Libretro API.

**What this fork is**

FCEUmm Next aims to provide faster updates, new features, and better integration with the Libretro API. It includes a complete rewrite of the mapper interface and updates to some code sections that required improvement. The goal is to retain a consistent code-style, enhance the existing framework while ensuring seamless integration with the Libretro ecosystem, among other changes.

These updates were initially intended to be merged into the main fork, libretro-fceumm. However, when I submitted my PRs, they were rejected without clear explanations or meaningful feedback. The concerns raised — particularly about multicart support — were vague, with no actionable details provided.

Since the regressions were likely fixable if the issues had been communicated clearly, and because anything already existing in libretro-fceumm could be moved or added to FCEUmm Next without major difficulty, the lack of concrete information made progress impossible. Despite this, the same changes and concepts I had initially submitted were later submitted and merged, without any explanation as to why my contributions were rejected in the first place.

With no proper communication, or transparency, I decided to create FCEUmm Next to continue pushing forward with the necessary updates and features that were blocked or delayed in the original project.

**Changes/Differences**

------

* additional mappers support
* mapper fixes and updates
* Refactored common mappers (MMC1, MMC3, VRC24 and others) into a modular and reusable interface
* Refactored common expansion audio (FDS, MMC5, VRC-series and others) into a modular and reusable interface
* additional input options (SNES Mouse, SNES Gamepad, PowerPad A/B, VirtualBoy Controller etc)
* variable overscan cropping options, including separate top, bottom, left, right overscan cropping
* replaced on/off audio options to volume controls
* add volume controls for expansion audio (FDS, MMC5, VRC6, VRC7, Namco163, Sunsoft5B)
* fix to audio controls not muting some channels when in low quality mode
* Fix low volume issue
* update VRC7 sound code (emu2413)
* replace Sunsoft 5B sound code with emu2149 for better accuracy (including envelop and noise emulation)
* rewrite Namco 163 sound code
* rewrite VRC6 sound code
* rewrite FDS sound code
* add missing state variables, fixing runahead compatibility
* Assign F12 as Hard Reset (PowerNES) hotkey
* Write instructions now update the databus (backport https://github.com/TASEmulators/fceux/pull/659)
* Apply bisqwit's deemphasis method
* Add support for 512-palettes and apply bisqwit's deemphasis (fceux)
* Fix possible buffer overflow caused by emphasis buffer not initialized to zero
* Vs. System rework, fixes input issues and missing palettes, reworked inputs, update preexisting database and other updates.

* Use NES 2.0 info for supported input types (when available)
* Propagate FCEU_MemoryRand() to initialize mapper WRAM,CHRRAM and other cartridge memory types.
* FCEU_gmalloc default init state is based on FCEU_MemoryRand() settings.
* NSF: Fix waveform visualizer
* NSF: add support for multiple audio chip (wip)
* Add NSFE support
* use 32 bpp color format by default, with optional 16 if one desires on compile time. PSP/3DS targets by default uses 16 bpp. PS2 remains to use ARGB1555.
* misc changes under the hood (libretro, sound, etc)


**Button/Key Changes/Additions:**

------

* F12 - Hard-Reset
* L1 - Change Disk Side in FDS
* R1 - Insert/Eject Disk in FDS
* L2/R2 - Insert Coins for slot 1/2 respectively in Vs. Unisystem and some coin-operated mappers.
* holding L2 and pressing LEFT/RIGHT will change internal palette
* L3 - simulate Famicom Player 2 Microphone (probably)

**Compiling:**

------

* Linux:
    make clean && make

* Windows (MSYS2):
    make clean && make


**Input Devices**

------

* Standard Joypad (4-players)
* NES Zapper
* SNES Pad
* SNES Mouse
* Virtual Boy Controller
* Power Pad A/B Mat
* Family Trainer A/B Mat
* Arkanoid Paddle
* Hyper Shot Gun
* Hyper Shot Mat
* Party Tap
* Exciting Boxing Punching Bag

**Additional Notes:**

------

This core is a work-in-progress! Expect breakage or regressions.
With the massive changes, savestates will not be compatible between versions or commits. SRAM or battery saves are mostly compatible.
NES 2.0 headered rom is prefered in most cases.

PS2/PSP targets are special cases and may not include some enhancements and/or maybe break in future (unless someone wants to step in to maintain them) since i dont have a way to test it on my own.

------

emu2413 - https://github.com/digital-sound-antiques/emu2413
emu2149 - https://github.com/digital-sound-antiques/emu2149

Changelog:
----------
25.05.2x
- Mapper rebuilds (start using structs, new naming scheme, etc)
- Fixed mapper 48
- Fixed mapper 19
- Fixed mapper 56 incorrect sprite image caused by
  wrong address mask on register writes (regression).
- Fixed bug in vrc24 module that caused incorrect chr bank data (regression)
- Add helpers to easily set or remove read/write access to cpu pages
- Revert opcode update that broke a few games (opcode 0xAB, 0x9C, 0x9E)

25.04.25
- moved to a standalone repo

25.04.24
- lots of changes to write

24.10.04
- Add option to disable ppu color emphasis
- Fix allocation and deallocation of CHR-RAM

24.10.03
- mmc3: Fix regression in handling 4-screen mirroring (fix Rad Racer 2)
- m064: Rework irq (fix Skull and Bones)

24.10.01
- Add mapper 470 (backport fceumm)

24.09.30
- Add mapper 515
- ps2: Update upload-artifacts to v4
- make x24c02 only read/write state if I2C address match, backport from fceux (fix SD Gundam Gaiden 2)

24.03.15
- Add mapper 284 (UNL-DRIPGAME, only works in newppu for now)

24.03.06
- Rewrite mapper 6 / 8 / 12.1 / 17

24.03.03
- Add mapper 561
- Add mapper 562

24.02.26
- Add New PPU support (backport from FCEUX)
- Fixed mapper 124

24.02.25
- Fix mapper 156
- Fix support for Ys Definitive Edition (romhack)
