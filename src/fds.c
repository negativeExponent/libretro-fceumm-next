/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* dec-12-19                                    */
/* 4024(w), 4025(w), 4031(r) by dink(fbneo)     */

#include <stdlib.h>
#include <string.h>

#include <file/file_path.h>

#include "fceu-types.h"
#include "x6502.h"
#include "fceu.h"
#include "fds.h"
#include "fdssound.h"
#include "sound.h"
#include "general.h"
#include "state.h"
#include "file.h"
#include "fceu-memory.h"
#include "cart.h"
#include "md5.h"

/*	TODO:  Add code to put a delay in between the time a disk is inserted
 *	and the when it can be successfully read/written to.  This should
 *	prevent writes to wrong places OR add code to prevent disk ejects
 *	when the virtual motor is on(mmm...virtual motor).
 */

#define DISK_EJECTED 255
#define DISK_SEEK_IRQ_CYCLES 150

#define BOOT_INSERT_DELAY (60 * 6) /* wait about 6 seconds before inserting disk */
#define DISK_INSERT_DELAY (60 * 1) /* wait 1 second after inserting disks */

#define FORMAT_FDS 0
#define FORMAT_QD  1

#define BYTES_PER_DISK_SIDE_FDS 65500
#define BYTES_PER_DISK_SIDE_QD  65536

static DECLFR(FDSReadReg);
static DECLFW(FDSWriteReg);

static void FDSPower(void);
static void FDSReset(void);
static void FDSClose(void);

static void FDSFix(int a);

enum FDS_DiskBlockIDs {
	DSK_INIT = 0,
	DSK_VOLUME,
	DSK_FILECNT,
	DSK_FILEHDR,
	DSK_FILEDATA
};

static struct _FDS {
	uint32_t total_sides;
	uint32_t boot_delay;
	uint32_t disk_insert_delay;

	uint8_t  *current_disk_ptr;

	int32_t  seek_irq_timer;
	uint8_t  selected_disk, current_disk;

	int32_t  irq_latch, irq_count;
	uint8_t  irq_enabled;
	uint8_t  irq_repeat;

	uint8_t  irq_timer;
	uint8_t  transfer_flag;

	uint8_t  disk_reg_enabled;
	uint8_t  snd_reg_enabled;

	uint8_t  control;       /* 4025(w) control register */
	uint16_t filesize;      /* size of file being read/written */
	uint8_t  blockid;       /* block-id of current block */
	uint16_t blockstart;    /* start-address of current block */
	uint16_t blocklen;      /* length of current block */
	uint16_t blockpos;      /* current address relative to blockstart */
	uint8_t  accessed;      /* disk needs to be accessed at least once before writing */

	struct _disk {
		uint8_t  format;
		uint8_t  no_crc;
		uint32_t bytes_per_size;
		uint8_t  *data[8];
	} disk;
} fds;

typedef struct FDSBlock1 {
	uint32_t position;
	uint8_t  game_name[3 + 1];
	uint8_t  game_version;
	uint8_t  side_number;
	uint8_t  disk_number;
} FDSBlock1;

typedef struct FDSBlock2 {
	uint32_t position;
	uint8_t  file_count; /* 0x01 */
} FDSBlock2;

typedef struct FDSBlock3 {
	uint32_t position;
	uint8_t  file_number;   /* 0x01 */
	uint8_t  file_id;       /* 0x02 */
	uint8_t  file_name[9];  /* 0x03-0x0A */
	uint16_t load_address;  /* 0x0B-0x0C */
	uint16_t file_size;     /* 0x0D-0x0E */
	uint8_t  file_type;     /* 0x0F */
} FDSBlock3;

typedef struct FDSBlock4 {
	uint32_t position;
} FDSBlock4;

typedef struct FDSFileEntry {
	FDSBlock3 header_block;
	FDSBlock4 data_block;
} FDSFileEntry;

#define MAX_FILES 64

typedef struct FDSInfo {
	FDSBlock1 volume_block;
	FDSBlock2 count_block;
	FDSFileEntry files[MAX_FILES];
	uint32_t total_files;
	uint32_t files_counted;
	uint8_t  game_name[4];
} FDSInfo;

static INLINE void fds_memset(uint8_t *dst, uint8_t value, size_t len) {
	uint32_t i;

	for (i = 0; i < len; i++) {
		(*dst++) = value;
	}
}

static INLINE void fds_memcpy(uint8_t *dst, const uint8_t *src, size_t len) {
	uint32_t i;

	for (i = 0; i < len; i++) {
		(*dst++) = (*src++);
	}
}

static INLINE void fds_memcpy_ascii(uint8_t *dst, const uint8_t *src, size_t len) {
	uint32_t i;

	fds_memset(dst, 0, len);
	for (i = 0; i < (len - 1); i++ ) {
		uint8_t ch = src[i] & 0xFF;

		dst[i] = ((ch >= 0x20) && (ch <= 0x7E)) ? ch : 0x20;
	}
}

