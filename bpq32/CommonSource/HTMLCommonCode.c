
// General C Routines common to bpq32 and linbpq.mainly moved from BPQ32.c

#pragma data_seg("_BPQDATA")

#define _CRT_SECURE_NO_DEPRECATE
#define _USE_32BIT_TIME_T

#pragma data_seg("_BPQDATA")

#include "CHeaders.h"

char * GetTemplateFromFile(int Version, char * FN)
{
	int FileSize;
	char * MsgBytes;
	char MsgFile[265];
	FILE * hFile;
	int ReadLen;
	BOOL Special = FALSE;
	struct stat STAT;

	sprintf(MsgFile, "%s/HTML/%s", BPQDirectory, FN);

	if (stat(MsgFile, &STAT) == -1)
	{
		MsgBytes = _strdup("File is missing");
		return MsgBytes;
	}

	hFile = fopen(MsgFile, "rb");
	
	if (hFile == 0)
	{
		MsgBytes = _strdup("File is missing");
		return MsgBytes;
	}

	FileSize = STAT.st_size;
	MsgBytes = malloc(FileSize + 1);
	ReadLen = fread(MsgBytes, 1, FileSize, hFile); 
	MsgBytes[FileSize] = 0;
	fclose(hFile);

	// Check Version

	if (Version)
	{
		int PageVersion = 0;

		if (memcmp(MsgBytes, "<!-- Version", 12) == 0)
			PageVersion = atoi(&MsgBytes[13]);

		if (Version != PageVersion)
		{
			free(MsgBytes);
			MsgBytes = _strdup("Wrong Version of HTML Pages - please update");
		}
	}
	
	return MsgBytes;
}
