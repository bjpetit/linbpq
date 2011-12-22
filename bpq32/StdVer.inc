
1 VERSIONINFO
 FILEVERSION Vers
 PRODUCTVERSION Vers
 FILEFLAGSMASK 0x3fL
#ifdef _DEBUG
 FILEFLAGS 0x1L
#else
 FILEFLAGS 0x0L
#endif
 FILEOS 0x40004L
 FILETYPE 0x2L
 FILESUBTYPE 0x0L
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "080904b0"
        BEGIN
            VALUE "Comments", VerComments
            VALUE "CompanyName", "G8BPQ\0"
            VALUE "FileDescription", VerDesc
            VALUE "FileVersion", Verstring
 //           VALUE "InternalName", "bpq32\0"
            VALUE "LegalCopyright", VerCopyright
//            VALUE "OriginalFilename", "bpq32.dll\0"
            VALUE "ProductName", "BPQ32\0"
            VALUE "ProductVersion", Verstring
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x809, 1200
    END
END

#define BPQICON 2

BPQICON             ICON    DISCARDABLE     "..\\bpqicon.ico"

#ifdef MAILCHAT

IDD_ABOUTBOX DIALOG DISCARDABLE  22, 17, 202, 69
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "About"
FONT 8, "System"
BEGIN
    ICON            107,BPQICON,14,9,20,20
    LTEXT           "G8BPQ Mail and Chat Version ",-1,39,15,101,8,
                    SS_NOPREFIX | NOT WS_GROUP
    LTEXT           Verstring,-1,139,15,58,8,NOT WS_GROUP
    LTEXT           VerCopyright,-1,10,32,180,13,NOT WS_GROUP | ES_CENTER   
                    
    DEFPUSHBUTTON   "OK",IDOK,88,51,30,11,BS_CENTER | BS_VCENTER | WS_GROUP
END

#endif

#ifdef APRS

IDD_ABOUTBOX DIALOG DISCARDABLE  22, 17, 234, 161
STYLE DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "About"
FONT 8, "System"
BEGIN
    ICON            107,BPQICON,14,9,20,20
    LTEXT           "G8BPQ APRS Client/I-Gate",-1,39,15,96,8,SS_NOPREFIX | 
                    NOT WS_GROUP
    LTEXT           "0.0.0.1",-1,143,15,52,8,NOT WS_GROUP
    CTEXT           "Copyright © 2011 John Wiseman G8BPQ",-1,10,28,180,8,NOT 
                    WS_GROUP
    DEFPUSHBUTTON   "OK",IDOK,100,136,30,11,BS_CENTER | BS_VCENTER | 
                    WS_GROUP
    LTEXT           "APRS® is a registered trademark of Bob Bruninga.",-1,27,
                    43,173,13,NOT WS_GROUP
    LTEXT           "  Includes code from libpng (http://www.libpng.org",-1,
                    31,59,180,8,NOT WS_GROUP
    LTEXT           "             libpng version 1.2.8 - December 3, 2004\n    Copyright (c) 1998-2004 Glenn Randers-Pehrson\n    Copyright (c) 1996-1997 Andreas Dilger\n    Copyright (c) 1995-1996 Guy Eric Schalnat, Group 42, Inc.\n",
                    -1,15,73,200,40,NOT WS_GROUP
    LTEXT           "Mapping from OpenStreetMap (http://openstreetmap.org)",-1,25,117,
                    200,10,NOT WS_GROUP
    LTEXT           "Includes code from zlib (http://zlib.net)",-1,49,137,
                    135,10,NOT WS_GROUP
END

#endif
