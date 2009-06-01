		
// Mail and Chat Server for BPQ32 Packet Switch
//
//	Configuration Module

#include "stdafx.h"
#define C_PAGES 4

int CurrentPage=0;				// Page currently on show in tabbed Dialog

#define BBSPARAMS 0
#define ISPPARAMS 1
#define CHATPARAMS 2

extern HINSTANCE hInst;
extern char BBSName[];
extern char HRoute[];
extern char BBSApplNum;
extern int ChatApplNum;
extern char BaseDir[];
extern int SMTPInPort;
extern int POP3InPort;
extern int MaxStreams;
extern UCHAR * OtherNodes;

extern BOOL ISP_Gateway_Enabled;

extern char MyDomain[];			// Mail domain for BBS<>Internet Mapping

extern char ISPSMTPName[];
extern int ISPSMTPPort;

extern char ISPPOP3Name[];
extern int ISPPOP3Port;
extern int ISPPOP3Interval;

extern char ISPAccountName[];
extern char ISPAccountPass[];
extern char EncryptedISPAccountPass[];
extern int EncryptedPassLen;


typedef struct tag_dlghdr {

HWND hwndTab; // tab control
HWND hwndDisplay; // current child dialog box
RECT rcDisplay; // display rectangle for the tab control

DLGTEMPLATE *apRes[C_PAGES];

} DLGHDR;

HWND hwndDlg;		// Config Dialog


DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName);
VOID WINAPI OnSelChanged(HWND hwndDlg);
VOID WINAPI OnChildDialogInit(HWND hwndDlg);
VOID WINAPI OnTabbedDialogInit(HWND hwndDlg);

// POP3 Password is encrypted by xor'ing it with an MD5 hash of the hostname and pop2 server name


int EncryptPass(char * Pass, char * Encrypt)
{
	char hash[50];
	char key[100];
	unsigned int i;
	char hostname[100];
	char extendedpass[100];
	unsigned int passlen;

	gethostname(hostname, 100);

	strcpy(key, hostname);
	strcat(key, ISPPOP3Name);

	md5(key, hash);
	memcpy(&hash[16], hash, 16);	// in case very long password

	// if password is less than 16 chars, extend with zeros

	passlen=strlen(Pass);

	strcpy(extendedpass, Pass);

	if (passlen < 16)
	{
		for  (i=passlen+1; i <= 16; i++)
		{
			extendedpass[i] = 0;
		}

		passlen = 16;
	}

	for (i=0; i < passlen; i++)
	{
		Encrypt[i] = extendedpass[i] ^ hash[i];
	}

	return passlen;
}

VOID DecryptPass(char * Encrypt, char * Pass, unsigned int len)
{
	char hash[50];
	char key[100];
	unsigned int i;
	char hostname[100]="";

	gethostname(hostname, 100);

	i = GetLastError();


	strcpy(key, hostname);
	strcat(key, ISPPOP3Name);

	md5(key, hash);
	memcpy(&hash[16], hash, 16);	// in case very long password

	for (i=0; i < len; i++)
	{
		Pass[i] =  Encrypt[i] ^ hash[i];
	}

	return;
}




INT_PTR CALLBACK ConfigWndProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		OnTabbedDialogInit(hDlg);
		return (INT_PTR)TRUE;

	case WM_NOTIFY:

        switch (((LPNMHDR)lParam)->code)
        {
		case TCN_SELCHANGE:
			 OnSelChanged(hDlg);
				 return TRUE;
         // More cases on WM_NOTIFY switch.

        }

       break;

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case 1:							// OK
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		case 3:							// Reset
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		case 4:							// Save
			SaveConfig();
			return TRUE;
		case 2:							// Cancel
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;

		default:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;

}

INT_PTR CALLBACK ChildDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


//The following function processes the WM_INITDIALOG message for the main dialog box. The function allocates the DLGHDR structure, loads the dialog template resources for the child dialog boxes, and creates the tab control.

