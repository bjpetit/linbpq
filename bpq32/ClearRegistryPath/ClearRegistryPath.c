#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Use LOCAL_MACHINE on Vista or above
// Add /quiet option

int main(int argc, char **argv)
{     
	HKEY hKey=0;
	int retCode,i;
	HKEY REGTREE = HKEY_CURRENT_USER;
	char * REGTREETEXT = "HKEY_CURRENT_USER";
	BOOL Quiet = FALSE;

	printf("ClearRegistryPath Version 2.0.1\n\n");

	if (argc > 1)
		if (_stricmp(argv[1], "/quiet") == 0)
			Quiet = TRUE;

	if (_winver < 0x600)
	{
		REGTREE = HKEY_LOCAL_MACHINE;
		REGTREETEXT = "HKEY_LOCAL_MACHINE";
	}

	retCode = RegOpenKeyEx (REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,
                              KEY_ALL_ACCESS,
                              &hKey);

    if (retCode == ERROR_SUCCESS)
	{
		// Try "BPQ Directory"

		retCode = RegDeleteValue(hKey,"BPQ Directory");

		printf("Registry Key \"%s\\SOFTWARE\\G8BPQ\\BPQ32\" opened\n\n", REGTREETEXT);

		if (retCode == ERROR_SUCCESS)
	
			printf("Registry Value \"BPQ Directory\" deleted\n");
		else
			printf("Registry Value \"BPQ Directory\" not found\n");

		retCode = RegDeleteValue(hKey,"BPQ Program Directory");

		if (retCode == ERROR_SUCCESS)
	
			printf("Registry Value \"BPQ Program Directory\" deleted\n");	
		else
			printf("Registry Value \"BPQ Program Directory\" not found\n");
			
		RegCloseKey(hKey);	
	
		if (!Quiet)
		{
			printf("\nPress any key to Exit");
			i = _getch();
		}

		return 0;

	}

	printf("Registry Key \"HKEY_LOCAL_MACHINE\\SOFTWARE\\G8BPQ\\BPQ32\" not found\n");
	
	printf("\nPress any key to Exit");
	i = _getch();

	return 0;

}