#ifndef _HGX_H
#define _HGX_H

#include "cs2.h"
#include "bitBuffer.h"

typedef struct _HGxHeader {
		uchar magic[4];
		ulong unknown[3];
		ulong entries;
} HGxHeader;

typedef struct _HG3Tag {
	uchar name[8];
	ulong offsetNextTag;
	ulong length;
} HG3Tag;

typedef struct _HG3StdInfo {
	ulong width;
	ulong height;
	ulong bpp;
	ulong offsetX;
	ulong offsetY;
	ulong totalWidth;
	ulong totalHeight;
	ulong unknown[3];
} HG3StdInfo;

typedef struct _HG3ImgHeader {
	ulong unknown;
	ulong height;
	ulong compressedLen;
	ulong originalLen;
	ulong cmdLen;
	ulong originalCmdLen;
} HG3ImgHeader;

int openHGx(const char *filename);
void readHG3Image(int fd, HG3StdInfo stdInfo, HG3ImgHeader imgHdr, char *baseName);
uchar *unRLE(uchar *buff, ulong buffLen, uchar *cmdBuff, ulong cmdLen, ulong *outLen);
void unDelta(uchar *buff, ulong buffLen, ulong width, ulong height, ulong Bpp, uchar *RGBABuff);
void saveImage(uchar *RGBABuffer, ulong width, ulong height, const char *filename);

#endif //_HGX_H