//The size of each child dialog box is specified by the DLGTEMPLATE structure. The function examines the size of each dialog box and uses the macro for the TCM_ADJUSTRECT message to calculate an appropriate size for the tab control. Then it sizes the dialog box and positions the two buttons accordingly. This example sends TCM_ADJUSTRECT by using the TabCtrl_AdjustRect macro.

VOID WINAPI OnTabbedDialogInit(HWND hDlg)
{
	DLGHDR *pHdr = (DLGHDR *) LocalAlloc(LPTR, sizeof(DLGHDR));
	DWORD dwDlgBase = GetDialogBaseUnits();
	int cxMargin = LOWORD(dwDlgBase) / 4;
	int cyMargin = HIWORD(dwDlgBase) / 8;

	TC_ITEM tie;
	RECT rcTab;
	HWND hwndButton;
	RECT rcButton;

	int i, pos;
	INITCOMMONCONTROLSEX init;

	hwndDlg = hDlg;			// Save Window Handle

	// Save a pointer to the DLGHDR structure.

	SetWindowLong(hwndDlg, GWL_USERDATA, (LONG) pHdr);

	// Create the tab control.


	init.dwICC=ICC_STANDARD_CLASSES;
	init.dwSize=sizeof(init);
	i=InitCommonControlsEx(&init);

	pHdr->hwndTab = CreateWindow(WC_TABCONTROL, "", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 100, 100, hwndDlg, NULL, hInst, NULL);

	if (pHdr->hwndTab == NULL) {

	// handle error

	}

	// Add a tab for each of the three child dialog boxes.

	tie.mask = TCIF_TEXT | TCIF_IMAGE;

	tie.iImage = -1;

	tie.pszText = "BBS Params";

	TabCtrl_InsertItem(pHdr->hwndTab, 0, &tie);

	tie.pszText = "ISP Interface";

	TabCtrl_InsertItem(pHdr->hwndTab, 1, &tie);

	tie.pszText = "Chat Params";

	TabCtrl_InsertItem(pHdr->hwndTab, 2, &tie);

	tie.pszText = "Misc Params";

	TabCtrl_InsertItem(pHdr->hwndTab, 3, &tie);

	// Lock the resources for the three child dialog boxes.

	pHdr->apRes[0] = DoLockDlgRes("BBS_CONFIG");

	pHdr->apRes[1] = DoLockDlgRes("ISP_CONFIG");

	pHdr->apRes[2] = DoLockDlgRes("CHAT_CONFIG");

	pHdr->apRes[3] = DoLockDlgRes("DIALOG_3");

//
	// Determine the bounding rectangle for all child dialog boxes.

	SetRectEmpty(&rcTab);

	for (i = 0; i < C_PAGES; i++)
	{
		if (pHdr->apRes[i]->cx > rcTab.right)
			rcTab.right = pHdr->apRes[i]->cx;

		if (pHdr->apRes[i]->cy > rcTab.bottom)
			rcTab.bottom = pHdr->apRes[i]->cy;

	}

	MapDialogRect(hwndDlg, &rcTab);

//	rcTab.right = rcTab.right * LOWORD(dwDlgBase) / 4;

//	rcTab.bottom = rcTab.bottom * HIWORD(dwDlgBase) / 8;

	// Calculate how large to make the tab control, so

	// the display area can accomodate all the child dialog boxes.

	TabCtrl_AdjustRect(pHdr->hwndTab, TRUE, &rcTab);

	OffsetRect(&rcTab, cxMargin - rcTab.left, cyMargin - rcTab.top);

	// Calculate the display rectangle.

	CopyRect(&pHdr->rcDisplay, &rcTab);

	TabCtrl_AdjustRect(pHdr->hwndTab, FALSE, &pHdr->rcDisplay);

	// Set the size and position of the tab control, buttons,

	// and dialog box.

	SetWindowPos(pHdr->hwndTab, NULL, rcTab.left, rcTab.top, rcTab.right - rcTab.left, rcTab.bottom - rcTab.top, SWP_NOZORDER);

	// Move the Buttons to bottom of page

	pos=rcTab.left+cxMargin;

	for (i = 1; i < 5; i++)
	{
		hwndButton = GetDlgItem(hwndDlg, i);
	
		SetWindowPos(hwndButton, NULL, pos, rcTab.bottom + cyMargin, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		GetWindowRect(hwndButton, &rcButton);

		pos = pos + (rcButton.right - rcButton.left) +2;
	}
	
	// Size the dialog box.



	SetWindowPos(hwndDlg, NULL, 0, 0, rcTab.right + cyMargin + 2 * GetSystemMetrics(SM_CXDLGFRAME),
		rcTab.bottom + rcButton.bottom - rcButton.top + 2 * cyMargin + 2 * GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION),
		SWP_NOMOVE | SWP_NOZORDER);

	// Simulate selection of the first item.

	OnSelChanged(hwndDlg);

}