static void fds_info_side(uint8_t side, FDSInfo *info) {
	uint32_t pos = 0, filesize = 0;
	const uint8_t *src = fds.disk.data[side];

	fds_memset((uint8_t *)info, 0x00, sizeof(FDSInfo));

	for (pos = 0; pos < fds.disk.bytes_per_size;) {
		uint8_t blockid = src[pos], stop = FALSE;
		uint32_t blocklen = 1;

		switch (blockid) {
		case DSK_VOLUME:   blocklen = 0x38; break;
		case DSK_FILECNT:  blocklen = 0x02; break;
		case DSK_FILEHDR:  blocklen = 0x10; break;
		case DSK_FILEDATA: blocklen = filesize + 1; break;
		default: stop = TRUE; break;
		}

		if (stop) {
			break;
		}

		if (pos + blocklen > fds.disk.bytes_per_size) {
			break;
		}

		if (blockid) {
			const uint32_t file = info->files_counted;

			switch (blockid) {
			case DSK_VOLUME:
				info->volume_block.position = pos;
				fds_memcpy_ascii(&info->volume_block.game_name[0], &src[pos + 0x10], sizeof(info->volume_block.game_name));
				info->volume_block.game_version = src[pos + 0x14];
				info->volume_block.side_number = src[pos + 0x15];
				info->volume_block.disk_number = src[pos + 0x16];
				break;
			case DSK_FILECNT:
				info->count_block.position = pos;
				info->count_block.file_count = src[pos + 1];
				info->files_counted = 0;
				break;
			case DSK_FILEHDR:
				info->files[file].header_block.position = pos;
				info->files[file].header_block.load_address = (src[pos + 0x0C] << 8) | src[pos + 0xD];
				info->files[file].header_block.file_size = (src[pos + 0x0E] << 8) | src[pos + 0x0D];
				fds_memcpy_ascii(&info->files[file].header_block.file_name[0], &src[pos + 0x03], sizeof(info->files[file].header_block.file_name));
				info->files[file].header_block.file_type = src[pos + 0x0F];
				filesize = info->files[file].header_block.file_size;
				break;
			case DSK_FILEDATA:
				info->files[file].data_block.position = pos;
				info->files_counted++;
				break;
			default:
				break;
			}

			pos += blocklen;
			if (!fds.disk.no_crc) {
				pos += 2;
			}
		}
	}
}

static uint16_t gen_qd_crc(uint8_t *data, unsigned size) {
	size_t byte_index, bit_index;
	uint16_t sum = 0x8000;

	for (byte_index = 0; byte_index < size + 2; byte_index++) {
		uint8_t byte = byte_index < size ? data[byte_index] : 0x00;
		for (bit_index = 0; bit_index < 8; bit_index++) {
			uint16_t bit = (byte >> bit_index) & 1;
			uint16_t carry = sum & 1;
			sum = (sum >> 1) | (bit << 15);
			if (carry)
				sum ^= 0x8408;
		}
	}
	return sum;
}

static int fds_build_image_qd(uint8_t *dst, uint8_t *src, int src_is_qd) {
	size_t dest_pos = 0, source_pos = 0, file_count = 0;
	uint16_t crc_read = 0, crc_calc = 0;
	size_t blockstart, blocklen;

	if ((memcmp(&src[0], "\x1*NINTENDO-HVC*", 15))) {
		FCEU_PrintError(" Bios string invalid. (block 1)\n");
		return FALSE;
	}

	/* disk info block (block 1) */
	blockstart = source_pos;
	blocklen = 56;
	fds_memcpy(&dst[dest_pos], &src[source_pos], blocklen);

	dest_pos = blocklen;
	source_pos = blocklen;

	if (src_is_qd == FORMAT_QD) {
		crc_read = (src[source_pos + 1] << 8) + src[source_pos];
		crc_calc = gen_qd_crc(&src[blockstart], blocklen);
		if (crc_read != crc_calc && crc_read != 0) {
			FCEU_printf(" WARN: CRC mismatch at offset 0x%8X, read 0x%02X%02X, "
			            "expected 0x%02X%02X\n",
			            source_pos,
			            crc_read % 256,
			            crc_read / 256,
			            crc_calc % 256,
			            crc_calc / 256);
		}
		fds_memcpy(&dst[dest_pos], (const uint8_t *)&crc_calc, 2);
		source_pos += 2;
		dest_pos += 2;
	} else {
		crc_calc = gen_qd_crc(&src[blockstart], blocklen);
		fds_memcpy(&dst[dest_pos], (const uint8_t *)&crc_calc, 2);
		dest_pos += 2;
	}

	/* file count block (block 2) */
	if (src[source_pos] != 2) {
		FCEU_printf(" WARN: Invalid file amount block (block 2)\n");
		return FALSE;
	}

	blockstart = source_pos;
	blocklen = 2;
	file_count = src[source_pos + 1];
	fds_memcpy(&dst[dest_pos], &src[source_pos], blocklen);

	dest_pos += blocklen;
	source_pos += blocklen;

	if (src_is_qd == FORMAT_QD) {
		crc_read = (src[source_pos + 1] << 8) + src[source_pos];
		crc_calc = gen_qd_crc(&src[blockstart], blocklen);
		if (crc_read != crc_calc && crc_read != 0) {
			FCEU_printf(" WARN: CRC mismatch at offset 0x%8X, read 0x%02X%02X, "
			            "expected 0x%02X%02X\n",
			            source_pos,
			            crc_read % 256,
			            crc_read / 256,
			            crc_calc % 256,
			            crc_calc / 256);
		}
		fds_memcpy(&dst[dest_pos], (const uint8_t *)&crc_calc, 2);
		source_pos += 2;
		dest_pos += 2;
	} else {
		crc_calc = gen_qd_crc(&src[blockstart], blocklen);
		fds_memcpy(&dst[dest_pos], (const uint8_t *)&crc_calc, 2);
		dest_pos += 2;
	}

	while (src[source_pos] == 3) {
		size_t filesize = 0;

		blockstart = source_pos;
		blocklen = 16;
		filesize = (src[source_pos + 14] << 8) | (src[source_pos + 13]);
		fds_memcpy(&dst[dest_pos], &src[source_pos], blocklen);

		dest_pos += blocklen;
		source_pos += blocklen;

		if (src_is_qd == FORMAT_QD) {
			crc_read = (src[source_pos + 1] << 8) + src[source_pos];
			crc_calc = gen_qd_crc(&src[blockstart], blocklen);
			if (crc_read != crc_calc && crc_read != 0) {
				FCEU_printf(
				    " WARN: CRC mismatch at offset 0x%8X, read 0x%02X%02X, "
				    "expected 0x%02X%02X\n",
				    source_pos,
				    crc_read % 256,
				    crc_read / 256,
				    crc_calc % 256,
				    crc_calc / 256);
			}
			fds_memcpy(&dst[dest_pos], (const uint8_t *)&crc_calc, 2);
			source_pos += 2;
			dest_pos += 2;
		} else {
			crc_calc = gen_qd_crc(&src[blockstart], blocklen);
			fds_memcpy(&dst[dest_pos], (const uint8_t *)&crc_calc, 2);
			dest_pos += 2;
		}

		/* file data block (block 4) */
		if (src[source_pos] != 4) {
			FCEU_PrintError(" Error reading block 4\n");
			return FALSE;
		}

		blockstart = source_pos;
		blocklen = 1 + filesize;
		fds_memcpy(&dst[dest_pos], &src[source_pos], blocklen);

		dest_pos += blocklen;
		source_pos += blocklen;

		if (src_is_qd == FORMAT_QD) {
			crc_read = (src[source_pos + 1] << 8) + src[source_pos];
			crc_calc = gen_qd_crc(&src[blockstart], blocklen);
			if (crc_read != crc_calc && crc_read != 0) {
				FCEU_printf(
				    " WARN: CRC mismatch at offset 0x%8X, read 0x%02X%02X, "
				    "expected 0x%02X%02X\n",
				    source_pos,
				    crc_read % 256,
				    crc_read / 256,
				    crc_calc % 256,
				    crc_calc / 256);
			}
			fds_memcpy(&dst[dest_pos], (const uint8_t *)&crc_calc, 2);
			source_pos += 2;
			dest_pos += 2;
		} else {
			crc_calc = gen_qd_crc(&src[blockstart], blocklen);
			fds_memcpy(&dst[dest_pos], (const uint8_t *)&crc_calc, 2);
			dest_pos += 2;
		}
	}

	/* fill remainder with zeros */
	fds_memset(&dst[dest_pos], 0, BYTES_PER_DISK_SIDE_QD - dest_pos);
	return TRUE;
}

