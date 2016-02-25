#include "bitBuffer.h"

bitBuffer *bitBuffer_new(buffer, len)
	uchar *buffer;
	ulong len;
{
	bitBuffer *self = malloc(sizeof(bitBuffer));
	self->buffer = buffer;
	self->len = len;
	self->index = 0;
	return self;
}

uchar getBit(self)
	bitBuffer *self;
{
	if(self->index > 7)
	{
		self->index = 0;
		self->buffer++;
	}
	return (*(self->buffer) >> self->index++) & 1;
}

ulong getEliasGamma(self)
	bitBuffer *self;
{
	ulong digits = 0, ret;
	while(!getBit(self)) digits++; //Read the number of 0's
	ret = 1 << digits;
	while(digits--)
		if(getBit(self))
			ret |= 1 << digits;
	return ret;
}
