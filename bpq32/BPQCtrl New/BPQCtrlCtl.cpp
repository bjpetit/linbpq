// BPQCtrlCtl.cpp : Implementation of the CBPQCtrlCtrl OLE control class.


// Version 1.0.3.1 March 2006

//	Add BPQDirectory Property


// Version 1.0.3.2 March 2006

//	Correct definition of SetAppl prototype (Flags and Mask were swapped)


// Version 1.0.3.3 October 2006

//	Added APPLnCALL, ALIAS and QUAL Properties

// *** Remember to update Dialog box as well as Version resource and GetVersion routine ***


#define OLE2ANSI

#include "stdafx.h"
#include "BPQCtrl.h"
#include "BPQCtrlCtl.h"
#include "BPQCtrlPpg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "bpq32.h"

IMPLEMENT_DYNCREATE(CBPQCtrlCtrl, COleControl)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CBPQCtrlCtrl, COleControl)
	//{{AFX_MSG_MAP(CBPQCtrlCtrl)
//	ON_WM_TIMER()
 //   ON_REGISTERED_MESSAGE( BPQMsg, OnBPQ )
//	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
//	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
//	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)

//END_MESSAGE_MAP()*/

	//{{AFX_MSG_MAP(CBPQCtrlCtrl)
	{ 0x0113, 0, 0, 0, AfxSig_vw, (AFX_PMSG)(AFX_PMSGW)(void ( CWnd::*)(UINT))&CBPQCtrlCtrl::OnTimer },
    { 0xC000, 0, 0, 0, (UINT)(UINT*)(&BPQMsg), (AFX_PMSG)(AFX_PMSGW)(LRESULT ( CWnd::*)(WPARAM, LPARAM))&CBPQCtrlCtrl::OnBPQ },
	{ 0x0203, 0, 0, 0, AfxSig_vwp, (AFX_PMSG)(AFX_PMSGW)(void ( CWnd::*)(UINT, CPoint))&CBPQCtrlCtrl::OnLButtonDblClk },
	//}}AFX_MSG_MAP
	{ 0xC002, 0, 1, 1, 0xFE40, (AFX_PMSG)(BOOL ( CCmdTarget::*)(LPMSG, HWND, LPCRECT))&CBPQCtrlCtrl::OnEdit },
	{ 0xC002, 0, 1, 1, 0xFE41, (AFX_PMSG)(BOOL ( CCmdTarget::*)(LPMSG, HWND, LPCRECT))&CBPQCtrlCtrl::OnProperties },

