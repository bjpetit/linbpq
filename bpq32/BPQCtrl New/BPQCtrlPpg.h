// BPQCtrlPpg.h : Declaration of the CBPQCtrlPropPage property page class.

////////////////////////////////////////////////////////////////////////////
// CBPQCtrlPropPage : See BPQCtrlPpg.cpp.cpp for implementation.

class CBPQCtrlPropPage : public COlePropertyPage
{
	DECLARE_DYNCREATE(CBPQCtrlPropPage)
	DECLARE_OLECREATE_EX(CBPQCtrlPropPage)

// Constructor
public:
	CBPQCtrlPropPage();

// Dialog Data
	//{{AFX_DATA(CBPQCtrlPropPage)
	enum { IDD = IDD_PROPPAGE_BPQCTRL };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA

// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Message maps
protected:
	//{{AFX_MSG(CBPQCtrlPropPage)
		// NOTE - ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
