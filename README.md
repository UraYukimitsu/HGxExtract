# HGxExtract
Extract HGx files from the cs2 VN engine

I've had great trouble with the HGx file format, but I guess I've got it working perfectly now!

Here's what I've understood about the HGx (here I did HG3 but HG2 is almost the same):
(It may contain minor errors as I wrote this when I didn't fully understand how this worked)

	HG3 File specifications:
	Header: 20 Bytes
		-Magic "HG-3" or "HG-2" 	(4B)
		-Unknown values         	(12B)
		-Entry count            	(4B) //Not always relevant
		
	Tag   : 16 Bytes
		-Signature              	(8B)
		-OffsetNextTag          	(4B) //Not always relevant
		-Length                 	(4B)
		
	Tag signature:"stdinfo"
		StdInfo: 40 Bytes
			-Width                  (4B)
			-Height                 (4B)
			-bpp                    (4B)
			-OffsetX                (4B)
			-OffsetY                (4B)
			-TotalWidth             (4B)
			-TotalHeight            (4B)
			-Unknown values         (12B)

	Tag signature:"img0000"
		ImgHdr:  24 Bytes
			-Unknown value          (4B)
			-Height                 (4B)
			-Compressed length      (4B)
			-Original length        (4B)
			-Compressed CMD length  (4B)
			-Original CMD length    (4B)

			Read image:
				1. Read compressed buffer and uncompress it -> buff
				2. Read compressed CMD and uncompress it    -> cmdBuff
				3. Un-RLE (buff, cmdBuff) -> (outBuff, outLen)
				4. RGBABuff[outLen]
				5. Un-delta (outBuff, Width, Height, Bpp{=bpp/8}) -> RGBABuff
				
			Un-RLE:(buff, cmdBuff) -> (outBuff, outLen)
				1. cmdBuff is a bit buffer, 1st bit is copyFlag (boolean)
				2. cmdBuff EliasGamma is outLen;
				3. outBuff[outLen]
				4. n = 0; for(i = 0; i < outLen; i += n)
					a. n = cmdBuff EliasGamma
					b. if(copyFlag)
						Copy n bytes from buff to (outBuff + i)
						buff += n
					c. else
						Set n bytes of (outBuff to 0
					d. Invert copyFlag
				5. Return (outBuff, outLen)
				
			Un-delta:(buff, Width, Height, Bpp) -> (RGBABuff)
				1. table1[256] //Optimisation: table1 to table4 could be hard-coded
				2. table2[256] //That's what I did ^^
				3. table3[256]
				4. table4[256]
				5. for(i = 0; i < 256; i++)
					a. Value = (((((i & 0xC0) << 6 | (i & 0x30)) << 6) | (i & 0x0C)) << 6) | (i & 0x03) //Didn't find how to put this simply...
					b. table4[i] = val << 0
					c. table3[i] = val << 2
					d. table2[i] = val << 4
					e. table1[i] = val << 6
				06. sectLen = buffLen / 4
				07. sect1   = buff
				08. sect2   = sect1 + sectLen
				09. sect3   = sect2 + sectLen
				10. sect4   = sect3 + sectLen
				11. outPtr  = RGBABuff
				12. outEnd  = RGBABuff + buffLen
				13. while(outPtr < outEnd)
					a. Value = table1[*(sect1++)] | table2[*(sect2++)] | table3[*(sect3++)] | table4[*(sect4++)] //Same ^^
					b. *(outPtr++) = ((uchar)(val >>  0) >> 1) ^ ((uchar)(val >>  0) & 1 ? 0xFF : 0x00) //Same ^^
					c. *(outPtr++) = ((uchar)(val >>  8) >> 1) ^ ((uchar)(val >>  8) & 1 ? 0xFF : 0x00)
					d. *(outPtr++) = ((uchar)(val >> 16) >> 1) ^ ((uchar)(val >> 16) & 1 ? 0xFF : 0x00)
					e. *(outPtr++) = ((uchar)(val >> 32) >> 1) ^ ((uchar)(val >> 32) & 1 ? 0xFF : 0x00)
				14. lineLen = Width * Bpp
				15. for(x = Bpp; x < lineLen; x++)
					RGBABuff[x] += RGBABuff[x - Bpp]
				16. for(y = 1; y < height; y++)
					a. line = RGBABuff + y * lineLen
					b. prev = RGBABuff + (y - 1) * lineLen
					c. for(x = 0; x < lineLen; x++)
						line[x] += prev[x]