{0, 0, 0, 0, AfxSig_end, (AFX_PMSG)0 } };


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CBPQCtrlCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CBPQCtrlCtrl)
	DISP_PROPERTY_EX(CBPQCtrlCtrl, "FreeBuffers", &CBPQCtrlCtrl::GetFreeBuffCount, &COleControl::SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(CBPQCtrlCtrl, "NumberofPorts", &CBPQCtrlCtrl::NumberofPorts, &COleControl::SetNotSupported, VT_I4)
	DISP_PROPERTY_EX(CBPQCtrlCtrl, "NodeCall", &CBPQCtrlCtrl::GetNodeCallx, &COleControl::SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(CBPQCtrlCtrl, "NodeAlias", &CBPQCtrlCtrl::GetNodeAliasx, &COleControl::SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(CBPQCtrlCtrl, "BBSCall", &CBPQCtrlCtrl::GetBBSCallx, &COleControl::SetNotSupported, VT_BSTR)
	DISP_PROPERTY_EX(CBPQCtrlCtrl, "BBSAlias", &CBPQCtrlCtrl::GetBBSAliasx, &COleControl::SetNotSupported, VT_BSTR)
	DISP_FUNCTION(CBPQCtrlCtrl, "Init", &CBPQCtrlCtrl::Init, VT_I4, VTS_NONE)
	DISP_FUNCTION(CBPQCtrlCtrl, "SendData", &CBPQCtrlCtrl::SendData, VT_I4, VTS_I4 VTS_BSTR VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "AllocateStream", &CBPQCtrlCtrl::AllocateStreamx, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "FindFreeStream", &CBPQCtrlCtrl::FindFreeStreamx, VT_I4, VTS_NONE)
	DISP_FUNCTION(CBPQCtrlCtrl, "Connect", &CBPQCtrlCtrl::Connect, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "Disconnect", &CBPQCtrlCtrl::Disconnect, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "ReturntoNode", &CBPQCtrlCtrl::SetFlags, VT_EMPTY, VTS_I4 VTS_I4 VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetMonFrame", &CBPQCtrlCtrl::GetMonFrame, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetAndDecodeMonFrame", &CBPQCtrlCtrl::GetAndDecodeMonFrame, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetData", &CBPQCtrlCtrl::GetData, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "SendRaw", &CBPQCtrlCtrl::SendRawx, VT_I4, VTS_I4 VTS_BSTR VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetStreamPort", &CBPQCtrlCtrl::GetStreamPort, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetStreamPaclen", &CBPQCtrlCtrl::GetStreamPaclen, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetStreamType", &CBPQCtrlCtrl::GetStreamType, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetStreamMaxframe", &CBPQCtrlCtrl::GetStreamMaxframe, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetStreamL4Window", &CBPQCtrlCtrl::GetStreamL4Window, VT_I4, VTS_I4)
//	DISP_PROPERTY_PARAM(CBPQCtrlCtrl, "ConnectState", &CBPQCtrlCtrl::GetConnectState, &CBPQCtrlCtrl:SetConnectState, VT_I4, VTS_I4)
//	DISP_PROPERTY_PARAM(CBPQCtrlCtrl, "Callsign", &CBPQCtrlCtrl::GetCall, &CBPQCtrlCtrl:SetCallsign, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "DeallocateStream", &CBPQCtrlCtrl::DeallocateStreamx, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetAndDecodeAGWFrame", &CBPQCtrlCtrl::GetAndDecodeAGWFrame, VT_BSTR, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetMonCount", &CBPQCtrlCtrl::GetMonCount, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetRXCount", &CBPQCtrlCtrl::GetRXCount, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetTXCount", &CBPQCtrlCtrl::GetTXCount, VT_I4, VTS_I4)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetVersion", &CBPQCtrlCtrl::GetVersion, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(CBPQCtrlCtrl, "GetMonFrameAndTimeStamp", &CBPQCtrlCtrl::GetMonFrameAndTimeStamp, VT_BSTR, VTS_I4 VTS_PI4 VTS_PI4)
	DISP_FUNCTION(CBPQCtrlCtrl, "DecodeFrameAGW", &CBPQCtrlCtrl::DecodeFrameAGW, VT_BSTR, VTS_BSTR VTS_I4 VTS_PI4)
	DISP_FUNCTION(CBPQCtrlCtrl, "DecodeFrame", &CBPQCtrlCtrl::DecodeFramex, VT_BSTR, VTS_BSTR VTS_I4)
	DISP_PROPERTY_EX(CBPQCtrlCtrl, "BPQDirectory", &CBPQCtrlCtrl::GetBPQDirectoryx, &COleControl::SetNotSupported, VT_BSTR)
	DISP_PROPERTY_PARAM(CBPQCtrlCtrl, "ApplCall", &CBPQCtrlCtrl::GetAppl1Callx, &CBPQCtrlCtrl::SetAppl1Callx, VT_BSTR, VTS_I4)
	DISP_PROPERTY_PARAM(CBPQCtrlCtrl, "ApplAlias", &CBPQCtrlCtrl::GetAppl1Aliasx, &CBPQCtrlCtrl::SetAppl1Aliasx, VT_BSTR, VTS_I4)
	DISP_PROPERTY_PARAM(CBPQCtrlCtrl, "ApplQual", &CBPQCtrlCtrl::GetAppl1Qualx, &CBPQCtrlCtrl::SetAppl1Qualx, VT_I4, VTS_I4)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CBPQCtrlCtrl, "AboutBox", DISPID_ABOUTBOX, &CBPQCtrlCtrl::AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CBPQCtrlCtrl, COleControl)
	//{{AFX_EVENT_MAP(CBPQCtrlCtrl)
	EVENT_CUSTOM("DataAvail", FireDataAvail, VTS_I4)
	EVENT_CUSTOM("MonDataAvail", FireMonDataAvail, VTS_I4)
	EVENT_CUSTOM("Connected", FireConnected, VTS_I4)
	EVENT_CUSTOM("Disconnected", FireDisconnected, VTS_I4)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CBPQCtrlCtrl, 1)
	PROPPAGEID(CBPQCtrlPropPage::guid)
END_PROPPAGEIDS(CBPQCtrlCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CBPQCtrlCtrl, "BPQCTRL.BPQCtrlCtrl.1",
	0x4750c5e3, 0x8c82, 0x11d4, 0x9e, 0x57, 0, 0x50, 0xbf, 0x11, 0x48, 0x7)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CBPQCtrlCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DBPQCtrl =
		{ 0x4750c5e1, 0x8c82, 0x11d4, { 0x9e, 0x57, 0, 0x50, 0xbf, 0x11, 0x48, 0x7 } };
const IID BASED_CODE IID_DBPQCtrlEvents =
		{ 0x4750c5e2, 0x8c82, 0x11d4, { 0x9e, 0x57, 0, 0x50, 0xbf, 0x11, 0x48, 0x7 } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwBPQCtrlOleMisc =
//	OLEMISC_INVISIBLEATRUNTIME |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE |
	OLEMISC_ACTIVATEWHENVISIBLE;

IMPLEMENT_OLECTLTYPE(CBPQCtrlCtrl, IDS_BPQCTRL, _dwBPQCtrlOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl::CBPQCtrlCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CBPQCtrlCtrl

BOOL CBPQCtrlCtrl::CBPQCtrlCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_BPQCTRL,
			IDB_BPQCTRL,
			TRUE,                       //  Insertable
			_dwBPQCtrlOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl::CBPQCtrlCtrl - Constructor

CBPQCtrlCtrl::CBPQCtrlCtrl()
{
	InitializeIIDs(&IID_DBPQCtrl, &IID_DBPQCtrlEvents);

	// TODO: Initialize your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl::~CBPQCtrlCtrl - Destructor

CBPQCtrlCtrl::~CBPQCtrlCtrl()
{
	// TODO: Cleanup your control's instance data here.
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl::OnDraw - Drawing function

void CBPQCtrlCtrl::OnDraw(
			CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid)
{
	// TODO: Replace the following code with your own drawing code.
	pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)));
	pdc->Ellipse(rcBounds);
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl::DoPropExchange - Persistence support

void CBPQCtrlCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl::OnResetState - Reset control to default state

void CBPQCtrlCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl::AboutBox - Display an "About" box to the user

void CBPQCtrlCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_BPQCTRL);
	dlgAbout.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl message handlers

long CBPQCtrlCtrl::Init() 
{
	// TODO: Add your dispatch handler code here
	
//	Invalidate(TRUE);
//	COleControl::OnLButtonDblClk(0,0);
	return 0;
}


long CBPQCtrlCtrl::SendData(long Stream, LPCTSTR Message, long Length) 
{

	return (SendMsg(Stream,Message,Length));
	return(0);

}

long CBPQCtrlCtrl::FindFreeStreamx() 
{
	int Stream;
	HWND hWnd;

	Stream = FindFreeStream();

	if (Stream != 255)
	{
//		hWnd=COleControl::GetSafeHwnd();
		hWnd=CBPQCtrlCtrl::GetSafeHwnd();
		BPQSetHandle(Stream, hWnd );
	}

	return Stream;

}

long CBPQCtrlCtrl::AllocateStreamx(long Stream) 
{
	int ret;
	HWND hWnd;
	
	ret=AllocateStream(Stream);

	if (ret == 0)
	{
		hWnd=m_hWnd;
		hWnd=COleControl::GetSafeHwnd();
		hWnd=CBPQCtrlCtrl::GetSafeHwnd();
		BPQSetHandle(Stream, hWnd );
	}

	return ret;
}


void CBPQCtrlCtrl::OnTimer(UINT nIDEvent) 
{
	// TODO: Add your message handler code here and/or call default
	
	COleControl::OnTimer(nIDEvent);
}

LRESULT CBPQCtrlCtrl::OnBPQ(WPARAM Stream, LPARAM Change) 
{
	int state, change;

	switch (Change)
	{
	case BPQMonitorAvail:
		
		FireMonDataAvail(Stream);
		break;
				
	case BPQDataAvail:
	
		FireDataAvail(Stream);
		break;
	
	case BPQStateChange:

		//	Get current Session State. Any state changed is ACK'ed
		//	automatically. See BPQHOST functions 4 and 5.
	
		SessionState(Stream, &state, &change);
		
		if (change == 1)
		{
			if (state == 1)
	
			// Connected
			
				FireConnected(Stream);	
			else
				FireDisconnected(Stream);
		}

		break;

	}
	
	return 0;
}

long CBPQCtrlCtrl::GetFreeBuffCount() 
{
	return GetFreeBuffs();
}


//	Send Session Control command (BPQHOST function 6)
//	Command = 0 Connect using APPL MASK IN param
//	Command = 1 Connect
//	Command = 2 Disconect
//	Command = 3 Return to Node



long CBPQCtrlCtrl::Connect(long Stream) 
{
	return SessionControl(Stream, 1, 0);
}

long CBPQCtrlCtrl::Disconnect(long Stream) 
{
	return SessionControl(Stream, 2, 0);
}

long CBPQCtrlCtrl::GetConnectState(long Stream) 
{
	int State;
	
	SessionStateNoAck(Stream, &State);

	return State;
}

void CBPQCtrlCtrl::SetConnectState(long Stream, long nNewValue) 
{
	// TODO: Add your property handler here

	SetModifiedFlag();
}

long CBPQCtrlCtrl::NumberofPorts() 
{
	return  GetNumberofPorts();
}

void CBPQCtrlCtrl::ReturntoNode(long Stream) 
{
	SessionControl(Stream, 3, 0);
}

void CBPQCtrlCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	COleControl::OnLButtonDblClk(nFlags, point);
}

BSTR CBPQCtrlCtrl::GetCall(long Stream) 
{
	char call[11];
	int ret;
	
	CString strResult="            ";


	ret=GetCallsign(Stream, call);
	
	if (ret == 0)	
		call[10]=0;
	else
		call[0]=0;			// Return null string if not connected

	strResult = call;

	return strResult.AllocSysString();
}

void CBPQCtrlCtrl::SetCallsign(long Stream, LPCTSTR lpszNewValue) 
{
	// TODO: Add your property handler here

	SetModifiedFlag();
}

BSTR CBPQCtrlCtrl::GetData(long Stream) 
{
	int count,len;
	UCHAR buffer[500];

	GetMsg(Stream,buffer, &len, &count );
 
	CString strResult((LPCTSTR)buffer,len);

	return strResult.AllocSysString();
}

BSTR CBPQCtrlCtrl::GetMonFrame(long Stream) 
{
	int count,len;
	UCHAR buffer[500];

	GetRaw(Stream,(char *)buffer, &len, &count );
 
	CString strResult((LPCTSTR)buffer,len);

	return strResult.AllocSysString();
}


void CBPQCtrlCtrl::SetFlags(long Stream, long Mask, long Flags) 
{
	SetAppl(Stream, Flags, Mask);
}

//BSTR CBPQCtrlCtrl::GetMonFrame(long Stream) 
//{
//	CString strResult;
	// TODO: Add your dispatch handler code here

//	return strResult.AllocSysString();
//}

//BSTR CBPQCtrlCtrl::DecodeFrameX(LPCTSTR Msg, long Len, long Stamp) 
//{
//	CString strResult;
	// TODO: Add your dispatch handler code here

//	return strResult.AllocSysString();
//}

BSTR CBPQCtrlCtrl::GetAndDecodeMonFrame(long Stream) 
{
	int count,len,Stamp;
	UCHAR buffer[500];
	UCHAR buffer2[2000];

	Stamp=GetRaw(Stream,(char *)buffer, &len, &count );
	
	len=DecodeFrame((char *)buffer,(char *)buffer2,Stamp);
	
	CString strResult((LPCTSTR)buffer2,len);

	return strResult.AllocSysString();
}

BSTR CBPQCtrlCtrl::GetNodeCallx() 
{
	UCHAR * CallPtr;

	CallPtr=GetNodeCall();

	CString strResult((LPCTSTR)CallPtr,10);
	
	return strResult.AllocSysString();
}

BSTR CBPQCtrlCtrl::GetNodeAliasx() 
{
	UCHAR * CallPtr;

	CallPtr=GetNodeAlias();

	CString strResult((LPCTSTR)CallPtr,6);
	
	return strResult.AllocSysString();

}

BSTR CBPQCtrlCtrl::GetBBSCallx() 
{
	UCHAR * CallPtr;

	CallPtr=GetBBSCall();

	CString strResult((LPCTSTR)CallPtr,10);
	
	return strResult.AllocSysString();

}

BSTR CBPQCtrlCtrl::GetBBSAliasx() 
{
	UCHAR * CallPtr;

	CallPtr=GetBBSAlias();

	CString strResult((LPCTSTR)CallPtr,6);

	return strResult.AllocSysString();
}

//BSTR CBPQCtrlCtrl::GetData(long Stream) 
//{
//	CString strResult;
	// TODO: Add your dispatch handler code here

//	return strResult.AllocSysString();
//}

long CBPQCtrlCtrl::SendRawx(long Port, LPCTSTR Msg, long Length) 
{
		return (SendRaw(Port,Msg,Length));
}

long CBPQCtrlCtrl::GetStreamPort(long Stream) 
{
	char call[11];
	int port, sesstype, paclen, maxframe, l4window;

	GetConnectionInfo(Stream, call, &port, &sesstype, &paclen, &maxframe, &l4window);

	return port;
}

long CBPQCtrlCtrl::GetStreamPaclen(long Stream) 
{
	char call[11];
	int port, sesstype, paclen, maxframe, l4window;

	GetConnectionInfo(Stream, call, &port, &sesstype, &paclen, &maxframe, &l4window);

	return paclen;

	return 0;
}

long CBPQCtrlCtrl::GetStreamType(long Stream) 
{
	char call[11];
	int port, sesstype, paclen, maxframe, l4window;

	GetConnectionInfo(Stream, call, &port, &sesstype, &paclen, &maxframe, &l4window);

	return sesstype;

	return 0;
}

long CBPQCtrlCtrl::GetStreamMaxframe(long Stream) 
{
	char call[11];
	int port, sesstype, paclen, maxframe, l4window;

	GetConnectionInfo(Stream, call, &port, &sesstype, &paclen, &maxframe, &l4window);

	return maxframe;

	return 0;
}

long CBPQCtrlCtrl::GetStreamL4Window(long Stream) 
{
	char call[11];
	int port, sesstype, paclen, maxframe, l4window;

	GetConnectionInfo(Stream, call, &port, &sesstype, &paclen, &maxframe, &l4window);

	return l4window;

	return 0;
}

long CBPQCtrlCtrl::DeallocateStreamx(long Stream) 
{
	int ret;
	
	ret=DeallocateStream(Stream);

	BPQSetHandle(Stream, 0);
	
	return ret;

	return 0;
}

int InternalAGWDecodeFrame(char * msg, char * buffer, int Stamp, long * FrameType);


BSTR CBPQCtrlCtrl::GetAndDecodeAGWFrame(long Stream) 
{
	int count,len,Stamp;
	long FrameType;
	UCHAR buffer[500];
	UCHAR buffer2[2000];

	Stamp=GetRaw(Stream,(char *)buffer, &len, &count );
	
	len=InternalAGWDecodeFrame((char *)buffer,(char *)buffer2,Stamp,&FrameType);
	
	CString strResult((LPCTSTR)buffer2,len);

	return strResult.AllocSysString();

}
long CBPQCtrlCtrl::GetMonCount(long Stream) 
{
	return MONCount(Stream);
}

long CBPQCtrlCtrl::GetRXCount(long Stream) 
{
	return RXCount(Stream);

}

long CBPQCtrlCtrl::GetTXCount(long Stream) 
{
	return TXCount(Stream);

}


BSTR CBPQCtrlCtrl::GetVersion() 
{
	CString strResult="1.0.3.3 November 2006";

	return strResult.AllocSysString();
}


BSTR CBPQCtrlCtrl::GetMonFrameAndTimeStamp(long Stream, long FAR* Stamp, long FAR* Count) 
{
	int len,ret,count;
	UCHAR buffer[500];

	ret=GetRaw(Stream,(char *)buffer, &len,&count );
 
	*Stamp=ret;
	*Count=count;
	
	CString strResult((LPCTSTR)buffer,len);

	return strResult.AllocSysString();
}

extern "C"  int AGWMONDECODE();
extern "C"  int AGWMONOPTIONS();


int InternalAGWDecodeFrame(char * msg, char * buffer, int Stamp, long * FrameType)
{

//	This is not an API function. It is a utility to decode a received
//	monitor frame into ascii text.
	int	returnit;


	_asm {

	pushfd
	cld
	pushad


	mov	esi,msg
	mov	eax,Stamp
	mov	edi,buffer

	call	AGWMONDECODE

	mov	edi,FrameType
	mov	[edi],eax
	mov	returnit,ecx


	popad
	popfd

	}				// End of ASM

	return (returnit);

}





BSTR CBPQCtrlCtrl::DecodeFrameAGW(LPCTSTR buffer, long Stamp, long FAR* FrameControl) 
{
	int len;
	UCHAR buffer2[1000];

	len=InternalAGWDecodeFrame((char *)buffer,(char *)buffer2,Stamp,FrameControl);
	
	CString strResult((LPCTSTR)buffer2,len);

	return strResult.AllocSysString();
}

BSTR CBPQCtrlCtrl::DecodeFramex(LPCTSTR Buffer, long Stamp) 
{
	int len;
	UCHAR buffer2[500];

	len=DecodeFrame((char *)Buffer,(char *)buffer2,Stamp);
	
	CString strResult((LPCTSTR)buffer2,len);

	return strResult.AllocSysString();

}



BSTR CBPQCtrlCtrl::GetBPQDirectoryx() 
{
	unsigned char * DirPtr;
	int Dirlen;

	DirPtr=GetBPQDirectory();

	Dirlen=strlen((const char *)DirPtr);

	CString strResult((LPCTSTR)DirPtr,Dirlen);
	
	return strResult.AllocSysString();

}

BSTR CBPQCtrlCtrl::GetAppl1Callx(long Appl) 
{
	char * DirPtr;

	DirPtr=GetApplCall(Appl);

	if (DirPtr == 0) return 0;

	CString strResult((LPCTSTR)DirPtr,9);
	
	return strResult.AllocSysString();

}

BOOL CBPQCtrlCtrl::SetAppl1Callx(long Appl, LPCTSTR lpszNewValue) 
{
	return SetApplCall(Appl,lpszNewValue);
}

BSTR CBPQCtrlCtrl::GetAppl1Aliasx(long Appl) 
{
	char * DirPtr;

	DirPtr=GetApplAlias(Appl);

	if (DirPtr == 0) return 0;

	CString strResult((LPCTSTR)DirPtr,6);
	
	return strResult.AllocSysString();

}

BOOL CBPQCtrlCtrl::SetAppl1Aliasx(long Appl, LPCTSTR lpszNewValue) 
{
	return SetApplAlias(Appl,lpszNewValue);
}


long CBPQCtrlCtrl::GetAppl1Qualx(long Appl) 
{	
	return GetApplQual(Appl);
}

BOOL CBPQCtrlCtrl::SetAppl1Qualx(long Appl, long NewValue) 
{
	return SetApplQual(Appl, NewValue);
}

