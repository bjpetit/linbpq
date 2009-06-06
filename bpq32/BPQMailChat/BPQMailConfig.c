		
// Mail and Chat Server for BPQ32 Packet Switch
//
//	Configuration Module

#include "stdafx.h"
#define C_PAGES 5

int CurrentPage=0;				// Page currently on show in tabbed Dialog

#define BBSPARAMS 0
#define ISPPARAMS 1
#define CHATPARAMS 2
#define USERPARAMS 3
#define FWDPARAMS 4

extern HINSTANCE hInst;
extern HWND hWnd;
extern char BBSName[];
extern char HRoute[];
extern char BBSApplNum;
extern int ChatApplNum;
extern char BaseDir[];
extern int SMTPInPort;
extern int POP3InPort;
extern int MaxStreams;
extern UCHAR * OtherNodes;
extern struct UserInfo * BBSChain;		// Chain of users that are BBSes
extern struct UserInfo ** UserRecPtr;
extern int NumberofUsers;


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
HWND hwndDisplay;   // current child dialog box

char CurrentConfigCall[20];

char InfoBoxText[100];			// Text to display in Config Info Popup

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

INT_PTR CALLBACK InfoDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command;
		
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:

		SetDlgItemText(hDlg, 5050, InfoBoxText);

		return (INT_PTR)TRUE;

	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{
		case 0:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		}
		break;
	}
	
	return (INT_PTR)FALSE;
}



INT_PTR CALLBACK ChildDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	This processes messages from controls on the tab subpages
	int Command;

		
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
		return (INT_PTR)TRUE;

	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{
		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case IDC_BBS:

			// BBS Selection Changed

			Do_BBS_Sel_Changed(hDlg);

			return TRUE;

		case IDC_USER:

			// BBS Selection Changed

			Do_User_Sel_Changed(hDlg);

			return TRUE;

		case IDC_ADDUSER:

			Do_Add_User();

			return TRUE;

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

	// Add a tab for each of the child dialog boxes.

	tie.mask = TCIF_TEXT | TCIF_IMAGE;

	tie.iImage = -1;

	tie.pszText = "BBS Params";
	TabCtrl_InsertItem(pHdr->hwndTab, 0, &tie);

	tie.pszText = "ISP Interface";
	TabCtrl_InsertItem(pHdr->hwndTab, 1, &tie);

	tie.pszText = "Chat Params";
	TabCtrl_InsertItem(pHdr->hwndTab, 2, &tie);

	tie.pszText = "Users";
	TabCtrl_InsertItem(pHdr->hwndTab, 3, &tie);

	tie.pszText = "Forwarding Params";
	TabCtrl_InsertItem(pHdr->hwndTab, 4, &tie);

	// Lock the resources for the three child dialog boxes.

	pHdr->apRes[0] = DoLockDlgRes("BBS_CONFIG");
	pHdr->apRes[1] = DoLockDlgRes("ISP_CONFIG");
	pHdr->apRes[2] = DoLockDlgRes("CHAT_CONFIG");
	pHdr->apRes[3] = DoLockDlgRes("USEREDIT");
	pHdr->apRes[4] = DoLockDlgRes("FORWARDING");

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
	int n, ptr;
	int len;
	char Nodes[1000]="";
	struct UserInfo * user;


	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);

	CurrentPage = TabCtrl_GetCurSel(pHdr->hwndTab);

	// Destroy the current child dialog box, if any.

	if (pHdr->hwndDisplay != NULL)

		DestroyWindow(pHdr->hwndDisplay);

	// Create the new child dialog box.

	pHdr->hwndDisplay = CreateDialogIndirect(hInst, pHdr->apRes[CurrentPage], hwndDlg, ChildDialogProc);

	hwndDisplay = pHdr->hwndDisplay;		// Save

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

		SetDlgItemText(pHdr->hwndDisplay, IDC_MyMailDomain, MyDomain);

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

		if (OtherNodes)
		{
			while (OtherNodes[ptr])
			{
				len=strlen(&OtherNodes[ptr]);
				strcat(Nodes, &OtherNodes[ptr]);
				strcat(Nodes, "\r\n");
				ptr+= (len + 1);
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDC_ChatNodes, Nodes);

		break;


	case USERPARAMS:
		
		for (n = 1; n <= NumberofUsers; n++)
		{
			SendDlgItemMessage(pHdr->hwndDisplay, IDC_USER, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)UserRecPtr[n]->Call);
		}  

	case FWDPARAMS:

		for (user = BBSChain; user; user = user->BBSNext)
		{
			SendDlgItemMessage(pHdr->hwndDisplay, IDC_BBS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)user->Call);
		}  


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