// DoLockDlgRes - loads and locks a dialog template resource.

// Returns a pointer to the locked resource.

// lpszResName - name of the resource

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName)
{
	HRSRC hrsrc = FindResource(NULL, lpszResName, RT_DIALOG);
	HGLOBAL hglb = LoadResource(hInst, hrsrc);

	return (DLGTEMPLATE *) LockResource(hglb);
}


//The following function processes the TCN_SELCHANGE notification message for the main dialog box. The function destroys the dialog box for the outgoing page, if any. Then it uses the CreateDialogIndirect function to create a modeless dialog box for the incoming page.

// OnSelChanged - processes the TCN_SELCHANGE notification.

// hwndDlg - handle of the parent dialog box

VOID WINAPI OnSelChanged(HWND hwndDlg)
{
	int ptr;
	int len;
	char Nodes[1000]="";

	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	CurrentPage = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.

	if (pHdr->hwndDisplay != NULL)

		DestroyWindow(pHdr->hwndDisplay);

	// Create the new child dialog box.

	pHdr->hwndDisplay = CreateDialogIndirect(hInst, pHdr->apRes[CurrentPage], hwndDlg, ChildDialogProc);

	// Fill in the controls

	switch (CurrentPage)
	{
	case BBSPARAMS:
		
		SetDlgItemText(pHdr->hwndDisplay, IDC_BBSCall, BBSName);
		SetDlgItemText(pHdr->hwndDisplay, IDC_HRoute, HRoute);
		SetDlgItemText(pHdr->hwndDisplay, IDC_BaseDir, BaseDir);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_BBSAppl, BBSApplNum, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_BBSStreams, MaxStreams, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_POP3Port, POP3InPort, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_SMTPPort, SMTPInPort, FALSE);

		break;

	case ISPPARAMS:
		
		CheckDlgButton(pHdr->hwndDisplay, IDC_ISP_Gateway_Enabled, ISP_Gateway_Enabled);
 		
		SetDlgItemInt(pHdr->hwndDisplay, IDC_POP3Timer, ISPPOP3Interval, FALSE);

		SetDlgItemText(pHdr->hwndDisplay, IDC_MyMailDomain, BBSName);

		SetDlgItemText(pHdr->hwndDisplay, IDC_ISPSMTPName, ISPSMTPName);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_ISPSMTPPort, ISPSMTPPort, FALSE);

		SetDlgItemText(pHdr->hwndDisplay, IDC_ISPPOP3Name, ISPPOP3Name);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_ISPPOP3Port, ISPPOP3Port, FALSE);

		SetDlgItemText(pHdr->hwndDisplay, IDC_ISPAccountName, ISPAccountName);
		SetDlgItemText(pHdr->hwndDisplay, IDC_ISPAccountPass, ISPAccountPass);

		EncryptedPassLen = EncryptPass("gb7bpq", EncryptedISPAccountPass);


 		break;

	case CHATPARAMS:

		SetDlgItemInt(pHdr->hwndDisplay, IDC_ChatAppl, ChatApplNum, FALSE);

			// Set up other nodes list
	
		ptr=0;

		while (OtherNodes[ptr])
		{
			len=strlen(&OtherNodes[ptr]);
			strcat(Nodes, &OtherNodes[ptr]);
			strcat(Nodes, "\r\n");
			ptr+= (len + 1);
		}

		SetDlgItemText(pHdr->hwndDisplay, IDC_ChatNodes, Nodes);

		break;
	}

	ShowWindow(pHdr->hwndDisplay, SW_SHOWNORMAL);

}

