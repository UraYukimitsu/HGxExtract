#include "HGx.h"

ulong iteration = 0;

int openHGx(const char *filename)
{
	int fd;
	char *baseName = NULL;
	HGxHeader    fileHdr;
	HG3Tag       tag;
	HG3StdInfo   stdInfo;
	HG3ImgHeader imgHdr;

	fd = open(filename, O_RDONLY | O_BINARY);
	if(!fd)
	{
		fprintf(stderr, "File %s not found.\n", filename);
		return 1;
	}
	baseName = strdup(filename);
	baseName[strlen(baseName) - 3] = 0;
	iteration = 0;

	read(fd, &fileHdr, sizeof(fileHdr));
	if(!memcmp(fileHdr.magic, "HG-3", 4)) //Seems like a correct HG3 file for now
	{
		do {
			read(fd, &tag, sizeof(tag));
		} while(memcmp(tag.name, "stdinfo", 7));
		if(tag.length != sizeof(stdInfo))
		{
			fprintf(stderr, "stdinfo size mismatch.\nExpected %lu, got %lu.\n", (ulong)sizeof(stdInfo), tag.length);
			return 1;
		}
		read(fd, &stdInfo, sizeof(stdInfo));
		while(tag.offsetNextTag)
		{
			read(fd, &tag, sizeof(tag));
			if(!memcmp(tag.name, "img0000", 7))
			{
				read(fd, &imgHdr, sizeof(imgHdr));
				readHG3Image(fd, stdInfo, imgHdr, baseName);
			} else {
				//Save unknown data to file
			}
		}
	} else {
		fprintf(stderr, "File is not HG3.");
		return 1;
	}
	close(fd);
	return 0;
}

#define smalloc(ptr, size) 	ptr = malloc(size);\
							if(!ptr) exit(1);

void readHG3Image(int fd, HG3StdInfo stdInfo, HG3ImgHeader imgHdr, char *baseName)
{
	uchar *tempBuff = NULL, *buff = NULL, *cmdBuff = NULL, *RGBABuff = NULL, *outBuff = NULL;
	ulong originalLen = imgHdr.originalLen, originalCmdLen = imgHdr.originalCmdLen, outLen;
	int err;
	char outName[255];

	smalloc(tempBuff, imgHdr.compressedLen);
	smalloc(buff, imgHdr.originalLen);
	read(fd, tempBuff, imgHdr.compressedLen);
	err = uncompress(buff, &originalLen, tempBuff, imgHdr.compressedLen);
	if(err != Z_OK)
	{
		fprintf(stderr, "uncompress error on buff\n");
		fprintf(stderr, "%d %d %d %d\n", err, Z_BUF_ERROR, Z_MEM_ERROR, Z_DATA_ERROR);
		exit(1);
	}
	free(tempBuff); tempBuff = NULL;

	smalloc(tempBuff, imgHdr.cmdLen);
	smalloc(cmdBuff, imgHdr.originalCmdLen);
	read(fd, tempBuff, imgHdr.cmdLen);
	err = uncompress(cmdBuff, &originalCmdLen, tempBuff, imgHdr.cmdLen);
	if(err != Z_OK)
	{
		fprintf(stderr, "uncompress error on cmdBuff\n");
		fprintf(stderr, "%d %d %d %d\n", err, Z_BUF_ERROR, Z_MEM_ERROR, Z_DATA_ERROR);
		exit(1);
	}
	free(tempBuff); tempBuff = NULL;

	outBuff = unRLE(buff, originalLen, cmdBuff, originalCmdLen, &outLen);
	smalloc(RGBABuff, outLen);
	unDelta(outBuff, outLen, stdInfo.width, stdInfo.height, stdInfo.bpp / 8, RGBABuff);

    sprintf(outName, "%s%03lu.ppm", baseName, iteration);
    iteration++;
    saveImage(RGBABuff, stdInfo.width, stdInfo.height, outName);

	free(buff);
	free(cmdBuff);
	free(outBuff);
	free(RGBABuff);
}

uchar *unRLE(uchar *buff, ulong buffLen, uchar *cmdBuff, ulong cmdLen, ulong *outLen)
{
	bitBuffer *bitBuff = NULL;
	uchar copyFlag = 0;
	ulong n = 0, i = 0;
	uchar *outBuff = NULL;

	bitBuff  = bitBuffer_new(cmdBuff, cmdLen);
	copyFlag = getBit(bitBuff);

	*outLen = getEliasGamma(bitBuff);
	smalloc(outBuff, *outLen);
	for(i = 0; i < *outLen; i += n)
	{
		n = getEliasGamma(bitBuff);
		if(copyFlag)
		{
			memcpy(outBuff + i, buff, n);
			buff += n;
		} else
			memset(outBuff + i, 0, n);
		copyFlag = !copyFlag;
	}
    free(bitBuff);
	return outBuff;
}

inline uchar unpackValue(uchar c)
{
    uchar z = c & 1 ? 0xFF : 0x00;
    return (c >> 1) ^ z;
}

void unDelta(uchar *buff, ulong buffLen, ulong width, ulong height, ulong Bpp, uchar *RGBABuff)
{
#include "tables.h"
	ulong sectLen = buffLen / 4;
	ulong lineLen = width * Bpp;
	ulong x, y, val;
	uchar *sect1  = buff;
	uchar *sect2  = sect1 + sectLen;
	uchar *sect3  = sect2 + sectLen;
	uchar *sect4  = sect3 + sectLen;
	uchar *outPtr = RGBABuff;
	uchar *outEnd = RGBABuff + buffLen;
	uchar *line   = NULL;
	uchar *prev   = NULL;

	while(outPtr < outEnd)
	{
		val = table1[*sect1++] | table2[*sect2++] | table3[*sect3++] | table4[*sect4++];
		*outPtr++ = unpackValue((uchar)(val >>  0));
		*outPtr++ = unpackValue((uchar)(val >>  8));
		*outPtr++ = unpackValue((uchar)(val >> 16));
		*outPtr++ = unpackValue((uchar)(val >> 24));
	}
	for(x = Bpp; x < lineLen; x++)
		RGBABuff[x] += RGBABuff[x - Bpp];
	for(y = 1; y < height; y++)
	{
		line = RGBABuff + y * lineLen;
		prev = RGBABuff + (y - 1) * lineLen;
		for(x = 0; x < lineLen; x++)
			line[x] += prev[x];
	}
}

void saveImage(uchar *RGBABuffer, ulong width, ulong height, const char *filename)
{
    FILE *outFile;
    ulong i;
    uchar *buffer = NULL;
    outFile = fopen(filename, "wb+");
    fprintf(outFile, "P6\n%lu %lu\n255\n", width, height);
    smalloc(buffer, 3 * width * height);
    for(i = 0; i < width * height; i++)
    {
        memcpy(&buffer[3*i], &RGBABuffer[4 * ((height - 1 - i/width) * width + i % width)], 3);
        buffer[3*i + 2] += buffer[3*i];
        buffer[3*i] = buffer[3*i + 2] - buffer[3*i];
        buffer[3*i + 2] -= buffer[3*i];
    }
    fwrite(buffer, 3 * width * height, 1, outFile);
    free(buffer);
    fclose(outFile);
}

#undef smalloc
