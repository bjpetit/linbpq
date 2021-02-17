// AIS_MM_Iface.c

//	Calls routines in MMAPI.dll if you don't have an import library

//

#include "stdafx.h"

#define DllImport	__declspec( dllimport )
#define DllExport	__declspec( dllexport )

HINSTANCE ExtDriver=0;

typedef int (FAR *FARPROCX)();

FARPROCX MM_StartPtr;
FARPROCX MM_GetVersionPtr;
FARPROCX MM_MarkCreatePtr;
FARPROCX MM_GetWindowPtr;
FARPROCX MM_EndPtr;
FARPROCX MM_ShowWindowPtr;
FARPROCX MM_AddIconPtr;
FARPROCX MM_MarkCreatePtr;
FARPROCX MM_ObjFindPtr;
FARPROCX MM_ObjGetTypePtr;
FARPROCX MM_ObjGetNamePtr;
FARPROCX MM_ObjGetCategoryPtr;
FARPROCX MM_ObjSetCategoryPtr;
FARPROCX MM_MarkGetPositionPtr;
FARPROCX MM_PosnDetailsPtr;
FARPROCX MM_PosnUpdatePtr;
FARPROCX MM_PosnCreatePtr;
FARPROCX MM_GetCurrentPositionStructPtr;
FARPROCX MM_GetCurrentPositionPtr;
FARPROCX MM_GetCurrentPositionObjPtr;
FARPROCX MM_SetCurrentPositionPtr;
FARPROCX MM_MarkSetCommentPtr;
FARPROCX MM_ObjDeletePtr;
FARPROCX MM_ObjLockPtr;
FARPROCX MM_SetWindowPtr;
FARPROCX MM_SetPosnPolygonPtr;
FARPROCX MM_MarkMovePtr;
FARPROCX MM_TrackAddPtr;
FARPROCX MM_TrackCreatePtr;
FARPROCX MM_TrackGetNumPointsPtr;
FARPROCX MM_TrackGetPointsPtr;


char Dllname[6]="mmapi";


LRESULT CALLBACK WndProc(HWND  hwnd, UINT  uMsg, WPARAM  wParam,LPARAM  lParam);
int InitialiseMM();


BOOL APIENTRY DllMain(HANDLE hInst, DWORD ul_reason_being_called, LPVOID lpReserved)
{
	
	switch( ul_reason_being_called )
	{

	case DLL_PROCESS_ATTACH:

		return InitialiseMM();

	case DLL_THREAD_ATTACH:
		
		return 1;
    
	case DLL_THREAD_DETACH:
	
		return 1;
    
	case DLL_PROCESS_DETACH:
	
		return 1;
    
	}
 
	return 1;
	
}

FARPROCX GetAddress(char * Proc)
{
	FARPROCX ProcAddr;
	char msg[128];
	int err=0;

	ProcAddr=(FARPROCX) GetProcAddress(ExtDriver,Proc);

	if (ProcAddr == 0)
	{
		err=GetLastError();

		wsprintf(msg,"Error Getting address of %s - Error code %d",
				Proc,err);
		
	
		MessageBox(NULL,msg,"AVI_MM",MB_ICONSTOP);

	
		return(0);
	}

	return ProcAddr;
}

