
char VersionString[50];
char VersionStringWithBuild[50];

VOID GetVersionInfo(char * File)
{
	HRSRC RH;
  	struct tagVS_FIXEDFILEINFO * HG;
#ifdef _DEBUG 
	char isDebug[]="Debug Build";
#else
	char isDebug[]="";
#endif
	HMODULE HM;

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


	return;

}
