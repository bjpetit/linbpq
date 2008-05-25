// BPQCtrlCtl.h : Declaration of the CBPQCtrlCtrl OLE control class.

/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlCtrl : See BPQCtrlCtl.cpp for implementation.

class CBPQCtrlCtrl : public COleControl
{
	DECLARE_DYNCREATE(CBPQCtrlCtrl)

// Constructor
public:
	CBPQCtrlCtrl();


    //{{xAFX_MSG(CBPQCtrlCtrl)
    afx_msg LRESULT OnBPQ(WPARAM wParam, LPARAM lParam);
    //}}xAFX_MSG



// Overrides

	// Drawing function
	virtual void OnDraw(
				CDC* pdc, const CRect& rcBounds, const CRect& rcInvalid);

	// Persistence
	virtual void DoPropExchange(CPropExchange* pPX);

	// Reset control state
	virtual void OnResetState();

// Implementation
protected:
	~CBPQCtrlCtrl();

	DECLARE_OLECREATE_EX(CBPQCtrlCtrl)    // Class factory and guid
	DECLARE_OLETYPELIB(CBPQCtrlCtrl)      // GetTypeInfo
	DECLARE_PROPPAGEIDS(CBPQCtrlCtrl)     // Property page IDs
	DECLARE_OLECTLTYPE(CBPQCtrlCtrl)		// Type name and misc status

// Message maps
	//{{AFX_MSG(CBPQCtrlCtrl)
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Dispatch maps
	//{{AFX_DISPATCH(CBPQCtrlCtrl)
	afx_msg long GetFreeBuffCount();
	afx_msg long NumberofPorts();
	afx_msg BSTR GetNodeCallx();
	afx_msg BSTR GetNodeAliasx();
	afx_msg BSTR GetBBSCallx();
	afx_msg BSTR GetBBSAliasx();
	afx_msg long Init();
	afx_msg long SendData(long Stream, LPCTSTR Message, long Length);
	afx_msg long AllocateStreamx(long Stream);
	afx_msg long FindFreeStreamx();
	afx_msg long Connect(long Stream);
	afx_msg long Disconnect(long Stream);
	afx_msg void ReturntoNode(long Stream);
	afx_msg void SetFlags(long Stream, long Mask, long Flags);
	afx_msg BSTR GetMonFrame(long Stream);
	afx_msg BSTR GetAndDecodeMonFrame(long Stream);
	afx_msg BSTR GetData(long Stream);
	afx_msg long SendRawx(long Port, LPCTSTR Msg, long Length);
	afx_msg long GetStreamPort(long Stream);
	afx_msg long GetStreamPaclen(long Stream);
	afx_msg long GetStreamType(long Stream);
	afx_msg long GetStreamMaxframe(long Stream);
	afx_msg long GetStreamL4Window(long Stream);
	afx_msg long DeallocateStreamx(long Stream);
	afx_msg BSTR GetAndDecodeAGWFrame(long Stream);
	afx_msg long GetMonCount(long Streeam);
	afx_msg long GetRXCount(long Stream);
	afx_msg long GetTXCount(long Stream);
	afx_msg BSTR GetVersion();
	afx_msg BSTR GetMonFrameAndTimeStamp(long Stream, long FAR* Stamp, long FAR* Count);
	afx_msg BSTR DecodeFrameAGW(LPCTSTR Buffer, long Stamp, long FAR* FrameType);
	afx_msg BSTR DecodeFramex(LPCTSTR Buffer, long Stamp);
	afx_msg long GetConnectState(long Stream);
	afx_msg void SetConnectState(long Stream, long nNewValue);
	afx_msg BSTR GetCall(long Stream);
	afx_msg void SetCallsign(long Stream, LPCTSTR lpszNewValue);
	afx_msg BSTR GetBPQDirectoryx();
	afx_msg BSTR GetAppl1Callx(long Appl);
	afx_msg BOOL SetAppl1Callx(long Appl, LPCTSTR lpszNewValue);
	afx_msg BSTR GetAppl1Aliasx(long Appl);
	afx_msg BOOL SetAppl1Aliasx(long Appl, LPCTSTR lpszNewValue);
	afx_msg long GetAppl1Qualx(long Appl);
	afx_msg BOOL SetAppl1Qualx(long Appl, long NewValue);
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()

	afx_msg void AboutBox();

// Event maps
	//{{AFX_EVENT(CBPQCtrlCtrl)
	void FireDataAvail(long Stream)
		{FireEvent(eventidDataAvail,EVENT_PARAM(VTS_I4), Stream);}
	void FireMonDataAvail(long Stream)
		{FireEvent(eventidMonDataAvail,EVENT_PARAM(VTS_I4), Stream);}
	void FireConnected(long Stream)
		{FireEvent(eventidConnected,EVENT_PARAM(VTS_I4), Stream);}
	void FireDisconnected(long Stream)
		{FireEvent(eventidDisconnected,EVENT_PARAM(VTS_I4), Stream);}
	//}}AFX_EVENT
	DECLARE_EVENT_MAP()

// Dispatch and event IDs
public:
	enum {
	//{{AFX_DISP_ID(CBPQCtrlCtrl)
	dispidFreeBuffers = 1L,
	dispidNumberofPorts = 2L,
	dispidNodeCall = 3L,
	dispidNodeAlias = 4L,
	dispidBBSCall = 5L,
	dispidBBSAlias = 6L,
	dispidInit = 7L,
	dispidSendData = 8L,
	dispidFindFreeStream = 10L,
	dispidConnect = 11L,
	dispidDisconnect = 12L,
	dispidConnectState = 24L,
	dispidReturntoNode = 13L,
	dispidCallsign = 25L,
	dispidSetFlags = 14L,
	dispidGetMonFrame = 15L,
	dispidGetAndDecodeMonFrame = 16L,
	dispidGetData = 17L,
	dispidSendRaw = 18L,
	dispidGetStreamPort = 19L,
	dispidGetStreamPaclen = 20L,
	dispidGetStreamType = 21L,
	dispidGetStreamMaxframe = 22L,
	dispidGetStreamL4Window = 23L,
	dispidDeallocateStream = 26L,
	dispidGetAndDecodeAGWFrame = 27L,
	dispidGetMonCount = 28L,
	dispidGetRXCount = 29L,
	dispidGetTXCount = 30L,
	dispidGetVersion = 31L,
	dispidGetMonFrameAndTimeStamp = 32L,
	dispidDecodeFrameAGW = 33L,
	dispidDecodeFrame = 34L,
	dispidBPQDirectory = 35L,
	dispidApplCall = 36L,
	dispidApplAlias = 37L,
	dispidApplQual= 38L,
	eventidDataAvail = 1L,
	eventidMonDataAvail = 2L,
	eventidConnected = 3L,
	eventidDisconnected = 4L,
	//}}AFX_DISP_ID
	};
};