int Do_BBS_Sel_Changed(HWND hDlg)
{
	// Update BBS display with newly selected BBS

	struct UserInfo * user;

	int Sel = SendDlgItemMessage(hDlg, IDC_BBS, CB_GETCURSEL, 0, 0);

	SendDlgItemMessage(hDlg, IDC_BBS, CB_GETLBTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentConfigCall);

	for (user = BBSChain; user; user = user->BBSNext)
	{
		if (strcmp(user->Call, CurrentConfigCall) == 0)
		{
			struct	BBSForwardingInfo * ForwardingInfo = user->ForwardingInfo;
			char ** Calls;
			char Text[10000]="";

			Calls = ForwardingInfo->TOCalls;

			if (Calls)
			{
				while(Calls[0])
				{
					strcat(Text, Calls[0]);
					strcat(Text, "\r\n");
					Calls++;
				}
			}
			SetDlgItemText(hDlg, IDC_TOCALLS, Text);

			Text[0] = 0;

			Calls = ForwardingInfo->ATCalls;

			if (Calls)
			{
				while(Calls[0])
				{
					strcat(Text, Calls[0]);
					strcat(Text, "\r\n");
					Calls++;
				}
			}

			SetDlgItemText(hDlg, IDC_ATCALLS, Text);

			Text[0] = 0;
			Calls = ForwardingInfo->Haddresses;

			if (Calls)
			{
				while(Calls[0])
				{
					strcat(Text, Calls[0]);
					strcat(Text, "\r\n");
					Calls++;
				}
			
			}

			SetDlgItemText(hDlg, IDC_HROUTES, Text);


			Text[0] = 0;
			Calls = ForwardingInfo->ConnectScript;

			if (Calls)
			{
				while(Calls[0])
				{
					strcat(Text, Calls[0]);
					strcat(Text, "\r\n");
					Calls++;
				}
			
			}

			SetDlgItemText(hDlg, IDC_CALL, Text);

			CheckDlgButton(hDlg, IDC_FWDENABLE, ForwardingInfo->Enabled);
			CheckDlgButton(hDlg, IDC_REVERSE, ForwardingInfo->ReverseFlag);

			return 0;
		}

	}


	return 0;

}

