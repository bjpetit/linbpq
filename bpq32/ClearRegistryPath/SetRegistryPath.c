#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{     
	HKEY hKey=0, hSubKey = 0;
	int retCode, i, disp, p;
	char Dir[MAX_PATH];
	char ProgDir[MAX_PATH];
	HKEY REGTREE = HKEY_CURRENT_USER;
	char * REGTREETEXT = "HKEY_CURRENT_USER";
	BOOL Quiet = FALSE;

	printf("SetRegistryPath Version 2.0.0\n\n");

	if (argc >1)
	{
		if (_stricmp(argv[1], "/quiet") == 0)
		{
			Quiet = TRUE;
			argv[1] = argv[2];
			argc--;
		}
	}
	if (_winver < 0x600)
	{
		REGTREE = HKEY_LOCAL_MACHINE;
		REGTREETEXT = "HKEY_LOCAL_MACHINE";
	}

	retCode = RegCreateKeyEx(REGTREE,
                              "SOFTWARE\\G8BPQ\\BPQ32",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);


    if (retCode == ERROR_SUCCESS)
	{
		if (argc > 1)
		{
			strcpy(Dir, argv[1]);
			i = strlen(Dir);
		}
		else
			i = GetCurrentDirectory(MAX_PATH, Dir);

		GetModuleFileName(NULL, ProgDir, MAX_PATH);

		// Remove program name

		for (p = strlen(ProgDir); p >= 0; p--)
		{
			if (ProgDir[p]=='\\') 
			{
				break;
			}

		}

		ProgDir[p] = 0;		

		printf("Registry Key \"%s\\SOFTWARE\\G8BPQ\\BPQ32\" %s\n\n",
			
			REGTREETEXT, (disp == 1) ? "Created" : "Opened");

		retCode = RegSetValueEx(hKey,"BPQ Directory",0,REG_SZ ,Dir, i + 1);
  
		if (retCode == ERROR_SUCCESS)
			printf("Registry Value \"BPQ Directory\" set to %s\n", Dir);	
		else
			printf("Registry Value \"BPQ Directory\" create failed\n");

		retCode = RegSetValueEx(hKey,"BPQ Program Directory",0,REG_SZ, ProgDir, strlen(ProgDir) + 1);
  
		if (retCode == ERROR_SUCCESS)
			printf("Registry Value \"BPQ Program Directory\" set to %s\n", ProgDir);	
		else
			printf("Registry Value \"BPQ Program Directory\" create failed\n");

		if (!Quiet)
		{
			printf("\nPress any key to Exit");
			i = _getch();
		}
		return 0;

	}

	printf("Registry Key \"%s\\SOFTWARE\\G8BPQ\\BPQ32\" not found\n",REGTREETEXT);
	
	printf("\nPress any key to Exit");
	i = _getch();

	return 0;

}