int InitialiseMM()
{
	// Load the Memory Map Interface DLL and get the entry points we need

	char msg[128];
	int err=0;

	ExtDriver=LoadLibrary(Dllname);

	if (ExtDriver == NULL)
	{
		err=GetLastError();
		wsprintf(msg,"Error loading Driver %s - Error code %d",
				Dllname,err);
		
		MessageBox(NULL,msg,"AIS_MM",MB_ICONSTOP);

		return(FALSE);
	}

	MM_StartPtr=GetAddress("?MM_Start@@YAH_N@Z");
	MM_GetVersionPtr=GetAddress("?MM_GetVersion@@YAHQAH@Z");
	MM_MarkCreatePtr=GetAddress("?MM_MarkCreate@@YAHPBDNNHPAPAX@Z");
	MM_GetWindowPtr=GetAddress("?MM_GetWindow@@YAHPAN000@Z");
	MM_EndPtr=GetAddress("?MM_End@@YAH_N@Z");
	MM_ShowWindowPtr=GetAddress("?MM_ShowWindow@@YAHH@Z");
	MM_AddIconPtr=GetAddress("?MM_AddIcon@@YAHPBDH@Z");
	MM_MarkCreatePtr=GetAddress("?MM_MarkCreate@@YAHPBDNNHPAPAX@Z");
	MM_ObjFindPtr=GetAddress("?MM_ObjFind@@YAHPBDHPAPAX@Z");
	MM_ObjLockPtr=GetAddress("?MM_ObjLock@@YAHPAX_N@Z");
	MM_ObjGetTypePtr=GetAddress("?MM_ObjGetType@@YAHPAXPAH@Z");
	MM_ObjGetNamePtr=GetAddress("?MM_ObjGetName@@YAHPAXPADH@Z");
	MM_ObjGetCategoryPtr=GetAddress("?MM_ObjGetCategory@@YAHPAXPADH@Z");
	MM_ObjSetCategoryPtr=GetAddress("?MM_ObjSetCategory@@YAHPAXPBD@Z");
	MM_MarkGetPositionPtr=GetAddress("?MM_MarkGetPosition@@YAHPAXPAN1@Z");
	MM_PosnDetailsPtr=GetAddress("?MM_PosnDetails@@YAHPAXPBD11@Z");
	MM_PosnUpdatePtr=GetAddress("?MM_PosnUpdate@@YAHPAXINNNNNNNKPBD@Z");
	MM_PosnCreatePtr=GetAddress("?MM_PosnCreate@@YAHPBDPAPAX@Z");
    MM_GetCurrentPositionPtr=GetAddress("?MM_GetCurrentPosition@@YAHPAN0000@Z");
    MM_SetCurrentPositionPtr=GetAddress("?MM_SetCurrentPosition@@YAHNNNNNH@Z");
    MM_GetCurrentPositionObjPtr=GetAddress("?MM_GetCurrentPositionObj@@YAHPAPAX@Z");
	MM_GetCurrentPositionStructPtr=GetAddress("?MM_GetCurrentPositionStruct@@YAHPAUMM_position_data2@@@Z");
	MM_MarkSetCommentPtr=GetAddress("?MM_MarkSetComment@@YAHPAXPBD@Z");
	MM_ObjDeletePtr=GetAddress("?MM_ObjDelete@@YAHPAX@Z");
//	MM_SetWindowPtr=GetAddress("?MM_SetWindow@@YAHHHHH@Z"); // int params??
	MM_SetWindowPtr=GetAddress("?MM_SetWindow@@YAHNNNN@Z");
	MM_SetPosnPolygonPtr=GetAddress("?MM_SetPosnPolygon@@YAHPAXHPAH@Z");
	MM_MarkMovePtr=GetAddress("?MM_MarkMove@@YAHPAXNN@Z");

	MM_TrackAddPtr=GetAddress("?MM_TrackAdd@@YAHPAXHPAN1PAIPAM@Z");
	MM_TrackCreatePtr=GetAddress("?MM_TrackCreate@@YAHPADK_NPAPAX@Z");
	MM_TrackGetNumPointsPtr=GetAddress("?MM_TrackGetNumPoints@@YAHPAXPAH@Z");
	MM_TrackGetPointsPtr=GetAddress("?MM_TrackGetPoints@@YAHPAXHHPAN1PAIPAM@Z");



	return TRUE;

}



#define MMAPI_API __declspec( dllexport )

MMAPI_API int APIENTRY MM_Start(BOOL run_mmnav)
{
	return MM_StartPtr(run_mmnav);
}

// Initialize the library. If run_mmnav is true, launch MMNav if it is not already running.
//  NB: run_mmnav is currently unsupported on WinCE

MMAPI_API int APIENTRY MM_GetVersion(int * ver)
{
	return MM_GetVersionPtr(ver);
}

MMAPI_API int APIENTRY MM_End(BOOL close_mmnav)    

// Closes the library. If close_mmnav is true, MMNav is closed.
{
	return MM_EndPtr(close_mmnav);
}

MMAPI_API int APIENTRY MM_ShowWindow(int cmd)    

// Control the MMNav main window. cmd is SW_SHOW, SW_MINIMIZE, etc.
{
	return MM_ShowWindowPtr(cmd);
}

MMAPI_API int APIENTRY MM_SetWindow(double max_lat, double min_lat, double max_lon, double min_lon)

// MMNav zooms to the approximate area requested, loading an 
// appropriate map, if needed. If max_lat==min_lat and 
// max_lon==min_lon, then this point is placed at the 
// center of the window in the largest available scale 
// map. Note: The zoom level is limited to factors of two, and
// the size and shape of the window are not changed by this function.
{
	return MM_SetWindowPtr(max_lat, min_lat, max_lon, min_lon);
}
MMAPI_API int APIENTRY MM_GetWindow(double *,double *,double *,double *);

MMAPI_API int APIENTRY MM_PosnCreate(char const * name,HANDLE *h)
{
	return MM_PosnCreatePtr(name,h);
}