int Do_User_Sel_Changed(HWND hDlg)
{
	int n;
	
	// Update BBS display with newly selected BBS

	struct UserInfo * user;

	int Sel = SendDlgItemMessage(hDlg, IDC_USER, CB_GETCURSEL, 0, 0);

	if (Sel == -1)
		SendDlgItemMessage(hDlg, IDC_USER, WM_GETTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentConfigCall);
	else
		SendDlgItemMessage(hDlg, IDC_USER, CB_GETLBTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentConfigCall);

	

	for (n = 1; n <= NumberofUsers; n++)
	{
		user = UserRecPtr[n];

		if (strcmp(user->Call, CurrentConfigCall) == 0)
		{
			SetDlgItemText(hDlg, IDC_NAME, user->Name);
			SetDlgItemText(hDlg, IDC_PASSWORD, user->pass);
			SetDlgItemText(hDlg, IDC_ZIP, user->Address);
			SetDlgItemText(hDlg, IDC_HOMEBBS, user->HomeBBS);

			CheckDlgButton(hDlg, IDC_SYSOP, (user->flags & F_SYS));
			CheckDlgButton(hDlg, IDC_BBSFLAG, (user->flags & F_BBS));
			CheckDlgButton(hDlg, IDC_PMSFLAG, (user->flags & F_PMS));
			CheckDlgButton(hDlg, IDC_EXPERT, (user->flags & F_EXP));
			CheckDlgButton(hDlg, IDC_EXCLUDED, (user->flags & F_EXC));

			return 0;
		}
	}

	// Typing in new user

	SetDlgItemText(hDlg, IDC_NAME, "");
	SetDlgItemText(hDlg, IDC_PASSWORD, "");
	SetDlgItemText(hDlg, IDC_ZIP, "");
	SetDlgItemText(hDlg, IDC_HOMEBBS, "");

	CheckDlgButton(hDlg, IDC_SYSOP, FALSE);
	CheckDlgButton(hDlg, IDC_BBSFLAG, FALSE);
	CheckDlgButton(hDlg, IDC_PMSFLAG, FALSE);
	CheckDlgButton(hDlg, IDC_EXPERT, FALSE);
	CheckDlgButton(hDlg, IDC_EXCLUDED, FALSE);

	return 0;
}

VOID Do_Add_User()
{
	struct UserInfo * user;
	
	if (LookupCall(CurrentConfigCall))
		wsprintf(InfoBoxText, "User %s already exists", CurrentConfigCall);
	else if	((strlen(CurrentConfigCall) < 3) || (strlen(CurrentConfigCall) > 6))
		wsprintf(InfoBoxText, "User %s is invalid", CurrentConfigCall);
	else

	{
		user = AllocateUserRecord(CurrentConfigCall);
		wsprintf(InfoBoxText, "User %s added", CurrentConfigCall);
		SendDlgItemMessage(hwndDisplay, IDC_USER, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)user->Call);

	}	
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
	return;

}