static INLINE uint8_t fds_disk_read_byte(uint32_t A) {
	return fds.current_disk_ptr[A];
}

static INLINE void fds_disk_write_byte(uint32_t A, uint8_t V) {
	fds.current_disk_ptr[A] = V;
}

static INLINE void fds_insert_disk(int side) {
	fds.current_disk = side;
	fds.current_disk_ptr = &fds.disk.data[side][0];
}

static INLINE void fds_set_seek_irq_timer(void) {
	fds.seek_irq_timer = DISK_SEEK_IRQ_CYCLES;
}

static void FDSGI(int h) {
	switch (h) {
	case GI_CLOSE:   FDSClose(); break;
	case GI_RESETM2: FDSReset(); break;
	case GI_POWER:   FDSPower(); break;
	}
}

static void FDSStateRestore(int version) {
	uint32_t x;

	setmirror(((fds.control & 8) >> 3) ^ 1);

	for (x = 0; x < fds.total_sides; x++) {
		uint32_t b;
		for (b = 0; b < fds.disk.bytes_per_size; b++) {
			ROM.disk.data[(fds.disk.bytes_per_size * x) + b] ^=
				ROM.disko.data[(fds.disk.bytes_per_size * x) + b];
		}
	}
}

static void FDSReset(void) {
}

static void FDSPower(void) {
	setprg8r(0x10, 0x6000, 0);
	setprg8r(0x10, 0x8000, 1);
	setprg8r(0x10, 0xA000, 2);
	setprg8r(0x10, 0xC000, 3); /* 32K WRAM */
	setprg8(0xE000, 0);        /* BIOS */
	setchr8(0);                /* 8KB CHR RAM */

	setmirror(1);

	MapIRQHook = FDSFix;
	GameStateRestore = FDSStateRestore;

	SetReadHandler(0x4020, 0x5FFF, FDSReadReg);
	SetWriteHandler(0x4020, 0x5FFF, FDSWriteReg);

	SetWriteHandler(0x6000, 0xDFFF, CartBW);
	SetReadHandler(0x6000, 0xFFFF, CartBR);

	FDSSoundRegReset();
	FDSSound_SC();

	fds.disk_reg_enabled = 0;
	fds.seek_irq_timer = 0;

	fds.irq_count = fds.irq_latch = fds.irq_enabled = 0;

	fds.current_disk = DISK_EJECTED;
	fds.selected_disk = 0;

	fds.control = 0;
	fds.filesize = 0;
	fds.blockid = 0;
	fds.blockstart = 0;
	fds.blocklen = 0;
	fds.blockpos = 0;
	fds.accessed = 0;

	fds.irq_timer = 0;
	fds.transfer_flag = 0;

	fds.boot_delay = BOOT_INSERT_DELAY;
}

static INLINE uint8_t FCEU_DiskReady(void) {
	return (fds.current_disk != DISK_EJECTED);
}

void FCEU_FDSEject(void) {
	fds.current_disk = DISK_EJECTED;
}

void FCEU_FDSInsert(int oride) {
	if (!FCEU_DiskReady()) {
		FCEU_DispMessage(RETRO_LOG_INFO, 2000, "Disk %d of %d Side %s Inserted",
			1 + (fds.selected_disk >> 1), (fds.total_sides + 1) >> 1,
			(fds.selected_disk & 1) ? "B" : "A");
		fds.disk_insert_delay = DISK_INSERT_DELAY;
	} else {
		FCEU_DispMessage(RETRO_LOG_INFO, 2000, "Disk %d of %d Side %s Ejected",
			1 + (fds.selected_disk >> 1), (fds.total_sides + 1) >> 1,
			(fds.selected_disk & 1) ? "B" : "A");
		FCEU_FDSEject();
	}
}

