#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "sioctl.h"



int main(int argc, char **argv)
{     

	HKEY hKey=0,hSubKey=0;
	int retCode,i,disp;
	char Dir[128];

	printf("SetRegistryPath Version 1.0\n\n");


	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
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
		
		i =  GetCurrentDirectory(128,Dir);

  		
		printf("Registry Key \"HKEY_LOCAL_MACHINE\\SOFTWARE\\G8BPQ\\BPQ32\" %s\n\n",
			
			(disp == 1) ? "Created" : "Opened");

		retCode = RegSetValueEx(hKey,"BPQ Directory",0,REG_SZ,Dir,i);
  
		
		if (retCode == ERROR_SUCCESS)
		{

			printf("Registry Value \"BPQ Directory\" set to %s\n",Dir);
		
			
			
		}
		else
			printf("Registry Value \"BPQ Directory\" create failed\n");


		printf("\nPress any key to Exit");
		i = _getch();

		return 0;

	}

	printf("Registry Key \"HKEY_LOCAL_MACHINE\\SOFTWARE\\G8BPQ\\BPQ32\" not found\n");
	
	printf("\nPress any key to Exit");
	i = _getch();

	return 0;

}