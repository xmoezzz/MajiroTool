#include <cstdio> 
#include <windows.h>
#include <string>

using namespace std;

typedef char s8;
typedef DWORD u32;


typedef struct
{
	s8 magic[8];		/* "????TC00" and "????TC01" and "????TS00" and "????TS01" */
	u32 width;
	u32 height;
	u32 data_length;
} rct_header_t;

string filename;


static DWORD rct_decompress(BYTE *uncompr, DWORD uncomprLen,
	BYTE *compr, DWORD comprLen, DWORD width)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	DWORD pos[32];

	pos[0] = -3;
	pos[1] = -6;
	pos[2] = -9;
	pos[3] = -12;
	pos[4] = -15;
	pos[5] = -18;
	pos[6] = (3 - width) * 3;
	pos[7] = (2 - width) * 3;
	pos[8] = (1 - width) * 3;
	pos[9] = (0 - width) * 3;
	pos[10] = (-1 - width) * 3;
	pos[11] = (-2 - width) * 3;
	pos[12] = (-3 - width) * 3;
	pos[13] = 9 - ((width * 3) << 1);
	pos[14] = 6 - ((width * 3) << 1);
	pos[15] = 3 - ((width * 3) << 1);
	pos[16] = 0 - ((width * 3) << 1);
	pos[17] = -3 - ((width * 3) << 1);
	pos[18] = -6 - ((width * 3) << 1);
	pos[19] = -9 - ((width * 3) << 1);
	pos[20] = 9 - width * 9;
	pos[21] = 6 - width * 9;
	pos[22] = 3 - width * 9;
	pos[23] = 0 - width * 9;
	pos[24] = -3 - width * 9;
	pos[25] = -6 - width * 9;
	pos[26] = -9 - width * 9;
	pos[27] = 6 - ((width * 3) << 2);
	pos[28] = 3 - ((width * 3) << 2);
	pos[29] = 0 - ((width * 3) << 2);
	pos[30] = -3 - ((width * 3) << 2);
	pos[31] = -6 - ((width * 3) << 2);

	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];

	while (1) {
		BYTE flag;
		DWORD copy_bytes, copy_pos;

		if (curByte >= comprLen)
			break;

		flag = compr[curByte++];

		if (!(flag & 0x80)) {
			if (flag != 0x7f)
				copy_bytes = flag * 3 + 3;
			else {
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
				copy_bytes *= 3;
			}

			if (curByte + copy_bytes - 1 >= comprLen)
				break;
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;

			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;
		}
		else {
			copy_bytes = flag & 3;
			copy_pos = (flag >> 2) & 0x1f;

			if (copy_bytes != 3) {
				copy_bytes = copy_bytes * 3 + 3;
			}
			else {
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 4;
				copy_bytes *= 3;
			}

			for (unsigned int i = 0; i < copy_bytes; i++) {
				if (act_uncomprLen >= uncomprLen)
					goto out;
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];
				act_uncomprLen++;
			}
		}
	}
out:
	//	if (curByte != comprLen)
	//		fprintf(stderr, "compr miss-match %d VS %d\n", curByte, comprLen);

	return act_uncomprLen;
}


int CALLBACK WriteBMP32A(const string&    filename, unsigned char* buff, unsigned long len,
	unsigned long    width,
	unsigned long    height,
	unsigned short   depth)
{
	BITMAPFILEHEADER bmf;
	BITMAPINFOHEADER bmi;

	memset(&bmf, 0, sizeof(bmf));
	memset(&bmi, 0, sizeof(bmi));

	bmf.bfType = 0x4D42;
	bmf.bfSize = sizeof(bmf)+sizeof(bmi)+len;
	bmf.bfOffBits = sizeof(bmf)+sizeof(bmi);

	bmi.biSize = sizeof(bmi);
	bmi.biWidth = width;
	bmi.biHeight = height;
	bmi.biPlanes = 1;
	bmi.biBitCount = depth * 8;

	/*
	char pDic[260];
	memset(pDic,0,sizeof(pDic));
	GetCurrentDirectoryA(260, pDic);
	*/

	FILE* fp = fopen(filename.c_str(), "wb");
	fwrite(&bmf, sizeof(bmf), 1, fp);
	fwrite(&bmi, sizeof(bmi), 1, fp);
	fwrite(buff, len, 1, fp);
	fclose(fp);
	return 0;
}


static int dump_rct1(rct_header_t *rct)
{
	BYTE *compr, *uncompr;
	DWORD uncomprLen, comprLen, actLen;
	WORD fn_len;

	uncomprLen = rct->width * rct->height * 3;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -1;

	compr = (BYTE *)(rct + 1);
	fn_len = *(WORD *)compr;
	compr += 2;
	compr += fn_len;
	comprLen = rct->data_length;

	actLen = rct_decompress(uncompr, uncomprLen, compr, comprLen, rct->width);
	if (actLen != uncomprLen) {
		//		fprintf(stderr, "rct decompress error occured (%d VS %d)\n",
		//			actLen, uncomprLen);
		free(uncompr);
		return -1;
	}

	WriteBMP32A(filename, uncompr, uncomprLen, rct->width, 0 - rct->height, 24 / 8);
	free(uncompr);

	return 0;
}

static int dump_rct(rct_header_t *rct)
{
	BYTE *compr, *uncompr;
	DWORD uncomprLen, comprLen, actLen;

	uncomprLen = rct->width * rct->height * 3;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -1;

	compr = (BYTE *)(rct + 1);
	comprLen = rct->data_length;
	actLen = rct_decompress(uncompr, uncomprLen, compr, comprLen, rct->width);
	if (actLen != uncomprLen) {
		//		fprintf(stderr, "rct decompress error occured (%d VS %d)\n",
		//			actLen, uncomprLen);
		free(uncompr);
		return -1;
	}

	WriteBMP32A(filename, uncompr, uncomprLen, rct->width, 0 - rct->height, 24 / 8);

	free(uncompr);

	return 0;
}



int main(int argc, char **argv)
{
	if (argc != 2)
	{
		MessageBoxA(NULL, "Usage: MajiroDecoder <infile>\n", "Majiro Tool", MB_OK);
		return -2;
	}
	FILE *fin = fopen(argv[1], "rb");
	fseek(fin, 0, SEEK_END);
	DWORD FileSize = ftell(fin);
	rewind(fin);
	char *pFile = new char[FileSize];
	fread(pFile, FileSize, 1, fin);
	fclose(fin);

	filename = argv[1];
	filename += ".bmp";

	if (!memcmp(pFile, "\x98\x5a\x92\x9aTC00", 8))
	{
		if (dump_rct((rct_header_t *)pFile))
		{
			delete[] pFile;
			return -1;
		}
	}
	else if (!memcmp(pFile, "\x98\x5a\x92\x9aTC01", 8))
	{
		if (dump_rct1((rct_header_t *)pFile))
		{
			delete[] pFile;
			return -1;
		}
	}

	delete[] pFile;
	return 0;
}
