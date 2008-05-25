// BPQCtrlPpg.cpp : Implementation of the CBPQCtrlPropPage property page class.

#include "stdafx.h"
#include "BPQCtrl.h"
#include "BPQCtrlPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CBPQCtrlPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CBPQCtrlPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CBPQCtrlPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CBPQCtrlPropPage, "BPQCTRL.BPQCtrlPropPage.1",
	0x4750c5e4, 0x8c82, 0x11d4, 0x9e, 0x57, 0, 0x50, 0xbf, 0x11, 0x48, 0x7)


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlPropPage::CBPQCtrlPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CBPQCtrlPropPage

BOOL CBPQCtrlPropPage::CBPQCtrlPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_BPQCTRL_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlPropPage::CBPQCtrlPropPage - Constructor

CBPQCtrlPropPage::CBPQCtrlPropPage() :
	COlePropertyPage(IDD, IDS_BPQCTRL_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CBPQCtrlPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlPropPage::DoDataExchange - Moves data between page and properties

void CBPQCtrlPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CBPQCtrlPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CBPQCtrlPropPage message handlers