void FCEU_FDSSelect(void) {
	if (FCEU_DiskReady()) {
		FCEUD_DispMessage(RETRO_LOG_WARN, 2000, "Eject disk before selecting");
		return;
	}
	fds.selected_disk = ((fds.selected_disk + 1) % fds.total_sides) & 3;
	FCEU_DispMessage(RETRO_LOG_INFO, 2000, "Disk %d of %d Side %s Selected",
		1 + (fds.selected_disk >> 1), (fds.total_sides + 1) >> 1,
		(fds.selected_disk & 1) ? "B" : "A");
}

extern int scanline;
static void FDSFix(int a) {
	if (fds.irq_enabled && (fds.irq_count > 0)) {
		fds.irq_count -= a;
		if (fds.irq_count <= 0) {
			fds.irq_timer = 0x01;
			X6502_IRQBegin(FCEU_IQEXT);
			if (fds.irq_repeat) {
				fds.irq_count = fds.irq_latch;
			}
		}
	}

	if (fds.seek_irq_timer > 0) {
		fds.seek_irq_timer -= a;
		if (fds.seek_irq_timer <= 0) {
			if (scanline == 240) {
				fds.transfer_flag = 0x02;
			}
			if (fds.control & 0x80) {
				fds.transfer_flag = 0x02;
				X6502_IRQBegin(FCEU_IQEXT2);
			}
		}
	}
}

static DECLFR(FDSSndRead) {
	if ((A >= 0x4040) && (A <= 0x407F)) {
		return FDSWaveRead(A);
	}

	switch (A) {
	case 0x4090: return FDSEnvVolumeRead(A);
	case 0x4092: return FDSEnvModRead(A);
	}

	return cpu.openbus;
}

static DECLFW(FDSSndWrite) {
	if ((A >= 0x4040) && (A <= 0x407F)) {
		FDSWaveWrite(A, V);
		return;
	}

	switch (A) {
	case 0x4080: FDSSReg0Write(A, V); break;
	case 0x4082: FDSSReg1Write(A, V); break;
	case 0x4083: FDSSReg2Write(A, V); break;
	case 0x4084: FDSSReg3Write(A, V); break;
	case 0x4085: FDSSReg4Write(A, V); break;
	case 0x4086: FDSSReg5Write(A, V); break;
	case 0x4087: FDSSReg6Write(A, V); break;
	case 0x4088: FDSSReg7Write(A, V); break;
	case 0x4089: FDSSReg8Write(A, V); break;
	case 0x408A: FDSSReg9Write(A, V); break;
	}
}

static DECLFR(FDSReadReg) {
	uint8_t ret = cpu.openbus;
	int i;

	if (fds.snd_reg_enabled && (A >= 0x4040) && (A <= 0x4097)) {
		return FDSSndRead(A);
	}

	if (!fds.disk_reg_enabled) {
		return cpu.openbus;
	}

	switch (A) {
	case 0x4030:
		ret  = fds.irq_timer;
		ret |= fds.transfer_flag;
		ret |= fds.control & 0x08;

		fds.irq_timer = fds.transfer_flag = 0;
		X6502_IRQEnd(FCEU_IQEXT);
		X6502_IRQEnd(FCEU_IQEXT2);

		return ret;

	case 0x4031:
		ret = 0xFF;
		if (FCEU_DiskReady() && (fds.control & 0x04)) {
			fds.accessed = 1;

			ret = 0;

			if (fds.blockpos < fds.blocklen) {
				ret = fds_disk_read_byte(fds.blockstart + fds.blockpos);
				switch (fds.blockid) {
				case DSK_FILEHDR:
					switch (fds.blockpos) {
					case 13: fds.filesize = ret; break;
					case 14: fds.filesize |= (ret << 8); break;
					}
					break;
				}
				fds.blockpos++;
			}
			fds_set_seek_irq_timer();
			fds.transfer_flag = 0;
			X6502_IRQEnd(FCEU_IQEXT2);
		}
		return ret;

	case 0x4032:
		ret &= 0xF8;
		if (!FCEU_DiskReady()) {
			ret |= 0x07;
		} else {
			ret |= (!(fds.control & 0x01) || (fds.control & 0x02)) ? 0x02 : 0;
		}
		return ret;

	case 0x4033:
		return 0x80;
	}

	return cpu.openbus;
}

