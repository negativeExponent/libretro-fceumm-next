/* FCE Ultra - NES/Famicom Emulator
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include "fceu-types.h"
#include "file.h"
#include "fceu-endian.h"
#include "fceu-memory.h"
#include "driver.h"
#include "general.h"

static MEMWRAP *MakeMemWrap(FILE *tz) {
	MEMWRAP *tmp = NULL;

	if (!(tmp = (MEMWRAP *)FCEU_malloc(sizeof(MEMWRAP)))) {
		goto doret;
   }
	tmp->location = 0;

	fseek(tz, 0, SEEK_END);
	tmp->size = ftell(tz);
	fseek(tz, 0, SEEK_SET);

	if (!(tmp->data_int = (uint8_t *)FCEU_malloc(tmp->size))) {
		FCEU_free(tmp);
		tmp = NULL;
		goto doret;
	}

	fread(tmp->data_int, 1, tmp->size, tz);
	tmp->data = tmp->data_int;

doret:
	return tmp;
}

static MEMWRAP *MakeMemWrapBuffer(const uint8_t *buffer, size_t bufsize) {
	MEMWRAP *tmp = (MEMWRAP *)FCEU_malloc(sizeof(MEMWRAP));

	if (!tmp) {
		return NULL;
   }

	tmp->location = 0;
	tmp->size = bufsize;
	tmp->data_int = NULL;
	tmp->data = buffer;

	return tmp;
}

FCEUFILE *FCEU_fopen(const char *path, const uint8_t *buffer, size_t bufsize) {
	FCEUFILE *fceufp = (FCEUFILE *)FCEU_malloc(sizeof(FCEUFILE));
	FILE *t = NULL;

	if (!fceufp) {
		return NULL;
	}

	if (buffer) {
		fceufp->fp = MakeMemWrapBuffer(buffer, bufsize);
		if (!fceufp->fp) {
			FCEU_free(fceufp);
			return NULL;
		}
		return fceufp;
	}

	t = fopen(path, "rb");
	if (t) {
		fceufp->fp = MakeMemWrap(t);
		fclose(t);
		return fceufp;
	}

	FCEU_free(fceufp);
	return NULL;
}

int FCEU_fclose(FCEUFILE *fp) {
	if (!fp) {
		return 0;
   }

	if (fp->fp) {
		if (fp->fp->data_int) {
			FCEU_free(fp->fp->data_int);
      }
		fp->fp->data_int = NULL;

		FCEU_free(fp->fp);
	}
	fp->fp = NULL;

	FCEU_free(fp);
	fp = NULL;

	return 1;
}

uint64_t FCEU_fread(void *ptr, size_t element_size, size_t nmemb, FCEUFILE *fp) {
	uint32_t total = nmemb * element_size;

	if (fp->fp->location >= fp->fp->size) {
		return 0;
   }

	if ((fp->fp->location + total) > fp->fp->size) {
		int64_t ak = fp->fp->size - fp->fp->location;

		memcpy((uint8_t *)ptr, fp->fp->data + fp->fp->location, ak);
		fp->fp->location = fp->fp->size;
		return (ak / element_size);
	}

	memcpy((uint8_t *)ptr, fp->fp->data + fp->fp->location, total);
	fp->fp->location += total;
	return nmemb;
}

int FCEU_fseek(FCEUFILE *fp, long offset, int whence) {
	switch (whence) {
	case SEEK_SET:
		if (offset >= fp->fp->size) {
			return -1;
      }
		fp->fp->location = offset;
		break;
	case SEEK_CUR:
		if ((offset + fp->fp->location) > fp->fp->size) {
			return -1;
      }
		fp->fp->location += offset;
		break;
	}
	return 0;
}

int FCEU_read32le(uint32_t *Bufo, FCEUFILE *fp) {
	if ((fp->fp->location + 4) > fp->fp->size) {
		return 0;
   }
	*Bufo = FCEU_de32lsb(fp->fp->data + fp->fp->location);
	fp->fp->location += 4;
	return 1;
}

int FCEU_fgetc(FCEUFILE *fp) {
	if (fp->fp->location < fp->fp->size) {
		return fp->fp->data[fp->fp->location++];
   }
	return EOF;
}

uint64_t FCEU_ftell(FCEUFILE *fp) {
	return fp->fp->location;
}

uint64_t FCEU_fgetsize(FCEUFILE *fp) {
	return fp->fp->size;
}