MMAPI_API int APIENTRY MM_PosnUpdate(void * h,unsigned int i, double d1, double d2, double d3,double d4,double d5,double d6,double d7,unsigned long j,char const * a)
{
	return MM_PosnUpdatePtr(h,i,d1,d2,d3,d4,d5,d6,d7,j,a);
}

MMAPI_API int APIENTRY MM_PosnDetails(void * h,char const * a,char const * b ,char const * c)
{
	return MM_PosnDetailsPtr(h, a, b, c);
}

MMAPI_API int APIENTRY MM_GetCurrentPositionObj(HANDLE *h)
{
	return MM_GetCurrentPositionObjPtr(h);
}

MMAPI_API int APIENTRY MM_SetPosnPolygon(void * h ,int x,int * y)
{
	return MM_SetPosnPolygonPtr(h, x, y);
}

MMAPI_API int APIENTRY MM_GetCurrentPosition(double *lat, double *lon, double *course, double *speed, double *altitude)

// Returns the most recent position received from GPS. Pass NULL if you are not interested
//  in all returned values
{
	return MM_GetCurrentPositionPtr(lat, lon, course, speed, altitude);
}


MMAPI_API int APIENTRY MM_GetCurrentPositionStruct(struct MM_position_data2 * s)
{
	return MM_GetCurrentPositionStructPtr(s);
}


MMAPI_API int APIENTRY MM_SetCurrentPosition(double lat, double lon, 

double course, double speed, double altitude, int valid)

// Sets the current position object.
//  NB. The GPS in MMnav should be set to "None" when using this function.
//  The if valid is non-zero, the icon is shown in red, and the central dot
//  alternates on and off with each call. If valid is zero, the icon is shown
//  in grey. The API only supports a single Position object.
{
	return MM_SetCurrentPositionPtr(lat, lon, course, speed, altitude, valid);
}
 

MMAPI_API int APIENTRY MM_ObjFind(const TCHAR *name, int type, HANDLE *h)

// Returns a handle of an object with matching name and type 
//  Wildcards (* and ?) are allowed in the name. h is an in/out
//  parameter: Call with h==NULL first time, then with the returned 
//  handle, and so on until the return value is NULL. The 
//  objects are returned in the reverse order of creation, 
//  i.e., the most recently created object is returned 
//  first. Type is MM_OT... values below.

{
	return MM_ObjFindPtr(name, type, h);
}
 

MMAPI_API int APIENTRY MM_ObjGetType(HANDLE h, int *type)

// returns one of the following in *type.

#define MM_OT_UNKNOWN -1
#define MM_OT_ANY -1
#define MM_OT_MARKWP 1
#define MM_OT_ROUTE 2
#define MM_OT_TRACK 4 
#define MM_OT_POSITION 8
#define MM_OT_CHART 16
#define MM_OT_TEXT 32
{
	return MM_ObjGetTypePtr(h, type);
}


MMAPI_API int APIENTRY MM_ObjGetName(HANDLE h, TCHAR *name, int max_chars)

// Returns the name of the object;
{
	return MM_ObjGetNamePtr(h, name, max_chars);
}


MMAPI_API int APIENTRY MM_ObjDelete(HANDLE h)

// Object is permanently deleted from mmnav, even if it is locked
//  NB. When you delete a Route object, all the waypoints that
//  are not locked are are not part of any other routes are 
//  also deleted.
{
	return MM_ObjDeletePtr(h);
}
MMAPI_API int APIENTRY MM_ObjRename(HANDLE h, const TCHAR *name); 

// Rename object.

MMAPI_API int APIENTRY MM_ObjLock(HANDLE h, BOOL locked)
{
// Control whether user can change/delete the object.

	return MM_ObjLockPtr(h, locked);
}

// Control whether user can change/delete the object.

MMAPI_API int APIENTRY MM_ObjVisible(HANDLE h, BOOL visible); 

// Control whether object is visible or hidden.

MMAPI_API int APIENTRY MM_ObjGetCategory(HANDLE h, TCHAR *category, int max_chars)

// Returns the category of the object;

{
	return MM_ObjGetCategoryPtr(h, category, max_chars);
}


MMAPI_API int APIENTRY MM_ObjSetCategory(HANDLE h, const TCHAR *category) 

// Change the object's category
{
	return MM_ObjSetCategoryPtr(h, category);
}

MMAPI_API int APIENTRY MM_AddIcon(const TCHAR *bmp_filename, int id)

