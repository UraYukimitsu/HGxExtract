#ifndef _BITBUFFER_H
#define _BITBUFFER_H

#include "cs2.h"

typedef struct _bitBuffer {
	uchar *buffer;
	ulong len;
	uchar index;
} bitBuffer;

bitBuffer *bitBuffer_new(uchar*, ulong);
uchar getBit(bitBuffer*);
ulong getEliasGamma(bitBuffer*);

#endif //_BITBUFFER_H