BOOL SaveConfig()
{
	BOOL OK1,OK2,OK3,OK4;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);
	HKEY hKey=0;
	int retCode,disp;

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

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
		retCode = RegSetValueEx(hKey, "ChatApplNum",0 , REG_DWORD,(BYTE *)&ChatApplNum, 4);

		MultiLineDialogToREG_MULTI_SZ(pHdr->hwndDisplay, IDC_ChatNodes, hKey, "OtherChatNodes");

		break;
	

	case FWDPARAMS:

		{
			char Key[100] =  "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\BBSForwarding\\";
			HKEY hKey=0;
			int Rev;

			strcat(Key, CurrentConfigCall);

			retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

			MultiLineDialogToREG_MULTI_SZ(pHdr->hwndDisplay, IDC_ATCALLS, hKey, "ATCalls");
			MultiLineDialogToREG_MULTI_SZ(pHdr->hwndDisplay, IDC_TOCALLS, hKey, "ToCalls");
			MultiLineDialogToREG_MULTI_SZ(pHdr->hwndDisplay, IDC_HROUTES, hKey, "HRoutes");
			MultiLineDialogToREG_MULTI_SZ(pHdr->hwndDisplay, IDC_CALL, hKey, "Connect Script");

			Rev = IsDlgButtonChecked(pHdr->hwndDisplay, IDC_FWDENABLE);
			retCode = RegSetValueEx(hKey,"Enabled", 0, REG_DWORD, (BYTE *)&Rev,4);

			Rev = IsDlgButtonChecked(pHdr->hwndDisplay, IDC_REVERSE);
			retCode = RegSetValueEx(hKey,"RequestReverse", 0, REG_DWORD, (BYTE *)&Rev,4);

			RegCloseKey(hKey);


		}

		return TRUE;

		
	case USERPARAMS:
		{
			int n;
			struct UserInfo * user;

			for (n = 1; n <= NumberofUsers; n++)
			{
				user = UserRecPtr[n];

				if (strcmp(user->Call, CurrentConfigCall) == 0)
				{
					GetDlgItemText(pHdr->hwndDisplay, IDC_NAME, user->Name, 17);
					GetDlgItemText(pHdr->hwndDisplay, IDC_PASSWORD, user->pass, 12);
					GetDlgItemText(pHdr->hwndDisplay, IDC_ZIP, user->Address, 60);
					GetDlgItemText(pHdr->hwndDisplay, IDC_HOMEBBS, user->HomeBBS, 40);

					if (IsDlgButtonChecked(pHdr->hwndDisplay, IDC_BBSFLAG))
						user->flags |= F_BBS; else user->flags &= ~F_BBS;

					if (IsDlgButtonChecked(pHdr->hwndDisplay, IDC_PMSFLAG))
						user->flags |= F_PMS; else user->flags &= ~F_PMS;

					if (IsDlgButtonChecked(pHdr->hwndDisplay, IDC_EXPERT))
						user->flags |= F_EXP; else user->flags &= ~F_EXP;

					if (IsDlgButtonChecked(pHdr->hwndDisplay, IDC_EXCLUDED))
						user->flags |= F_EXC; else user->flags &= ~F_EXC;

					if (IsDlgButtonChecked(pHdr->hwndDisplay, IDC_SYSOP))
						user->flags |= F_SYS; else user->flags &= ~F_SYS;
					
					return 0;
				}
			}
		}

	} // End of switch


	// Write to Registry

	if (retCode == ERROR_SUCCESS)
	{		
		retCode = RegSetValueEx(hKey, "Streams", 0, REG_DWORD,(BYTE *)&MaxStreams, 4);
		retCode = RegSetValueEx(hKey, "BBSApplNum", 0, REG_DWORD,(BYTE *)&BBSApplNum, 4);
		retCode = RegSetValueEx(hKey, "BBSName", 0, REG_SZ,(BYTE *)&BBSName, strlen(BBSName));
		retCode = RegSetValueEx(hKey, "H-Route", 0, REG_SZ,(BYTE *)&HRoute, strlen(HRoute));
		retCode = RegSetValueEx(hKey, "BaseDir", 0, REG_SZ,(BYTE *)&BaseDir, strlen(BaseDir));
				
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

		RegCloseKey(hKey);

	}

	return TRUE;
}

MultiLineDialogToREG_MULTI_SZ(HWND hDialog, int DLGItem, HKEY hKey, char * ValueName)
{
	char Text[10000];
	char Value[10000];
	int retCode;
	char * ptr1, * ptr2;


	GetDlgItemText(hDialog, DLGItem, Text, 10000);

	// replace crlf with single null

	if (Text[strlen(Text)-1] != '\n')			// no terminating crlf?
		strcat(Text, "\r\n");

	ptr1 = Text;
	ptr2 = Value;
		
	while (*ptr1)
	{
		if (*ptr1 == '\r')
		{
			while (*(ptr1+2) == '\r')			// Blank line
				ptr1+=2;

			*++ptr1 = 0;
		}
		*ptr2++=*ptr1++;
	}

	*ptr2++ = 0;

	// Write to Registry

	retCode = RegSetValueEx(hKey, ValueName, 0, REG_MULTI_SZ, Value, ptr2-&Value[0]);

	return TRUE;

}
		



BOOL GetConfigFromRegistry()
{
	HKEY hKey=0;
	int retCode,Type,Vallen;

	// Get Config From Registry

TryAgain:

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

		if (Vallen)
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
				MessageBox(NULL, "Some Config Params Missing - Opening Configuration Dialog",
						"BPQMailChat", MB_ICONINFORMATION);

				DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hWnd, ConfigWndProc);
				goto TryAgain;

		}

		return TRUE;
	}
	
	MessageBox(NULL, "Registry Key HKEY_LOCAL_MACHINE\\SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat could not be opened - Opening Configuration Dialog",
						"BPQMailChat", MB_ICONINFORMATION);

	DialogBox(hInst, MAKEINTRESOURCE(IDD_CONFIG), hWnd, ConfigWndProc);
	goto TryAgain;

	return TRUE;

}