// Adds the mark/WP icon with the specified ID.
// This just registers the icon image. Use MM_MarkCreate to create a mark that uses the icon.
// The file must be a BMP 32x32 pixels, 16-color.
// Use a full pathname, not a relative path.
// The ID must be > 100.
{
	return MM_AddIconPtr(bmp_filename, id);
}

 
MMAPI_API int APIENTRY MM_RemoveIcon(int id);

// Remove the icon with that id. Any marks that use this icon default
// to a plain dot.

MMAPI_API int APIENTRY MM_MarkCreate(const TCHAR *name, double lat, double lon, int icon_id, HANDLE *h)
{
	return MM_MarkCreatePtr(name,  lat, lon, icon_id, h);
}

// Create a new mark / waypoint. The icon_id is one of the mm_smbl values below, or an icon id
//  defined with MM_AddIcon()

 

MMAPI_API int APIENTRY MM_MarkMove(HANDLE h, double lat, double lon)
{
	return MM_MarkMovePtr(h, lat, lon);
}

// Change the position of a mark / waypoint

MMAPI_API int APIENTRY MM_MarkGetPosition(HANDLE h, double *lat, double *lon)

// Return the lat/lon of the mark / waypoint
{
	return MM_MarkGetPositionPtr(h, lat, lon);
}


MMAPI_API int APIENTRY MM_MarkLinkFile(HANDLE h, const TCHAR *filename);

// Set the mark's HotSpot link to the given filename.

MMAPI_API int APIENTRY MM_MarkGetLinkFile(HANDLE h, TCHAR *filename, int max_chars);

// Returns the full path of the linked filename

MMAPI_API int APIENTRY MM_MarkSetIcon(HANDLE h, int id);

// Change the mark's symbol icon.

MMAPI_API int APIENTRY MM_MarkSetRadius(HANDLE h, float radius, int alarm_type);

// Radius in meters, alarm_type is one of:
//  0=>no circle shown 1=>show circle with no alarm (grey), 
//  2=>proximity alarm (red), 3=>anchor alarm (green)

MMAPI_API int APIENTRY MM_MarkSetComment(HANDLE h,char const * Comment)
{
	return MM_MarkSetCommentPtr(h, Comment);
}

 
MMAPI_API int APIENTRY MM_RouteCreate(COLORREF color, HANDLE *h); 

// Creates an empty route. Color is ignored in the current version.
// the handle is returned in h

MMAPI_API int APIENTRY MM_RouteAdd(HANDLE hRoute, HANDLE hMark);

// Adds the WP (created with MM_MarkCreate) to the route.

MMAPI_API int APIENTRY MM_RouteGetWP(HANDLE hRoute, int i, HANDLE *h);

// Returns waypoint handle at position i in the route
//  i==0 is the first WP. h is returned NULL if i is >=
// the number of WP.

MMAPI_API int APIENTRY MM_TrackCreate(TCHAR *name, COLORREF color, BOOL closed_polygon, HANDLE *h)
{
	return MM_TrackCreatePtr(name, color, closed_polygon, h);
}
// Creates an empty track

MMAPI_API int APIENTRY MM_TrackAdd(HANDLE hTrack, int n, double *lat, double *lon, unsigned int *times, float *altitude)
{
	return MM_TrackAddPtr(hTrack, n, lat, lon, times, altitude);
}

// Adds n points to the end of the track. 
//  time is in seconds since Jan 1, 1970.
//  time and/or altitude may be null.

MMAPI_API int APIENTRY MM_TrackGetNumPoints(HANDLE hTrack, int *n)
{
	return MM_TrackGetNumPointsPtr(hTrack, n);
}

// returns via n the number of points in the track

MMAPI_API int APIENTRY MM_TrackGetPoints(HANDLE hTrack, int start, int n,
double *lat, double *lon, unsigned int *times, float *altitude)
{
	return MM_TrackGetPointsPtr(hTrack, start, n, lat, lon, times, altitude);
}
// returns n track data points, starting from the zero-based index 'start'
// times and/or altitude pointers may be NULL 

// All functions return one of the following Error codes

#define MMERR_OK 0
#define MMERR_NOT_RUNNING 1
#define MMERR_API_INUSE 2
#define MMERR_LAUNCH_FAILED 3
#define MMERR_BAD_HWND 4
#define MMERR_NO_MAP 5
#define MMERR_HANDSHAKE_ERR 6
#define MMERR_TIMEOUT 7
#define MMERR_FILE_ERR 8
#define MMERR_STRING_TRUNC  9
#define MMERR_LICENSE 10
#define MMERR_NOT_IMPLEMENTED 11
#define MMERR_NOT_AVAILABLE 12
#define MMERR_BAD_HANDLE    13
#define MMERR_FAILED        14
#define MMERR_BAD_INPUT     15

