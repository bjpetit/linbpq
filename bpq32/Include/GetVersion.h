
char VersionString[50]="";
char VersionStringWithBuild[50]="";
int Ver[4];

VOID GetVersionInfo(char * File)
{
	HRSRC RH;
  	struct tagVS_FIXEDFILEINFO * HG;
	char isDebug[40]="";
	HMODULE HM;

#ifdef SPECIALVERSION
	strcat(isDebug, SPECIALVERSION);
	strcat(isDebug, " ");
#endif
#ifdef _DEBUG 
	strcat(isDebug, "Debug Build");
#endif

	HM=GetModuleHandle(File);

	RH=FindResource(HM,MAKEINTRESOURCE(VS_VERSION_INFO),RT_VERSION);

	HG=LoadResource(HM,RH);

	(int)HG+=40;

	sprintf(VersionString,"%d.%d.%d.%d %s",
					HIWORD(HG->dwFileVersionMS),
					LOWORD(HG->dwFileVersionMS),
					HIWORD(HG->dwFileVersionLS),
					LOWORD(HG->dwFileVersionLS),
					isDebug);

	sprintf(VersionStringWithBuild,"%d.%d.%d Build %d %s",
					HIWORD(HG->dwFileVersionMS),
					LOWORD(HG->dwFileVersionMS),
					HIWORD(HG->dwFileVersionLS),
					LOWORD(HG->dwFileVersionLS),
					isDebug);

	Ver[0] = HIWORD(HG->dwFileVersionMS);
	Ver[1] = LOWORD(HG->dwFileVersionMS);
	Ver[2] = HIWORD(HG->dwFileVersionLS);
	Ver[3] = LOWORD(HG->dwFileVersionLS);

	return;

}
