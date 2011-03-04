#pragma once

#include "resource.h"

#define MMAPI_API __declspec( dllimport )

#include "..\MMAPI.h"

struct TARGETRECORD
{  
	UINT ID;
    char name[21];
    char Callsign[8];
    char Dest[21];
    UINT IMO;
    double ROT;
    int LengthA;
    int LengthB;
    int WidthC;
    int WidthD;
    int Draft;
    double lat;
    double Long;
    double course;
    double speed;
    double Heading;
    int NavStatus;
    double LatIncr;
    double LongIncr;
    double LastCourse;
    double LastSpeed;
    double Distance;
    double Bearing;
    UINT Colour;
    double Closest;
    UINT TCA;
    UINT BCA;		//                           ' Bearing at closest
    UINT TimeAdded;
    UINT TimeLastUpdated;
    BOOL HiSpeedAlerted;
    HANDLE MMHandle;
    HANDLE TrackHandle;
	int DisplayLine;
     
} TargetRecord;

struct NAVAIDRECORD
{
	UINT ID;
    char name[21];
	int NavAidType;
	double lat;
    double Long;
	int LengthA;
    int LengthB;
    int WidthC;
    int WidthD;
    int FixType;
    UINT TimeAdded;
    UINT TimeLastUpdated;
    HANDLE MMHandle;

} NavaidRecord;

struct  BASESTATIONRECORD
{
	UINT ID;
    int FixType;
	double lat;
    double Long;
   int NumberofStations;
    UINT TimeAdded;
    UINT TimeLastUpdated;
    HANDLE MMHandle;

} BaseStationRecord;