//The following function processes the WM_INITDIALOG message for each of the child dialog boxes. You cannot specify the position of a dialog box created using the CreateDialogIndirect function. This function uses the SetWindowPos function to position the child dialog within the tab control's display area.

// OnChildDialogInit - Positions the child dialog box to fall

// within the display area of the tab control.

VOID WINAPI OnChildDialogInit(HWND hwndDlg)
{
	HWND hwndParent = GetParent(hwndDlg);
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndParent, GWL_USERDATA);

	SetWindowPos(hwndDlg, HWND_TOP, pHdr->rcDisplay.left, pHdr->rcDisplay.top, 0, 0, SWP_NOSIZE);
}

BOOL SaveConfig()
{
	BOOL OK1,OK2,OK3,OK4;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);
	HKEY hKey=0;
	int retCode,disp;
	char * ptr1, * ptr2;
	char Nodes[1000];

	switch (CurrentPage)
	{
	case BBSPARAMS:
		
		GetDlgItemText(pHdr->hwndDisplay, IDC_BBSCall, BBSName, 50);
		GetDlgItemText(pHdr->hwndDisplay, IDC_HRoute, HRoute, 50);
		GetDlgItemText(pHdr->hwndDisplay, IDC_BaseDir, BaseDir, 50);

		BBSApplNum = GetDlgItemInt(pHdr->hwndDisplay, IDC_BBSAppl, &OK1, FALSE);
		MaxStreams = GetDlgItemInt(pHdr->hwndDisplay, IDC_BBSStreams, &OK2, FALSE);
		POP3InPort = GetDlgItemInt(pHdr->hwndDisplay, IDC_POP3Port, &OK3, FALSE);
		SMTPInPort = GetDlgItemInt(pHdr->hwndDisplay, IDC_SMTPPort, &OK4, FALSE);

		break;

	case ISPPARAMS:
		
		ISP_Gateway_Enabled = IsDlgButtonChecked(pHdr->hwndDisplay, IDC_ISP_Gateway_Enabled);

		ISPPOP3Interval = GetDlgItemInt(pHdr->hwndDisplay, IDC_POP3Timer, &OK1, FALSE);

		GetDlgItemText(pHdr->hwndDisplay, IDC_MyMailDomain, MyDomain, 50);

		GetDlgItemText(pHdr->hwndDisplay, IDC_ISPSMTPName, ISPSMTPName, 50);
		ISPSMTPPort = GetDlgItemInt(pHdr->hwndDisplay, IDC_ISPSMTPPort, &OK2, FALSE);

		GetDlgItemText(pHdr->hwndDisplay, IDC_ISPPOP3Name, ISPPOP3Name, 50);
		ISPPOP3Port = GetDlgItemInt(pHdr->hwndDisplay, IDC_ISPPOP3Port, &OK3, FALSE);

		GetDlgItemText(pHdr->hwndDisplay, IDC_ISPAccountName, ISPAccountName, 50);
		GetDlgItemText(pHdr->hwndDisplay, IDC_ISPAccountPass, ISPAccountPass, 50);

 		break;

	case CHATPARAMS:

		ChatApplNum = GetDlgItemInt(pHdr->hwndDisplay, IDC_ChatAppl, &OK1, FALSE);

		GetDlgItemText(pHdr->hwndDisplay, IDC_ChatNodes, Nodes, 1000);

		// replace crlf with single null

		if (Nodes[strlen(Nodes)-1] != '\n')			// no terminating crlf?
			strcat(Nodes, "\r\n");

		ptr1 = Nodes;

		OtherNodes = realloc(OtherNodes, strlen(Nodes) + 2);

		ptr2 = OtherNodes;
		
		while (*ptr1)
		{
			if (*ptr1 == '\r')
			{
				*++ptr1 = 0;
			}
			*ptr2++=*ptr1++;
		}
		*ptr2++ = 0;

		break;
	}

	// Write to Registry

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	if (retCode == ERROR_SUCCESS)
	{

/*
		retCode += RegQueryValueEx(hKey,"Streams",0,			
			(ULONG *)&Type,(UCHAR *)&MaxStreams,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode += RegQueryValueEx(hKey,"ChatApplNum",0,			
			(ULONG *)&Type,(UCHAR *)&ChatApplNum,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"BBSApplNum",0,			
			(ULONG *)&Type,(UCHAR *)&BBSApplNum,(ULONG *)&Vallen);
		
		Vallen=100;
		retCode += RegQueryValueEx(hKey,"BBSName",0,			
			(ULONG *)&Type,(UCHAR *)&BBSName,(ULONG *)&Vallen);
				
		Vallen=100;
		retCode += RegQueryValueEx(hKey,"H-Route",0,			
			(ULONG *)&Type,(UCHAR *)&HRoute,(ULONG *)&Vallen);

		Vallen=MAX_PATH;
		retCode += RegQueryValueEx(hKey,"BaseDir",0,			
			(ULONG *)&Type,(UCHAR *)&BaseDir,(ULONG *)&Vallen);
*/

		retCode += RegSetValueEx(hKey,"OtherChatNodes", 0, REG_MULTI_SZ, OtherNodes, ptr2-&OtherNodes[0]);		


		retCode = RegSetValueEx(hKey,"SMTPPort",0,REG_DWORD,(BYTE *)&SMTPInPort,4);
		retCode = RegSetValueEx(hKey,"POP3Port",0,REG_DWORD,(BYTE *)&POP3InPort,4);

		retCode = RegSetValueEx(hKey,"SMTPGatewayEnabled",0,REG_DWORD,(BYTE *)&ISP_Gateway_Enabled,4);
		retCode = RegSetValueEx(hKey,"ISPSMTPPort",0,REG_DWORD,(BYTE *)&ISPSMTPPort,4);
		retCode = RegSetValueEx(hKey,"ISPPOP3Port",0,REG_DWORD,(BYTE *)&ISPPOP3Port,4);

		retCode = RegSetValueEx(hKey,"SMTPGatewayEnabled",0,REG_DWORD,(BYTE *)&ISP_Gateway_Enabled,4);
		retCode = RegSetValueEx(hKey,"POP3 Polling Interval",0,REG_DWORD,(BYTE *)&ISPPOP3Interval,4);

		retCode = RegSetValueEx(hKey,"MyDomain",0,REG_SZ,(BYTE *)&MyDomain, strlen(MyDomain));
		retCode = RegSetValueEx(hKey,"ISPSMTPName",0,REG_SZ,(BYTE *)&ISPSMTPName, strlen(ISPSMTPName));
		retCode = RegSetValueEx(hKey,"ISPPOP3Name",0,REG_SZ,(BYTE *)&ISPPOP3Name, strlen(ISPPOP3Name));
		retCode = RegSetValueEx(hKey,"ISPAccountName",0,REG_SZ,(BYTE *)&ISPAccountName, strlen(ISPAccountName));
		retCode = RegSetValueEx(hKey,"ISPAccountPass",0,REG_BINARY,(BYTE *)&EncryptedISPAccountPass, EncryptedPassLen);
	}

	return TRUE;
}
BOOL GetConfigFromRegistry()
{
	HKEY hKey=0;
	int retCode,Type,Vallen;

	// Get Config From Registry

	retCode = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);

	if (retCode == ERROR_SUCCESS)
	{
		Vallen=4;
		retCode += RegQueryValueEx(hKey,"Streams",0,			
			(ULONG *)&Type,(UCHAR *)&MaxStreams,(ULONG *)&Vallen);
		
		Vallen=4;
		retCode += RegQueryValueEx(hKey,"ChatApplNum",0,			
			(ULONG *)&Type,(UCHAR *)&ChatApplNum,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"BBSApplNum",0,			
			(ULONG *)&Type,(UCHAR *)&BBSApplNum,(ULONG *)&Vallen);
		
		Vallen=100;
		retCode += RegQueryValueEx(hKey,"BBSName",0,			
			(ULONG *)&Type,(UCHAR *)&BBSName,(ULONG *)&Vallen);
				
		Vallen=100;
		retCode += RegQueryValueEx(hKey,"H-Route",0,			
			(ULONG *)&Type,(UCHAR *)&HRoute,(ULONG *)&Vallen);

		Vallen=MAX_PATH;
		retCode += RegQueryValueEx(hKey,"BaseDir",0,			
			(ULONG *)&Type,(UCHAR *)&BaseDir,(ULONG *)&Vallen);

		// Get lentgh of Chatnodes String
				
		Vallen=0;
		retCode = RegQueryValueEx(hKey,"OtherChatNodes",0,			
			(ULONG *)&Type,NULL,(ULONG *)&Vallen);

		OtherNodes=malloc(Vallen);

		retCode += RegQueryValueEx(hKey,"OtherChatNodes",0,			
			(ULONG *)&Type,OtherNodes,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"SMTPPort",0,			
			(ULONG *)&Type,(UCHAR *)&SMTPInPort,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"POP3Port",0,			
			(ULONG *)&Type,(UCHAR *)&POP3InPort,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"SMTPGatewayEnabled",0,			
			(ULONG *)&Type,(UCHAR *)&ISP_Gateway_Enabled,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"POP3 Polling Interval",0,			
			(ULONG *)&Type,(UCHAR *)&ISPPOP3Interval,(ULONG *)&Vallen);

		Vallen=50;
		retCode += RegQueryValueEx(hKey,"MyDomain",0,			
			(ULONG *)&Type,(UCHAR *)&MyDomain,(ULONG *)&Vallen);

		Vallen=50;
		retCode += RegQueryValueEx(hKey,"ISPSMTPName",0,			
			(ULONG *)&Type,(UCHAR *)&ISPSMTPName,(ULONG *)&Vallen);

		Vallen=50;
		retCode += RegQueryValueEx(hKey,"ISPPOP3Name",0,			
			(ULONG *)&Type,(UCHAR *)&ISPPOP3Name,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"ISPSMTPPort",0,			
			(ULONG *)&Type,(UCHAR *)&ISPSMTPPort,(ULONG *)&Vallen);

		Vallen=4;
		retCode += RegQueryValueEx(hKey,"ISPPOP3Port",0,			
			(ULONG *)&Type,(UCHAR *)&ISPPOP3Port,(ULONG *)&Vallen);


		Vallen=50;
		retCode += RegQueryValueEx(hKey,"ISPAccountName",0,			
			(ULONG *)&Type,(UCHAR *)&ISPAccountName,(ULONG *)&Vallen);

		EncryptedPassLen=50;
		retCode += RegQueryValueEx(hKey,"ISPAccountPass",0,			
			(ULONG *)&Type,(UCHAR *)&EncryptedISPAccountPass,(ULONG *)&EncryptedPassLen);

		RegCloseKey(hKey);

		DecryptPass(EncryptedISPAccountPass, ISPAccountPass, EncryptedPassLen);

		if (retCode)
		{
				MessageBox(NULL, "Some Config Params Missing - Run \"BPQMailChat CONFIGURE\" to fix configuration",
						"BPQMailChat", MB_ICONSTOP);

				return FALSE;
		}

		return TRUE;
	}
	
	MessageBox(NULL, "Registry Key HKEY_LOCAL_MACHINE\\SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat could not be opened\r\n\r\n\
Run \"BPQMailChat CONFIGURE\" to create configuration",
						"BPQMailChat", MB_ICONSTOP);

	return FALSE;

}