static DECLFW(FDSWriteReg) {
	if (fds.snd_reg_enabled && (A >= 0x4040) && (A <= 0x4097)) {
		FDSSndWrite(A, V);
		return;
	}

	if (!fds.disk_reg_enabled && (A >= 0x4024)) {
		return;
	}

	switch (A) {
	case 0x4020:
		fds.irq_latch = (fds.irq_latch & 0xFF00) | (V & 0xFF);
		break;
	
	case 0x4021:
		fds.irq_latch = (fds.irq_latch & 0x00FF) | (V << 8);
		break;
	
	case 0x4022:
		fds.irq_enabled = (fds.disk_reg_enabled && (V & 0x02) == 0x02);
		fds.irq_repeat = ((V & 0x01) == 0x01);
		if (fds.irq_enabled) {
			fds.irq_count = fds.irq_latch;
		} else {
			fds.irq_timer = 0;
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	
	case 0x4023:
		fds.disk_reg_enabled = (V & 0x01) == 0x01;
		fds.snd_reg_enabled = (V & 0x02) == 0x02;

		if (!fds.disk_reg_enabled) {
			fds.irq_enabled = 0;
			fds.irq_timer = fds.transfer_flag = 0;
			X6502_IRQEnd(FCEU_IQEXT);
			X6502_IRQEnd(FCEU_IQEXT2);
		}
		break;
	
	case 0x4024:
		if (FCEU_DiskReady() && (~fds.control & 0x04)) {

			if (fds.accessed == 0) {
				fds.accessed = 1;
				break;
			}

			if (fds.blockpos < fds.blocklen) {
				fds_disk_write_byte(fds.blockstart + fds.blockpos, V);
				switch (fds.blockid) {
				case DSK_FILEHDR:
					switch (fds.blockpos) {
					case 13: fds.filesize = V; break;
					case 14: fds.filesize |= V << 8; break;
					}
					break;
				}
				fds.blockpos++;
			}
			fds_set_seek_irq_timer();
			fds.transfer_flag = 0;
			X6502_IRQEnd(FCEU_IQEXT2);
		}
		break;

	case 0x4025:
		if (FCEU_DiskReady()) {
			if ((V & 0x40) && !(fds.control & 0x40)) {
				fds.accessed = 0;

				fds_set_seek_irq_timer();

				if (!fds.disk.no_crc && (fds.blockid >= DSK_VOLUME)) {
					/* In QD format (when CRC is enabled), each block is
					 * followed by a 2-byte CRC. After reading a block, we skip
					 * these 2 bytes to align with the next block start. */
					fds.blockpos += 2;
				}

				/* blockstart  - address of block on disk */
				/* diskaddr    - address relative to blockstart */
				fds.blockstart += fds.blockpos;
				fds.blockpos = 0;

				fds.blockid++;
				if (fds.blockid > DSK_FILEDATA) {
					fds.blockid = DSK_FILEHDR;
				}

				switch (fds.blockid) {
				case DSK_VOLUME: fds.blocklen = 0x38; break;
				case DSK_FILECNT: fds.blocklen = 0x02; break;
				case DSK_FILEHDR: fds.blocklen = 0x10; break;
				case DSK_FILEDATA: fds.blocklen = fds.filesize + 1; break;
				}
			}
			
			if (V & 0x02) { /* transfer reset */
				fds.blockid = DSK_INIT;
				fds.blockstart = 0;
				fds.blocklen = 0;
				fds.blockpos = 0;
				fds_set_seek_irq_timer();
			}
			if (V & 0x40) {
				fds_set_seek_irq_timer();
			}
		}

		fds.transfer_flag = 0;
		X6502_IRQEnd(FCEU_IQEXT2);
		fds.control = V;
		setmirror(((V >> 3) & 0x01) ^ 0x01);
		break;
	}
}

struct codes_t {
	uint8_t code;
	char name[50];
} list[] = {
	{ 0x01, "Nintendo" },
	/* { 0x02, "Rocket Games" }, */
	{ 0x07, "Nomora Securities (unverified)" },
	{ 0x08, "Capcom" },
	{ 0x09, "Hot B Co." },
	{ 0x0A, "Jaleco" },
	{ 0x0B, "Coconuts Japan" },
	{ 0x0C, "Coconuts Japan/G.X.Media" },
	{ 0x13, "Electronic Arts Japan" },
	{ 0x18, "Hudson Soft Japan" },
	/* { 0x19, "S.C.P." }, */
	/* { 0x1A, "Yonoman" }, */
	/* { 0x20, "Destination Software" }, */
	{ 0x21, "Tokai Engineering" },
	/* { 0x22, "VR 1 Japan" }, */
	/* { 0x25, "San-X" }, */
	{ 0x28, "Kemco Japan" },
	{ 0x29, "SETA (Japan)" },
	{ 0x2B, "Tamtex" },
	{ 0x35, "Hector Playing Interface (Hect)" },
	/*{ 0x36, "Codemasters" },*/
	/*{ 0x37, "GAGA Communications" },*/
	/*{ 0x38, "Laguna" },*/
	/*{ 0x39, "Telstar Fun and Games" },*/
	{ 0x3D, "Loriciel" },
	{ 0x3E, "Gremlin" },
	{ 0x40, "Seika Corporation" },
	{ 0x41, "Ubi Soft Entertainment" },
	/*{ 0x42, "Sunsoft" },*/
	{ 0x46, "System 3" },
	/*{ 0x47, "Spectrum Holobyte" },*/
	{ 0x49, "Irem" },
	{ 0x4A, "Gakken" },
	/*{ 0x4D, "Malibu Games" },*/
	/*{ 0x4F, "Eidos/U.S. Gold" },*/
	{ 0x50, "Absolute Entertainment" },
	{ 0x51, "Acclaim" },
	{ 0x52, "Activision" },
	{ 0x53, "American Sammy Corp." },
	/*{ 0x54, "Take 2 Interactive" },*/
	{ 0x54, "Gametek" },
	{ 0x55, "Hi Tech Expressions" },
	{ 0x56, "LJN LTD." },
	{ 0x57, "Matchbox Toys" },
	{ 0x58, "Mattel" },
	{ 0x59, "Mitton Bradley" },
	{ 0x5A, "Mindscape/Software Toolworks" },
	{ 0x5B, "SETA (NA)" },
	{ 0x5C, "Taxan" },
	{ 0x5D, "Tradewest" },
	{ 0x5E, "INTV Corporation" },
	/*{ 0x5F, "American Softworks" },*/
	{ 0x60, "Titus Interactive Studios" },
	{ 0x61, "Virgin Interactive" },
	/*{ 0x62, "Maxis" },*/
	/*{ 0x64, "LucasArts Entertainment" },*/
	{ 0x67, "Ocean" },
	{ 0x69, "Electronic Arts (NA)" },
	{ 0x6B, "Bream Software" },
	{ 0x6E, "Elite Systems Ltd." },
	{ 0x6F, "Electro Brain" },
	{ 0x70, "Infogrames" },
	/*{ 0x71, "Interplay" },*/
	{ 0x72, "JVC Musical Industries Inc" },
	{ 0x73, "Parker Brothers" },
	{ 0x75, "SCI" },
	{ 0x78, "THQ" },
	{ 0x79, "Accolade" },
	{ 0x7A, "Triffix Ent. Inc." },
	{ 0x7C, "Microprose Software" },
	/*{ 0x7D, "Universal Interactive Studios" },*/
	{ 0x7F, "Kemco" },
	{ 0x80, "Misawa" },
	{ 0x83, "G. Amusements Co." },
	{ 0x85, "G.O 1" },
	{ 0x86, "Tokuma Shoten Intermedia" },
	{ 0x89, "Nihon Maicom Kaihatsu (NMK)" },
	{ 0x8B, "Bulletproof Software" },
	{ 0x8C, "Vic Tokai Inc." },
	{ 0x8D, "Sanritsu" },
	{ 0x8E, "Character Soft" },
	{ 0x8F, "I'Max" },
	/*{ 0x91, "Chun Soft" },*/
	/*{ 0x92, "Video System" },*/
	/*{ 0x93, "BEC" },*/
	{ 0x94, "Toaplan" },
	{ 0x95, "Varie" },
	{ 0x96, "Yonezawa/S'pal" },
	/*{ 0x97, "Kaneko" },*/
	{ 0x99, "Victor Interactive Software" },
	{ 0x9A, "Nichibutsu/Nihon Bussan" },
	{ 0x9B, "Tecmo" },
	{ 0x9C, "Imagineer" },
	{ 0x9E, "Face" },
	/*{ 0x9F, "Nova" },*/
	/*{ 0xA0, "Telenet" },*/
	/*{ 0xA1, "Hori" },*/
	{ 0xA2, "Scorpion Soft " },
	{ 0xA3, "Broderbund" },
	{ 0xA4, "Konami" },
	{ 0xA5, "K. Amusement Leasing Co. (KAC)" },
	{ 0xA6, "Kawada" },
	{ 0xA7, "Takara" },
	{ 0xA8, "Royal Industries" },
	{ 0xA9, "Tecnos Japan Corp." },
	{ 0xAA, "Victor Musical Industries" },
	{ 0xAB, "	Hi-Score Media Work" },
	{ 0xAC, "Toei Animation" },
	{ 0xAD, "Toho" },
	{ 0xAE, "TSS" },
	{ 0xAF, "Namco" },
	{ 0xB0, "Acclaim Japan" },
	{ 0xB1, "ASCII" },
	{ 0xB2, "Bandai" },
	{ 0xB3, "Soft Pro Inc." },
	{ 0xB4, "Enix" },
	{ 0xB5, "dB-SOFT" },
	{ 0xB6, "HAL Laboratory" },
	{ 0xB7, "SNK" },
	{ 0xB9, "Pony Canyon Hanbai" },
	{ 0xBA, "Culture Brain" },
	{ 0xBB, "Sunsoft" },
	{ 0xBC, "Toshiba EMI" },
	/*{ 0xBD, "Sony Imagesoft" },*/
	{ 0xBF, "Sammy" },
	{ 0xC0, "Taito" },
	{ 0xC1, "Sunsoft / Ask Co., Ltd." },
	{ 0xC2, "Kemco" },
	{ 0xC3, "Square / Disk Original Group (DOG)" },
	{ 0xC4, "Tokuma Shoten " },
	{ 0xC5, "Data East" },
	{ 0xC6, "Tonkin House" },
	{ 0xC7, "East Cube" },
	{ 0xC8, "Koei" },
	{ 0xC9, "UPL" },
	{ 0xCA, "Konami/Palcom/Ultra" },
	{ 0xCB, "Vapinc/NTVIC" },
	{ 0xCC, "Use Co.,Ltd." },
	{ 0xCD, "Meldac" },
	{ 0xCE, "FCI/Pony Canyon" },
	{ 0xCF, "Angel" },
	{ 0xD0, "Disco" },
	{ 0xD1, "Sofel" },
	{ 0xD2, "Bothtec, Inc." },
	{ 0xD3, "Sigma Enterprises" },
	{ 0xD4, "Ask Kodansa" },
	{ 0xD5, "Kyugo Trading Co." },
	{ 0xD6, "Naxat Soft / Kaga Tech" },
	/*{ 0xD7, "Copya System" },*/
	{ 0xD9, "Banpresto" },
	{ 0xDA, "TOMY" },
	{ 0xDB, "Hiro Co., Ltd." },
	{ 0xDD, "Nippon Computer Systems (NCS) / Masaya Games" },
	{ 0xDF, "Altron Corporation" },
	{ 0xE0, "K.K. DCE" },
	{ 0xE1, "Towa Chiki" },
	{ 0xE2, "Yutaka" },
	/* { 0xE3, "Varie" }, */
	{ 0xE3, "Kaken Corporation" },
	{ 0xE5, "Epoch" },
	{ 0xE7, "Athena" },
	{ 0xE8, "Asmik Ace Entertainment Inc." },
	{ 0xE9, "Natsume" },
	{ 0xEA, "King Records" },
	{ 0xEB, "Atlus" },
	{ 0xEC, "Epic/Sony Records" },
	/*{ 0xEE, "IGS" },*/
	{ 0xEF, "Fujimic" },
	{ 0xF0, "A Wave" },
	{ 0 },
};

static const char *GetCode(uint8_t code) {
	int x = 0;
	char *ret = "unlicensed";

	while (list[x].code != 0) {
		if (list[x].code == code) {
			ret = list[x].name;
			break;
		}
		x++;
	}

	return ret;
}

static void FreeFDSMemory(void) {
	if (ROM.disk.data) {
		free(ROM.disk.data);
	}
	ROM.disk.data = NULL;
	if (ROM.prg.data) {
		free(ROM.prg.data);
	}
	ROM.prg.data = NULL;
	if (WRAM) {
		free(WRAM);
	}
	WRAM = NULL;
	if (CHRRAM) {
		free(CHRRAM);
	}
	CHRRAM = NULL;
}

static int SubLoad(FCEUFILE *fp) {
	struct md5_context md5;
	int length = 0;
	int format = 0;
	int total_sides = 0;
	int i;
	uint8_t buffer[BYTES_PER_DISK_SIDE_QD];

	if (FCEU_fseek(fp, 0, SEEK_END) != 0) {
		return FALSE;
	}
	length = FCEU_fgetsize(fp);
	if (FCEU_fseek(fp, 0, SEEK_SET) != 0) {
		return FALSE;
	}
	if ((length % BYTES_PER_DISK_SIDE_QD) == 0) {
		format = FORMAT_QD;
		total_sides = length / BYTES_PER_DISK_SIDE_QD;
		FCEU_printf(" Disk Format  : QD\n");
	} else if ((length % BYTES_PER_DISK_SIDE_FDS) == 0) {
		format = FORMAT_FDS;
		total_sides = length / BYTES_PER_DISK_SIDE_FDS;
		FCEU_printf(" Disk Format  : FDS\n");
	} else if ((length % BYTES_PER_DISK_SIDE_FDS) == 16) {
		if (FCEU_fseek(fp, 16, SEEK_SET) != 0) {
			return FALSE;
		}
		length = FCEU_fgetsize(fp);
		format = FORMAT_FDS;
		total_sides = length / BYTES_PER_DISK_SIDE_FDS;
		FCEU_printf(" Disk Format  : fwNES FDS\n");
	} else {
		FCEU_PrintError(" Image is not in qd/fds format");
		return FALSE;
	}

	ROM.disk.size = total_sides * BYTES_PER_DISK_SIDE_QD;
	ROM.disk.data = (uint8_t *)FCEU_malloc(ROM.disk.size);

	if (format == FORMAT_QD) {
		for (i = 0; i < total_sides; i++) {
			fds_memset(&buffer[0], 0, sizeof(buffer));
			if (!FCEU_fread(buffer, BYTES_PER_DISK_SIDE_QD, 1, fp)) {
				goto error;
			}
			if (!fds_build_image_qd(&ROM.disk.data[i * BYTES_PER_DISK_SIDE_QD], buffer, TRUE)) {
				goto error;
			}
		}
	} else if (format == FORMAT_FDS) {
		for (i = 0; i < total_sides; i++) {
			fds_memset(&buffer[0], 0, sizeof(buffer));
			FCEU_fread(buffer, BYTES_PER_DISK_SIDE_FDS, 1, fp);
			if (!fds_build_image_qd(&ROM.disk.data[i * BYTES_PER_DISK_SIDE_QD], buffer, FALSE)) {
				goto error;
			}
		}
	}

	md5_starts(&md5);
	for (i = 0; i < total_sides; i++) {
		fds.disk.data[i] = &ROM.disk.data[i * BYTES_PER_DISK_SIDE_QD];
		md5_update(&md5, fds.disk.data[i], BYTES_PER_DISK_SIDE_QD);
	}
	md5_finish(&md5, GameInfo->MD5);

	fds.disk.format = FORMAT_QD;
	fds.total_sides = total_sides;
	fds.disk.no_crc = 0;
	fds.disk.bytes_per_size = BYTES_PER_DISK_SIDE_QD;

	return TRUE;

error:
	if (ROM.disk.data) {
		FCEU_free(ROM.disk.data);
	}
	FCEU_PrintError(" An error has occured creating disk image!\n");
	return FALSE;
}

static void PreSave(void) {
	uint32_t x;
	for (x = 0; x < fds.total_sides; x++) {
		uint32_t b;
		for (b = 0; b < fds.disk.bytes_per_size; b++) {
			ROM.disk.data[(fds.disk.bytes_per_size * x) + b] ^= ROM.disko.data[(fds.disk.bytes_per_size * x) + b];
		}
	}
}

static void PostSave(void) {
	uint32_t x;
	for (x = 0; x < fds.total_sides; x++) {
		uint32_t b;
		for (b = 0; b < fds.disk.bytes_per_size; b++) {
			ROM.disk.data[(fds.disk.bytes_per_size * x) + b] ^= ROM.disko.data[(fds.disk.bytes_per_size * x) + b];
		}
	}
}

static FCEUFILE *LoadBIOS(const char *name) {
	FCEUFILE *tmp = NULL;
	char *fn = FCEU_MakeFName(FCEUMKF_FDSROM, 0, 0);

	if (!(tmp = FCEU_fopen(fn, NULL, 0))) {
		FCEU_PrintError("FDS BIOS ROM image missing!\n");
		FCEUD_DispMessage(
			RETRO_LOG_ERROR, 3000, "FDS BIOS image (disksys.rom) missing");
		FCEU_free(fn);
		return FALSE;
	}

	free(fn);
	return (tmp);
}

int FDSLoad(const char *name, FCEUFILE *fp) {
	FCEUFILE *biosfile;

	if (!(biosfile = LoadBIOS(name))) {
		return FALSE;
	}

	FreeFDSMemory();

	ResetCartMapping();

	fds_memset((uint8_t *)&fds, 0, sizeof(fds));

	ROM.prg.size = 8192;
	ROM.prg.data = (uint8_t *)FCEU_gmalloc(ROM.prg.size);
	SetupCartPRGMapping(0, ROM.prg.data, ROM.prg.size, 0);

	if (FCEU_fread(ROM.prg.data, 1, ROM.prg.size, biosfile) != ROM.prg.size) {
		if (ROM.prg.data) {
			free(ROM.prg.data);
		}
		ROM.prg.data = NULL;
		FCEU_fclose(biosfile);
		FCEU_PrintError("Error reading FDS BIOS ROM image.\n");
		FCEUD_DispMessage(RETRO_LOG_ERROR, 3000, "Error reading FDS BIOS image (disksys.rom)");
		return FALSE;
	}

	FCEU_fclose(biosfile);

	FCEU_fseek(fp, 0, SEEK_SET);

	if (!SubLoad(fp)) {
		if (ROM.prg.data) {
			free(ROM.prg.data);
		}
		ROM.prg.data = NULL;
		return FALSE;
	}

	/* Original disk data backup, to help in creating save states. */
	ROM.disko.size = ROM.disk.size;
	ROM.disko.data = (uint8_t *)FCEU_malloc(ROM.disko.size);
	fds_memcpy(ROM.disko.data, ROM.disk.data, ROM.disk.size);

	GameInfo->type = GIT_FDS;
	GameInterface = FDSGI;

	fds.current_disk = 0;
	fds.selected_disk = 0;

	ResetExState(PreSave, PostSave);

	AddExState(ROM.disk.data, ROM.disk.size, 0, "DDTA");

	AddExState(&fds.disk_reg_enabled, 1, 0, "DREG");
	AddExState(&fds.snd_reg_enabled, 1, 0, "SREG");
	AddExState(&fds.irq_count, 4 | FCEUSTATE_RLSB, 1, "IRQC");
	AddExState(&fds.irq_latch, 4 | FCEUSTATE_RLSB, 1, "IQL1");
	AddExState(&fds.irq_enabled, 1, 0, "IRQA");
	AddExState(&fds.seek_irq_timer, 4 | FCEUSTATE_RLSB, 1, "DSIR");
	AddExState(&fds.selected_disk, 1, 0, "SELD");
	AddExState(&fds.current_disk, 1, 0, "INDI");

	AddExState(&fds.control, 1, 0, "CTRG");
	AddExState(&fds.filesize, 2 | FCEUSTATE_RLSB, 1, "FLSZ");
	AddExState(&fds.blockid, 1, 0, "BLCK");
	AddExState(&fds.blockstart, 2 | FCEUSTATE_RLSB, 1, "BLKS");
	AddExState(&fds.blocklen, 2 | FCEUSTATE_RLSB, 1, "BLKL");
	AddExState(&fds.blockpos, 2 | FCEUSTATE_RLSB, 1, "DADR");
	AddExState(&fds.accessed, 1, 0, "DACC");

	AddExState(&fds.irq_timer, 1, 0, "IRQt");
	AddExState(&fds.transfer_flag, 1, 0, "TFLG");

	AddExState(&fds.boot_delay, 4 | FCEUSTATE_RLSB, 1, "BDLY");
	AddExState(&fds.disk_insert_delay, 4 | FCEUSTATE_RLSB, 1, "DDLY");

	FDSSound_AddStateInfo();

	CHRRAMSIZE = 8192;
	CHRRAM = (uint8_t *)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0, CHRRAM, CHRRAMSIZE, 1);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CHRR");

	WRAMSIZE = 32768;
	WRAM = (uint8_t *)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "FDSR");

	setmirror(MI_H);

	FCEUI_SetVidSystem(0);

	{
		int i;
		uint32_t side;

		FCEU_printf(" Code         : %02X\n", ROM.disk.data[0x0f]);
		FCEU_printf(" Manufacturer : %s\n", GetCode(ROM.disk.data[0x0f]));
		FCEU_printf(" Total Sides  : %d\n", fds.total_sides);
		FCEU_printf(" ROM MD5      : 0x%s\n", md5_asciistr(GameInfo->MD5));
		FCEU_printf(" Filename     : %s\n", path_remove_extension((char *)path_basename(name)));
		/* FCEU_printf(" Disk Format  : %s\n", fds.disk.format == FORMAT_QD ? "QD" : "FDS"); */

		for (side = 0; side < fds.total_sides; side++) {
			FDSInfo info;

			fds_info_side(side, &info);

			FCEU_printf(" ===\n");
			FCEU_printf(" Side  %c      : disk %d, side %c, name %-3s, version %d, files = %d\n",
				side + 'A',
				info.volume_block.disk_number,
				info.volume_block.side_number + 'A',
				info.volume_block.game_name,
				info.volume_block.game_version,
				info.count_block.file_count);

			FCEU_printf("  Block   1   : (0x%04X - 0x%04X)\n",
				info.volume_block.position,
				info.volume_block.position + 0x38 + (!fds.disk.no_crc * 2) - 1);

			FCEU_printf("  Block   2   : (0x%04X - 0x%04X)\n",
				info.count_block.position,
				info.count_block.position + 0x02 + (!fds.disk.no_crc * 2) - 1);

			for (i = 0; i < info.count_block.file_count; i++) {
				const char *ftype[3] = { "PRAM", "CRAM", "VRAM" };
				int hdr_start = info.files[i].header_block.position;
				int hdr_end   = hdr_start + 0x10 + (!fds.disk.no_crc * 2);
				int data_start = info.files[i].data_block.position;
				int data_end   = data_start + info.files[i].header_block.file_size + 1 + (!fds.disk.no_crc * 2) - 1;

				FCEU_printf("  Header %2d   : (0x%04X - 0x%04X)  type: %-8s  load: $%04X\n",
					i, hdr_start, hdr_end,
					ftype[info.files[i].header_block.file_type],
					info.files[i].header_block.load_address);

				FCEU_printf("  File   %2d   : (0x%04X - 0x%04X)  name: %-8s  size: %d bytes\n",
					i, data_start, data_end,
					info.files[i].header_block.file_name,
					info.files[i].header_block.file_size);
			}

		}
		FCEU_printf(" ===\n");
	}

	return TRUE;
}

void FDSClose(void) {
	if (ROM.disko.data) {
		free(ROM.disko.data);
		ROM.disko.data = NULL;
	}
	FreeFDSMemory();
}

uint8_t *FDSROM_ptr(void) {
	return (ROM.disk.data);
}

uint32_t FDSROM_size(void) {
	return (ROM.disk.size);
}

/* run on every frame */
void FDSFrameCycle(void) {
	if (!FCEU_DiskReady() && fds.boot_delay) {
		fds.boot_delay--;
		if (fds.boot_delay == 0) {
			FCEU_FDSInsert(0);
		}
	}

	if (!FCEU_DiskReady() && fds.disk_insert_delay) {
		fds.disk_insert_delay--;
		if (fds.disk_insert_delay == 0) {
			fds_insert_disk(fds.selected_disk);
		}
	}
}
