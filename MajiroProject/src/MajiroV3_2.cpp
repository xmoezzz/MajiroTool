#include <cstdio>
#include <windows.h>
#include <cstring>
#include <wchar.h>

//Coded by X'moe Project

using namespace std;

typedef unsigned char u8;

typedef struct MJAHeader
{
	u8 Signature[16];
	DWORD FileCount;
	DWORD FilenameTableOffset;
	DWORD DataBlockOffset;
};

/*
typedef struct MJAEntryPoint
{
	DWORD HashName;
	DWORD Offset;
	DWORD Length;  
};
*/

typedef struct MJAEntryPoint {
  unsigned long unknown1;
  unsigned long unknown2;
  unsigned long Offset;
  unsigned long Length;
};


int main(int argc, char **argv)
{
	printf("MJA_Unpacker,coded by X'moe\n");
	if(argv[1] == NULL)
	{
		//printf("[Usage] %s <infile>\n",argv[0]);
		MessageBoxA(NULL, "[Usage] MajiroV2.exe <infile>", "MajiroV2.00 X'moe", MB_OK | MB_ICONHAND);
		return 0;
	}
	FILE *fp = fopen(argv[1],"rb");
	if(fp == NULL)
	{
		printf("Cannot open [%s]\n",argv[1]);
		return 0;
	}
	
	//fseek(fp,0,SEEK_END);
	//DWORD FileSize=ftell(fp);
	//rewind(fp);
	//char *pFile=new char [FileSize];
	//fread(pFile,FileSize,1,fp);
	//fclose(fp);
	
	MJAHeader pFileHeader;
	//memcpy(&pFileHeader,pFile,sizeof(MJAHeader));
	fread(&pFileHeader, 1 ,sizeof(MJAHeader), fp);
	
	bool proc_flag = false;
	if(!memcmp(pFileHeader.Signature, "MajiroArcV3.000\0",16))
	{
		MessageBoxA(NULL, "Error : \n"
		                  "The Package's Version is Ver3.000\n"
						  "Please use MajiroV3.exe firstly\n"
						  "PS : Make sure you've already backuped it.\n",
		                  "MajiroV2.00 X'moe", MB_OK | MB_ICONHAND);
        
	}
	else if(!memcmp(pFileHeader.Signature, "MajiroArcV2.000\0",16))
	{
		proc_flag = true;
	}
	
	if(!proc_flag)
	{
		MessageBoxA(NULL, "Error : \n"
						  "Invaild package or unsupported version.\n",
		                  "MajiroV2.00 X'moe", MB_OK | MB_ICONHAND);
	}
	DWORD EntryPointLen            = sizeof(MJAEntryPoint)*pFileHeader.FileCount;
	MJAEntryPoint *EntryPointCount = new MJAEntryPoint[pFileHeader.FileCount];
	//memcpy(EntryPointCount, (pFile+sizeof(MJAHeader)), EntryPointLen);
	fseek(fp, sizeof(MJAHeader), SEEK_SET);
	fread(EntryPointCount, 1, EntryPointLen, fp);

	//NametableBlock
    DWORD  NameTableLen  = pFileHeader.DataBlockOffset - pFileHeader.FilenameTableOffset;
    char*  NameTablebuf  = new char[NameTableLen];
    //memcpy(NameTablebuf,(pFile+pFileHeader.FilenameTableOffset), NameTableLen);   
    fseek(fp, pFileHeader.FilenameTableOffset, SEEK_SET);
    fread(NameTablebuf ,1 ,NameTableLen, fp);  

	//FileNameTable
    char* name = NameTablebuf;
    
    for(DWORD i = 0;i<pFileHeader.FileCount;i++)
    {
    	DWORD Pos = EntryPointCount[i].Offset;
    	DWORD Len = EntryPointCount[i].Length;
    	wchar_t *UniName = new wchar_t[MAX_PATH];
    	wmemset(UniName, 0, sizeof(UniName)/sizeof(wchar_t));
    	char *FileBuf = new char[Len];
    	fseek(fp, Pos, SEEK_SET);
    	//memcpy(FileBuf,(pFile+Pos),Len);
    	fread(FileBuf, 1, Len, fp);
    	MultiByteToWideChar(932, 0, name, -1, UniName, strlen(name)*2);
    	wprintf(L"%s\n",UniName);
    	FILE *fout = _wfopen(UniName,L"wb");
    	fwrite(FileBuf,Len,1,fout);
    	fclose(fout);
    	name+=strlen(name)+1;
    	delete[] UniName;
    	delete[] FileBuf;
    }
    
    fclose(fp);
	return 0;
}
