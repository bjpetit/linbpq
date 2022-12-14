// BPQCtrl.cpp : Implementation of CBPQCtrlApp and DLL registration.

#include "stdafx.h"
#include "BPQCtrl.h"

extern UINT BPQMsg;


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CBPQCtrlApp NEAR theApp;

const GUID CDECL BASED_CODE _tlid =
		{ 0x4750c5e0, 0x8c82, 0x11d4, { 0x9e, 0x57, 0, 0x50, 0xbf, 0x11, 0x48, 0x7 } };
const WORD _wVerMajor = 1;
const WORD _wVerMinor = 0;


////////////////////////////////////////////////////////////////////////////
// CBPQCtrlApp::InitInstance - DLL initialization

BOOL CBPQCtrlApp::InitInstance()
{

	BOOL bInit = COleControlModule::InitInstance();

	if (bInit)
	{

	BPQMsg = RegisterWindowMessage("BPQWindowMessage");

	}

	return bInit;
}


////////////////////////////////////////////////////////////////////////////
// CBPQCtrlApp::ExitInstance - DLL termination

int CBPQCtrlApp::ExitInstance()
{
	// TODO: Add your own module termination code here.

	return COleControlModule::ExitInstance();
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleRegisterTypeLib(AfxGetInstanceHandle(), _tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(TRUE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	AFX_MANAGE_STATE(_afxModuleAddrThis);

	if (!AfxOleUnregisterTypeLib(_tlid))
		return ResultFromScode(SELFREG_E_TYPELIB);

	if (!COleObjectFactoryEx::UpdateRegistryAll(FALSE))
		return ResultFromScode(SELFREG_E_CLASS);

	return NOERROR;
}
