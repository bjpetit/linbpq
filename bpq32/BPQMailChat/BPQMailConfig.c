		
// Mail and Chat Server for BPQ32 Packet Switch
//
//	Configuration Module

#include "stdafx.h"
#define C_PAGES 4

int CurrentPage=0;				// Page currently on show in tabbed Dialog

#define BBSPARAMS 0
#define ISPPARAMS 1
#define CHATPARAMS 2
#define MAINTPARAMS 3


typedef struct tag_dlghdr {

HWND hwndTab; // tab control
HWND hwndDisplay; // current child dialog box
RECT rcDisplay; // display rectangle for the tab control

DLGTEMPLATE *apRes[C_PAGES];

} DLGHDR;

HWND hwndDlg;		// Config Dialog
HWND hwndDisplay;   // Current child dialog box

char CurrentConfigCall[20];		// Current user or bbs
int CurrentConfigIndex;			// Index of current user record
int CurrentMsgIndex;			// Index of current Msg record
struct UserInfo * CurrentBBS;	// User Record of selected BBS iin Forwarding Config;

char InfoBoxText[100];			// Text to display in Config Info Popup

DLGTEMPLATE * WINAPI DoLockDlgRes(LPCSTR lpszResName);
VOID WINAPI OnSelChanged(HWND hwndDlg);
VOID WINAPI OnChildDialogInit(HWND hwndDlg);
VOID WINAPI OnTabbedDialogInit(HWND hwndDlg);

// POP3 Password is encrypted by xor'ing it with an MD5 hash of the hostname and pop3 server name


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

	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		switch (LOWORD(wParam))
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
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

	switch (message)
	{
	case WM_INITDIALOG:
		OnChildDialogInit( hDlg);
		return (INT_PTR)TRUE;

	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		Command = LOWORD(wParam);

		if (Command == 2002)
			return TRUE;

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case IDC_BBSSAVE:
			
			SaveBBSConfig();
			return TRUE;

		case IDC_CHATSAVE:
			
			SaveChatConfig();
			return TRUE;

		case IDC_ISPSAVE:
			
			SaveISPConfig();
			return TRUE;


		case IDM_MAINTSAVE:

			SaveMAINTConfig();
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

	tie.pszText = "Housekeeping";
	TabCtrl_InsertItem(pHdr->hwndTab, 3, &tie);

	// Lock the resources for the three child dialog boxes.

	pHdr->apRes[0] = DoLockDlgRes("BBS_CONFIG");
	pHdr->apRes[1] = DoLockDlgRes("ISP_CONFIG");
	pHdr->apRes[2] = DoLockDlgRes("CHAT_CONFIG");
	pHdr->apRes[3] = DoLockDlgRes("MAINT");

	// Determine the bounding rectangle for all child dialog boxes.

	SetRectEmpty(&rcTab);

	for (i = 0; i < C_PAGES-1; i++)
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

	
	// Size the dialog box.

	SetWindowPos(hwndDlg, NULL, 0, 0, rcTab.right + cyMargin + 2 * GetSystemMetrics(SM_CXDLGFRAME),
		rcTab.bottom  + 2 * cyMargin + 2 * GetSystemMetrics(SM_CYDLGFRAME) + GetSystemMetrics(SM_CYCAPTION),
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
	char Text[10000]="";
	char Line[80];
	struct Override ** Call;
	char Time[10];

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
		SetDlgItemText(pHdr->hwndDisplay, IDC_SYSOPCALL, SYSOPCall);
		SetDlgItemText(pHdr->hwndDisplay, IDC_HRoute, HRoute);
		SetDlgItemText(pHdr->hwndDisplay, IDC_BaseDir, BaseDir);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_BBSAppl, BBSApplNum, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_BBSStreams, MaxStreams, FALSE);
		CheckDlgButton(pHdr->hwndDisplay, IDC_ENABLEUI, EnableUI);
		SetDlgItemText(pHdr->hwndDisplay, IDC_UIPORTS, UIPortString);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_POP3Port, POP3InPort, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_SMTPPort, SMTPInPort, FALSE);
		CheckDlgButton(pHdr->hwndDisplay, IDC_REMOTEEMAIL, RemoteEmail);

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


	case MAINTPARAMS:

		SetDlgItemInt(pHdr->hwndDisplay, IDC_MAXMSG, MaxMsgno, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_BIDLIFETIME, BidLifetime, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDC_MAINTINTERVAL, MaintInterval, FALSE);
		wsprintf(Time, "%04d", MaintTime);
		SetDlgItemText(pHdr->hwndDisplay, IDC_MAINTTIME, Time);

		SetDlgItemInt(pHdr->hwndDisplay, IDM_PR, PR, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDM_PUR, PUR, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDM_PF, PF, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDM_PNF, PNF, FALSE);

		SetDlgItemInt(pHdr->hwndDisplay, IDM_BF, BF, FALSE);
		SetDlgItemInt(pHdr->hwndDisplay, IDM_BNF, BNF, FALSE);

		CheckDlgButton(pHdr->hwndDisplay, IDM_AP, AP);
		CheckDlgButton(pHdr->hwndDisplay, IDM_AB, AB);

		if (LTFROM)
		{
			Call = LTFROM;
			while(Call[0])
			{
				wsprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDM_LTFROM, Text);

		Text[0] = 0;

		if (LTTO)
		{
			Call = LTTO;
			while(Call[0])
			{
				wsprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDM_LTTO, Text);

		Text[0] = 0;

		if (LTAT)
		{
			Call = LTAT;
			while(Call[0])
			{
				wsprintf(Line, "%s, %d\r\n", Call[0]->Call, Call[0]->Days);
				strcat(Text, Line);
				Call++;
			}
		}
		SetDlgItemText(pHdr->hwndDisplay, IDM_LTAT, Text);

		Text[0] = 0;

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

			CurrentBBS = user;					// Save User Pointer
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
			CheckDlgButton(hDlg, IDC_USEB2, ForwardingInfo->AllowB2);
			SetDlgItemInt(hDlg, IDC_FWDINT, ForwardingInfo->FwdInterval, FALSE);



			return 0;
		}

	}


	return 0;

}

int Do_User_Sel_Changed(HWND hDlg)
{
	
	// Update BBS display with newly selected BBS

	struct UserInfo * user;

	int Sel = SendDlgItemMessage(hDlg, IDC_USER, CB_GETCURSEL, 0, 0);

	if (Sel == -1)
		SendDlgItemMessage(hDlg, IDC_USER, WM_GETTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentConfigCall);
	else
		SendDlgItemMessage(hDlg, IDC_USER, CB_GETLBTEXT, Sel, (LPARAM)(LPCTSTR)&CurrentConfigCall);

	

	for (CurrentConfigIndex = 1; CurrentConfigIndex <= NumberofUsers; CurrentConfigIndex++)
	{
		user = UserRecPtr[CurrentConfigIndex];

		if (strcmp(user->Call, CurrentConfigCall) == 0)
		{
			SetDlgItemText(hDlg, IDC_NAME, user->Name);
			SetDlgItemText(hDlg, IDC_PASSWORD, user->pass);
			SetDlgItemText(hDlg, IDC_ZIP, user->Address);
			SetDlgItemText(hDlg, IDC_HOMEBBS, user->HomeBBS);

			CheckDlgButton(hDlg, IDC_SYSOP, (user->flags & F_SYSOP));
			CheckDlgButton(hDlg, IDC_BBSFLAG, (user->flags & F_BBS));
			CheckDlgButton(hDlg, IDC_PMSFLAG, (user->flags & F_PMS));
			CheckDlgButton(hDlg, IDC_EXPERT, (user->flags & F_Expert));
			CheckDlgButton(hDlg, IDC_EXCLUDED, (user->flags & F_Excluded));
			CheckDlgButton(hDlg, IDC_EMAIL, (user->flags & F_EMAIL));

			return 0;
		}
	}

	// Typing in new user

	CurrentConfigIndex = -1;

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

VOID Do_Add_User(HWND hDlg)
{
	struct UserInfo * user;
	int n;
	
	if (LookupCall(CurrentConfigCall))
		wsprintf(InfoBoxText, "User %s already exists", CurrentConfigCall);
	else if	((strlen(CurrentConfigCall) < 3) || (strlen(CurrentConfigCall) > 6))
		wsprintf(InfoBoxText, "User %s is invalid", CurrentConfigCall);
	else
	{
		user = AllocateUserRecord(CurrentConfigCall);
		CurrentConfigIndex=NumberofUsers;
		Do_Save_User(hDlg, FALSE);
		wsprintf(InfoBoxText, "User %s added and info saved", CurrentConfigCall);
	}	
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	SendDlgItemMessage(hDlg, IDC_USER, CB_RESETCONTENT, 0, 0);

	for (n = 1; n <= NumberofUsers; n++)
	{
		SendDlgItemMessage(hDlg, IDC_USER, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)UserRecPtr[n]->Call);
	} 

	return;

}

VOID SetupNewBBS(struct UserInfo * user)
{
	user->BBSNext = BBSChain;
	BBSChain = user;
	user->BBSNumber = ++HighestBBSNumber;

	ReinitializeFWDStruct(user);

}



VOID Do_Delete_User(HWND hDlg)
{
	struct UserInfo * user;
	int n;


	if (CurrentConfigIndex == -1)
	{
		wsprintf(InfoBoxText, "Please select a user to delete");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	user = UserRecPtr[CurrentConfigIndex];

	if (strcmp(CurrentConfigCall, user->Call) != 0)
	{
		wsprintf(InfoBoxText, "Inconsistancy detected - user not deleted");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	for (n = CurrentConfigIndex; n < NumberofUsers; n++)
	{
		UserRecPtr[n] = UserRecPtr[n+1];		// move down all following entries
	}
	
	NumberofUsers--;

	SendDlgItemMessage(hDlg, IDC_USER, CB_RESETCONTENT, 0, 0);

	for (n = 1; n <= NumberofUsers; n++)
	{
		SendDlgItemMessage(hDlg, IDC_USER, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)UserRecPtr[n]->Call);
	} 

	wsprintf(InfoBoxText, "User %s deleted", user->Call);
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	if (user->flags & F_BBS)	// was a BBS?
		DeleteBBS(user);
	
	free(user);

	return;

}
VOID Do_Save_User(HWND hDlg, BOOL ShowBox)
{
	struct UserInfo * user;

	if (CurrentConfigIndex == -1)
	{
		wsprintf(InfoBoxText, "Please select a user to save");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	user = UserRecPtr[CurrentConfigIndex];

	if (strcmp(CurrentConfigCall, user->Call) != 0)
	{
		wsprintf(InfoBoxText, "Inconsistancy detected - information not saved");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	GetDlgItemText(hDlg, IDC_NAME, user->Name, 17);
	GetDlgItemText(hDlg, IDC_PASSWORD, user->pass, 12);
	GetDlgItemText(hDlg, IDC_ZIP, user->Address, 60);
	GetDlgItemText(hDlg, IDC_HOMEBBS, user->HomeBBS, 40);

	if (IsDlgButtonChecked(hDlg, IDC_BBSFLAG))
	{
		// If BBS Flag has changed, must set up or delete forwarding info

		if ((user->flags & F_BBS) == 0)
		{
			// New BBS

			user->flags |= F_BBS;
			SetupNewBBS(user);
		}
	}
	else
	{
		if (user->flags & F_BBS)
		{
			//was a BBS

			user->flags &= ~F_BBS;
			DeleteBBS(user);
		}
	}
			
	if (IsDlgButtonChecked(hDlg, IDC_PMSFLAG))
		user->flags |= F_PMS; else user->flags &= ~F_PMS;

	if (IsDlgButtonChecked(hDlg, IDC_EXPERT))
		user->flags |= F_Expert; else user->flags &= ~F_Expert;

	if (IsDlgButtonChecked(hDlg, IDC_EXCLUDED))
		user->flags |= F_Excluded; else user->flags &= ~F_Excluded;

	if (IsDlgButtonChecked(hDlg, IDC_SYSOP))
		user->flags |= F_SYSOP; else user->flags &= ~F_SYSOP;

	if (IsDlgButtonChecked(hDlg, IDC_EMAIL))
		user->flags |= F_EMAIL; else user->flags &= ~F_EMAIL;

	SaveUserDatabase();

	UpdateWPWithUserInfo(user);


	if (ShowBox)
	{
		wsprintf(InfoBoxText, "User information saved");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
	}				
}

VOID DeleteBBS(struct UserInfo * user)
{
	struct UserInfo * BBSRec, * PrevBBS = NULL;

	RemoveMenu(hFWDMenu, IDM_FORWARD_ALL + user->BBSNumber, MF_BYCOMMAND); 

	for (BBSRec = BBSChain; BBSRec; PrevBBS = BBSRec, BBSRec = BBSRec->BBSNext)
	{
		if (user == BBSRec)
		{
			if (PrevBBS == NULL)		// First in chain;
			{
				BBSChain = BBSRec->BBSNext;
				break;
			}
			PrevBBS->BBSNext = BBSRec->BBSNext;
			break;
		}
	}
}

int Do_Msg_Sel_Changed(HWND hDlg)
{
	
	// Update Msg display with newly selected Msg

	struct MsgInfo * Msg;
	char MsgnoText[10];
	int Msgno, BBSNo;
	struct UserInfo * user;
	int Sel = SendDlgItemMessage(hDlg, 0, LB_GETCURSEL, 0, 0);
	char Size[10];

	if (Sel != -1)
	{
		SendDlgItemMessage(hDlg, 0, LB_GETTEXT, Sel, (LPARAM)(LPCTSTR)&MsgnoText);
		Msgno = atoi(MsgnoText);
	}
	

	for (CurrentMsgIndex = 1; CurrentMsgIndex <= NumberofMessages; CurrentMsgIndex++)
	{
		Msg = MsgHddrPtr[CurrentMsgIndex];

		if (Msg->number == Msgno)
		{
			SetDlgItemText(hDlg, 6001, Msg->from);
			SetDlgItemText(hDlg, 6002, Msg->bid);
			SetDlgItemText(hDlg, 6003, Msg->to);
			SetDlgItemText(hDlg, 6004, Msg->via);
			SetDlgItemText(hDlg, 6005, Msg->title);
			wsprintf(Size, "%d", Msg->length);

			SetDlgItemText(hDlg, 6018, FormatDateAndTime(Msg->datecreated, FALSE));
			SetDlgItemText(hDlg, 6019, FormatDateAndTime(Msg->datereceived, FALSE));
			SetDlgItemText(hDlg, 6021, FormatDateAndTime(Msg->datechanged, FALSE));
			SetDlgItemText(hDlg, 6020, Size);

			SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_SETCURSEL, Msg->type  == 'B' ? 0 : 1, 0);

			switch(Msg->status)
			{
			case 'N': Sel = MSGSTATUS_N; break;
			case 'Y': Sel = MSGSTATUS_Y; break;
			case 'F': Sel = MSGSTATUS_F; break;
			case 'K': Sel = MSGSTATUS_K; break;
			case 'H': Sel = MSGSTATUS_H; break;
			case '$': Sel = MSGSTATUS_$; break;
			}

			SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_SETCURSEL, Sel, 0);

			for (BBSNo = 1; BBSNo <= NBBBS; BBSNo++ )
			{
				CheckDlgButton(hDlg, BBSNo + 24, BST_INDETERMINATE);
			}

			// Look for forward bits
			
			if (memcmp(Msg->fbbs, zeros, NBMASK) != 0)
			{	
				for (user = BBSChain; user; user = user->BBSNext)
				{
					if (check_fwd_bit(Msg->fbbs, user->BBSNumber))
					{
						SetDlgItemText(hDlg, user->BBSNumber+ 24, user->Call);
						CheckDlgButton(hDlg, user->BBSNumber+ 24, BST_UNCHECKED);
					}
				} 
			}

			if (memcmp(Msg->forw, zeros, NBMASK) != 0)
			{	
				for (user = BBSChain; user; user = user->BBSNext)
				{
					if (check_fwd_bit(Msg->forw, user->BBSNumber))
					{
						SetDlgItemText(hDlg, user->BBSNumber+ 24, user->Call);
						CheckDlgButton(hDlg, user->BBSNumber+ 24, BST_CHECKED);
					}
				}
			}

			return 0;
		}
	}

	CurrentMsgIndex = -1;

	return 0;
}

VOID Do_Save_Msg(HWND hDlg)
{
	struct MsgInfo * Msg;
	struct UserInfo * user;

	char status[2];
	int n, BBSNumber;
	BOOL toforward, forwarded;

	if (CurrentMsgIndex == -1)
	{
		wsprintf(InfoBoxText, "Please select a message to save");
		DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);
		return;
	}

	Msg = MsgHddrPtr[CurrentMsgIndex];

	GetDlgItemText(hDlg, 6001, Msg->from, 7);
	GetDlgItemText(hDlg, 6002, Msg->bid, 13);
	GetDlgItemText(hDlg, 6003, Msg->to, 7);
	GetDlgItemText(hDlg, 6004, Msg->via, 41);
	GetDlgItemText(hDlg, 6005, Msg->title, 61);

	GetDlgItemText(hDlg, IDC_MSGTYPE, status, 2);
	Msg->type = status[0];

	for (user = BBSChain; user; user = user->BBSNext)
	{
		n = IsDlgButtonChecked(hDlg, user->BBSNumber + 24);

		BBSNumber = user->BBSNumber;

		toforward = check_fwd_bit(Msg->fbbs, BBSNumber);
		forwarded = check_fwd_bit(Msg->forw, BBSNumber);

		if (n == BST_INDETERMINATE)
		{
			if ((!toforward) && (!forwarded))
			{
				// No Change
				continue;
			}
			else
			{
				clear_fwd_bit(Msg->fbbs, BBSNumber);
				if (toforward)
					user->ForwardingInfo->MsgCount--;

				clear_fwd_bit(Msg->forw, BBSNumber);
			}
		}
		else if (n == BST_UNCHECKED)
		{
			if (toforward)
			{
				// No Change
				continue;
			}
			else
			{
				set_fwd_bit(Msg->fbbs, BBSNumber);
				user->ForwardingInfo->MsgCount++;
				clear_fwd_bit(Msg->forw, BBSNumber);
				if (FirstMessagetoForward > CurrentMsgIndex)
					FirstMessagetoForward = CurrentMsgIndex;
			}
		}
		else if (n == BST_CHECKED)
		{
			if (forwarded)
			{
				// No Change
				continue;
			}
			else
			{
				set_fwd_bit(Msg->forw, BBSNumber);

				clear_fwd_bit(Msg->fbbs, BBSNumber);
				if (toforward)
					user->ForwardingInfo->MsgCount--;

			}
		}
	}

	GetDlgItemText(hDlg, IDC_MSGSTATUS, status, 2);
	
	if (Msg->status != status[0])
	{
		// Need to take action if killing message

		Msg->status = status[0];
		if (status[0] == 'K')
			FlagAsKilled(Msg);					// Clear forwarding bits
	}

	wsprintf(InfoBoxText, "Message Updated");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	Msg->datechanged=time(NULL);

	SaveMessageDatabase();		

	Do_Msg_Sel_Changed(hDlg);				// Refresh
}

VOID SaveBBSConfig()
{
	BOOL OK1,OK2,OK3,OK4;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);
	HKEY hKey=0;
	int retCode,disp;

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

		
	GetDlgItemText(hwndDisplay, IDC_BBSCall, BBSName, 50);
	GetDlgItemText(hwndDisplay, IDC_SYSOPCALL, SYSOPCall, 50);
	GetDlgItemText(hwndDisplay, IDC_HRoute, HRoute, 50);
	GetDlgItemText(hwndDisplay, IDC_BaseDir, BaseDir, 50);
	EnableUI = IsDlgButtonChecked(hwndDisplay, IDC_ENABLEUI);
	GetDlgItemText(hwndDisplay, IDC_UIPORTS, UIPortString, 100);

	BBSApplNum = GetDlgItemInt(hwndDisplay, IDC_BBSAppl, &OK1, FALSE);
	MaxStreams = GetDlgItemInt(hwndDisplay, IDC_BBSStreams, &OK2, FALSE);
	POP3InPort = GetDlgItemInt(hwndDisplay, IDC_POP3Port, &OK3, FALSE);
	SMTPInPort = GetDlgItemInt(hwndDisplay, IDC_SMTPPort, &OK4, FALSE);

	RemoteEmail = IsDlgButtonChecked(hwndDisplay, IDC_REMOTEEMAIL);

	strlop(BBSName, '-');
	strlop(SYSOPCall, '-');

	if (retCode == ERROR_SUCCESS)
	{		
		retCode = RegSetValueEx(hKey, "Streams", 0, REG_DWORD,(BYTE *)&MaxStreams, 4);
		retCode = RegSetValueEx(hKey, "BBSApplNum", 0, REG_DWORD,(BYTE *)&BBSApplNum, 4);
		retCode = RegSetValueEx(hKey, "BBSName", 0, REG_SZ,(BYTE *)&BBSName, strlen(BBSName));
		retCode = RegSetValueEx(hKey, "SYSOPCall", 0, REG_SZ,(BYTE *)&SYSOPCall, strlen(SYSOPCall));
		retCode = RegSetValueEx(hKey, "H-Route", 0, REG_SZ,(BYTE *)&HRoute, strlen(HRoute));
		retCode = RegSetValueEx(hKey, "BaseDir", 0, REG_SZ,(BYTE *)&BaseDir, strlen(BaseDir));
		retCode = RegSetValueEx(hKey, "EnableUI",0, REG_DWORD,(BYTE *)&EnableUI,4);
		retCode = RegSetValueEx(hKey, "UIPortString",0, REG_SZ,(BYTE *)&UIPortString, strlen(UIPortString));
				
		retCode = RegSetValueEx(hKey, "SMTPPort",0,REG_DWORD,(BYTE *)&SMTPInPort,4);
		retCode = RegSetValueEx(hKey, "POP3Port",0,REG_DWORD,(BYTE *)&POP3InPort,4);
		retCode = RegSetValueEx(hKey, "RemoteEmail",0,REG_DWORD,(BYTE *)&RemoteEmail,4);

		RegCloseKey(hKey);

	}

	wsprintf(InfoBoxText, "Warning - Program must be restarted for changes to be effective");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}
	

VOID SaveISPConfig()
{
	BOOL OK1,OK2,OK3;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);
	HKEY hKey=0;
	int retCode,disp;

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

		
		ISP_Gateway_Enabled = IsDlgButtonChecked(hwndDisplay, IDC_ISP_Gateway_Enabled);

		ISPPOP3Interval = GetDlgItemInt(hwndDisplay, IDC_POP3Timer, &OK1, FALSE);

		GetDlgItemText(hwndDisplay, IDC_MyMailDomain, MyDomain, 50);

		GetDlgItemText(hwndDisplay, IDC_ISPSMTPName, ISPSMTPName, 50);
		ISPSMTPPort = GetDlgItemInt(hwndDisplay, IDC_ISPSMTPPort, &OK2, FALSE);

		GetDlgItemText(hwndDisplay, IDC_ISPPOP3Name, ISPPOP3Name, 50);
		ISPPOP3Port = GetDlgItemInt(hwndDisplay, IDC_ISPPOP3Port, &OK3, FALSE);

		GetDlgItemText(hwndDisplay, IDC_ISPAccountName, ISPAccountName, 50);
		GetDlgItemText(hwndDisplay, IDC_ISPAccountPass, ISPAccountPass, 50);

		EncryptedPassLen = EncryptPass(ISPAccountPass, EncryptedISPAccountPass);

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

	wsprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}


VOID SaveChatConfig()
{
	BOOL OK1;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);
	HKEY hKey=0;
	int retCode, disp, Type, Vallen, len;
	int ptr;
	int OldChatAppl;

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	OldChatAppl = ChatApplNum;
	
	ChatApplNum = GetDlgItemInt(hwndDisplay, IDC_ChatAppl, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "ChatApplNum",0 , REG_DWORD,(BYTE *)&ChatApplNum, 4);

	MultiLineDialogToREG_MULTI_SZ(hwndDisplay, IDC_ChatNodes, hKey, "OtherChatNodes");

	// reinitialise other nodes list. rtlink messes with the string so pass copy

	node_close();

	// Show dialog box now - gives time for links to close
	
	if (ChatApplNum == OldChatAppl)
		wsprintf(InfoBoxText, "Configuration Changes Saved and Applied");
	else
	{
		ChatApplNum = OldChatAppl;
		wsprintf(InfoBoxText, "Warning Program must be restarted to change Chat Appl Num");
	}
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

	free(OtherNodes);

	// Get length of Chatnodes String
				
	Vallen=0;
	RegQueryValueEx(hKey,"OtherChatNodes",0,			
		(ULONG *)&Type,NULL,(ULONG *)&Vallen);

	if (Vallen)
		OtherNodes=malloc(Vallen);

	RegQueryValueEx(hKey,"OtherChatNodes",0,			
		(ULONG *)&Type,OtherNodes,(ULONG *)&Vallen);

	RegCloseKey(hKey);

	removelinks();
	 
	ptr=0;

	while (OtherNodes[ptr])
	{
		len=strlen(&OtherNodes[ptr]);		
		rtlink(_strdup(&OtherNodes[ptr]));			
		ptr+= (len + 1);
	}

	if (user_hd)			// Any Users?
		makelinks();		// Bring up links
}
	

VOID SaveFWDConfig(HWND hDlg)
{
	HKEY hKey=0;
	int retCode,disp, OK, Val;
	char Key[100] =  "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\BBSForwarding\\";
	int Rev;

	if (CurrentBBS)
	{
		strcat(Key, CurrentConfigCall);

		retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, Key, 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

		MultiLineDialogToREG_MULTI_SZ(hDlg, IDC_ATCALLS, hKey, "ATCalls");
		MultiLineDialogToREG_MULTI_SZ(hDlg, IDC_TOCALLS, hKey, "ToCalls");
		MultiLineDialogToREG_MULTI_SZ(hDlg, IDC_HROUTES, hKey, "HRoutes");
		MultiLineDialogToREG_MULTI_SZ(hDlg, IDC_CALL, hKey, "Connect Script");

		Rev = IsDlgButtonChecked(hDlg, IDC_FWDENABLE);
		retCode = RegSetValueEx(hKey,"Enabled", 0, REG_DWORD, (BYTE *)&Rev,4);

		Rev = IsDlgButtonChecked(hDlg, IDC_REVERSE);
		retCode = RegSetValueEx(hKey,"RequestReverse", 0, REG_DWORD, (BYTE *)&Rev,4);
	
		Rev = IsDlgButtonChecked(hDlg, IDC_USEB2);
		retCode = RegSetValueEx(hKey,"Use B2 Protocol", 0, REG_DWORD, (BYTE *)&Rev,4);

		Val = GetDlgItemInt(hDlg, IDC_FWDINT, &OK, FALSE);
		retCode = RegSetValueEx(hKey,"FWDInterval", 0, REG_DWORD, (BYTE *)&Val,4);

		RegCloseKey(hKey);

		ReinitializeFWDStruct(CurrentBBS);
	}
		
	// Interval and Max Sizes are not user specific

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
			"SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	MaxTXSize = GetDlgItemInt(hDlg, IDC_MAXSEND, &OK, FALSE);
	retCode = RegSetValueEx(hKey,"MaxTXSize", 0, REG_DWORD, (BYTE *)&MaxTXSize,4);

	MaxRXSize = GetDlgItemInt(hDlg, IDC_MAXRECV, &OK, FALSE);
	retCode = RegSetValueEx(hKey,"MaxRXSize", 0, REG_DWORD, (BYTE *)&MaxRXSize,4);

	MaxFBBBlockSize = GetDlgItemInt(hDlg, IDC_MAXBLOCK, &OK, FALSE);
	retCode = RegSetValueEx(hKey,"MaxFBBBlock", 0, REG_DWORD, (BYTE *)&MaxFBBBlockSize,4);


	RegCloseKey(hKey);
		
	wsprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}

VOID SaveMAINTConfig()
{
	BOOL OK1;
	DLGHDR *pHdr = (DLGHDR *) GetWindowLong(hwndDlg, GWL_USERDATA);
	HKEY hKey=0;
	int retCode, disp, val;

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\Housekeeping", 0, 0, 0, KEY_ALL_ACCESS, NULL, &hKey, &disp);

	MaxMsgno = GetDlgItemInt(hwndDisplay, IDC_MAXMSG, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "MaxMsgno",0 , REG_DWORD,(BYTE *)&MaxMsgno, 4);

	BidLifetime = GetDlgItemInt(hwndDisplay, IDC_BIDLIFETIME, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "BidLifetime",0 , REG_DWORD,(BYTE *)&BidLifetime, 4);

	MaintInterval = GetDlgItemInt(hwndDisplay, IDC_MAINTINTERVAL, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "MaintInterval",0 , REG_DWORD,(BYTE *)&MaintInterval, 4);

	MaintTime = GetDlgItemInt(hwndDisplay, IDC_MAINTTIME, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "MaintTime",0 , REG_DWORD,(BYTE *)&MaintTime, 4);

	PR = GetDlgItemInt(hwndDisplay, IDM_PR, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "PR",0 , REG_DWORD,(BYTE *)&PR, 4);

	PUR = GetDlgItemInt(hwndDisplay, IDM_PUR, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "PUR",0 , REG_DWORD,(BYTE *)&PUR, 4);

	PF = GetDlgItemInt(hwndDisplay, IDM_PF, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "PF",0 , REG_DWORD,(BYTE *)&PF, 4);

	PNF = GetDlgItemInt(hwndDisplay, IDM_PNF, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "PNF",0 , REG_DWORD,(BYTE *)&PNF, 4);

	BF = GetDlgItemInt(hwndDisplay, IDM_BF, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "BF",0 , REG_DWORD,(BYTE *)&BF, 4);

	BNF = GetDlgItemInt(hwndDisplay, IDM_BNF, &OK1, FALSE);
	retCode = RegSetValueEx(hKey, "BNF",0 , REG_DWORD,(BYTE *)&BNF, 4);

	val = IsDlgButtonChecked(hwndDisplay, IDM_AP);
	retCode = RegSetValueEx(hKey,"AP", 0, REG_DWORD, (BYTE *)&val,4);

	val = IsDlgButtonChecked(hwndDisplay, IDM_AB);
	retCode = RegSetValueEx(hKey,"AB", 0, REG_DWORD, (BYTE *)&val,4);


	MultiLineDialogToREG_MULTI_SZ(hwndDisplay, IDM_LTFROM, hKey, "LTFROM");
	MultiLineDialogToREG_MULTI_SZ(hwndDisplay, IDM_LTTO, hKey, "LTTO");
	MultiLineDialogToREG_MULTI_SZ(hwndDisplay, IDM_LTAT, hKey, "LTAT");

	// Refresh local copy

	LTFROM = GetOverrides(hKey, "LTFROM");
	LTTO = GetOverrides(hKey, "LTTO");
	LTAT = GetOverrides(hKey, "LTAT");

	RegCloseKey(hKey);

	wsprintf(InfoBoxText, "Configuration Saved");
	DialogBox(hInst, MAKEINTRESOURCE(IDD_USERADDED_BOX), hWnd, InfoDialogProc);

}


VOID ReinitializeFWDStruct(struct UserInfo * user)
{
	if (user->ForwardingInfo)
	{
		FreeForwardingStruct(user);
		free(user->ForwardingInfo); 
	}

	SetupForwardingStruct(user);

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

VOID SaveWindowConfig()
{
	HKEY hKey=0;
	int retCode, disp;
	char Size[80];

	retCode = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat",
                              0,	// Reserved
							  0,	// Class
							  0,	// Options
                              KEY_ALL_ACCESS,
							  NULL,	// Security Attrs
                              &hKey,
							  &disp);

	if (retCode == ERROR_SUCCESS)
	{
		wsprintf(Size,"%d,%d,%d,%d",ConsoleRect.left,ConsoleRect.right,ConsoleRect.top,ConsoleRect.bottom);
		retCode = RegSetValueEx(hKey,"ConsoleSize",0,REG_SZ,(BYTE *)&Size, strlen(Size));

		wsprintf(Size,"%d,%d,%d,%d",MonitorRect.left,MonitorRect.right,MonitorRect.top,MonitorRect.bottom);
		retCode = RegSetValueEx(hKey,"MonitorSize",0,REG_SZ,(BYTE *)&Size, strlen(Size));

		wsprintf(Size,"%d,%d,%d,%d",MainRect.left,MainRect.right,MainRect.top,MainRect.bottom);
		retCode = RegSetValueEx(hKey,"WindowSize",0,REG_SZ,(BYTE *)&Size, strlen(Size));

		retCode = RegSetValueEx(hKey,"Bells",0,REG_DWORD,(BYTE *)&Bells,4);
		retCode = RegSetValueEx(hKey,"StripLF",0,REG_DWORD,(BYTE *)&StripLF,4);
		retCode = RegSetValueEx(hKey,"WarnWrap",0,REG_DWORD,(BYTE *)&WarnWrap,4);
		retCode = RegSetValueEx(hKey,"WrapInput",0,REG_DWORD,(BYTE *)&WrapInput,4);
		retCode = RegSetValueEx(hKey,"FlashOnConnect",0,REG_DWORD,(BYTE *)&FlashOnConnect,4);
		retCode = RegSetValueEx(hKey,"CloseWindowOnBye",0,REG_DWORD,(BYTE *)&CloseWindowOnBye,4);

		retCode = RegSetValueEx(hKey,"Log_BBS",0,REG_DWORD,(BYTE *)&LogBBS,4);
		retCode = RegSetValueEx(hKey,"Log_TCP",0,REG_DWORD,(BYTE *)&LogTCP,4);
		retCode = RegSetValueEx(hKey,"Log_CHAT",0,REG_DWORD,(BYTE *)&LogCHAT,4);


		RegCloseKey(hKey);
	}

	return;
}





BOOL GetConfigFromRegistry()
{
	HKEY hKey=0;
	int retCode,Type,Vallen;
	char Size[80];

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

		Vallen=4;
		RegQueryValueEx(hKey,"EnableUI",0,			
			(ULONG *)&Type,(UCHAR *)&EnableUI,(ULONG *)&Vallen);

		Vallen=100;

		RegQueryValueEx(hKey, "UIPortString",0,			
			(ULONG *)&Type,(UCHAR *)&UIPortString,(ULONG *)&Vallen);

		
		Vallen=4;
		RegQueryValueEx(hKey,"MaxTXSize",0,			
			(ULONG *)&Type,(UCHAR *)&MaxTXSize,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"MaxRXSize",0,			
			(ULONG *)&Type,(UCHAR *)&MaxRXSize,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"MaxFBBBlock",0,			
			(ULONG *)&Type,(UCHAR *)&MaxFBBBlockSize,(ULONG *)&Vallen);

		Vallen=100;

		retCode += RegQueryValueEx(hKey,"BBSName",0,			
			(ULONG *)&Type,(UCHAR *)&BBSName,(ULONG *)&Vallen);

		Vallen=100;
		retCode += RegQueryValueEx(hKey,"SYSOPCall",0,			
			(ULONG *)&Type,(UCHAR *)&SYSOPCall,(ULONG *)&Vallen);
				
		Vallen=100;
		retCode += RegQueryValueEx(hKey,"H-Route",0,			
			(ULONG *)&Type,(UCHAR *)&HRoute,(ULONG *)&Vallen);

		Vallen=MAX_PATH;
		retCode += RegQueryValueEx(hKey,"BaseDir",0,			
			(ULONG *)&Type,(UCHAR *)&BaseDir,(ULONG *)&Vallen);

		// Get length of Chatnodes String
				
		Vallen=0;
		RegQueryValueEx(hKey,"OtherChatNodes",0,			
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

		Vallen=4;
		RegQueryValueEx(hKey,"RemoteEmail",0,			
			(ULONG *)&Type,(UCHAR *)&RemoteEmail,(ULONG *)&Vallen);

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

		DecryptPass(EncryptedISPAccountPass, ISPAccountPass, EncryptedPassLen);

		Vallen=80;
		RegQueryValueEx(hKey,"ConsoleSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"Bells",0,			
			(ULONG *)&Type,(UCHAR *)&Bells,(ULONG *)&Vallen);
	
		Vallen=4;
		RegQueryValueEx(hKey,"StripLF",0,			
			(ULONG *)&Type,(UCHAR *)&StripLF,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"CloseWindowOnBye",0,			
			(ULONG *)&Type,(UCHAR *)&CloseWindowOnBye,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"WarnWrap",0,			
			(ULONG *)&Type,(UCHAR *)&WarnWrap,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"WrapInput",0,			
			(ULONG *)&Type,(UCHAR *)&WrapInput,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"FlashOnConnect",0,			
			(ULONG *)&Type,(UCHAR *)&FlashOnConnect,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"Log_BBS",0,			
			(ULONG *)&Type,(UCHAR *)&LogBBS,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"Log_TCP",0,			
			(ULONG *)&Type,(UCHAR *)&LogTCP,(ULONG *)&Vallen);

		Vallen=4;
		RegQueryValueEx(hKey,"Log_CHAT",0,			
			(ULONG *)&Type,(UCHAR *)&LogCHAT,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&ConsoleRect.left,&ConsoleRect.right,&ConsoleRect.top,&ConsoleRect.bottom);

		Vallen=80;
		RegQueryValueEx(hKey,"MonitorSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&MonitorRect.left,&MonitorRect.right,&MonitorRect.top,&MonitorRect.bottom);

		Vallen=80;
		RegQueryValueEx(hKey,"WindowSize",0,			
			(ULONG *)&Type,(UCHAR *)&Size,(ULONG *)&Vallen);

		sscanf(Size,"%d,%d,%d,%d",&MainRect.left,&MainRect.right,&MainRect.top,&MainRect.bottom);
		RegCloseKey(hKey);
	
		retCode += RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\G8BPQ\\BPQ32\\BPQMailChat\\Housekeeping",
                              0,
                              KEY_QUERY_VALUE,
                              &hKey);
		
		if (retCode == ERROR_SUCCESS)
		{
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"MaxMsgno",0,			
			(ULONG *)&Type,(UCHAR *)&MaxMsgno,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey,"BidLifetime",0,			
			(ULONG *)&Type,(UCHAR *)&BidLifetime,(ULONG *)&Vallen);

	
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"MaintInterval",0,			
			(ULONG *)&Type,(UCHAR *)&MaintInterval,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey,"MaintTime",0,			
			(ULONG *)&Type,(UCHAR *)&MaintTime,(ULONG *)&Vallen);

			Vallen=4;
			retCode += RegQueryValueEx(hKey,"PR",0,			
			(ULONG *)&Type,(UCHAR *)&PR,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"PUR",0,			
			(ULONG *)&Type,(UCHAR *)&PUR,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"PF",0,			
			(ULONG *)&Type,(UCHAR *)&PF,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"PNF",0,			
			(ULONG *)&Type,(UCHAR *)&PNF,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"BF",0,			
			(ULONG *)&Type,(UCHAR *)&BF,(ULONG *)&Vallen);
		
			Vallen=4;
			retCode += RegQueryValueEx(hKey,"BNF",0,			
			(ULONG *)&Type,(UCHAR *)&BNF,(ULONG *)&Vallen);


			Vallen=4;
			retCode += RegQueryValueEx(hKey, "AP", 0,			
				(ULONG *)&Type,(UCHAR *)&AP,(ULONG *)&Vallen);
				
			Vallen=4;
			retCode += RegQueryValueEx(hKey, "AB", 0,			
				(ULONG *)&Type,(UCHAR *)&AB,(ULONG *)&Vallen);


			LTFROM = GetOverrides(hKey, "LTFROM");
			LTTO = GetOverrides(hKey, "LTTO");
			LTAT = GetOverrides(hKey, "LTAT");

		}

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


INT_PTR CALLBACK UserEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command, n;
		
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{

	case WM_INITDIALOG:

		for (n = 1; n <= NumberofUsers; n++)
		{
			SendDlgItemMessage(hDlg, IDC_USER, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)UserRecPtr[n]->Call);
		} 

		return (INT_PTR)TRUE;

		
	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }

	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;


		case IDC_USER:

			// User Selection Changed

			Do_User_Sel_Changed(hDlg);

			return TRUE;


		case IDC_ADDUSER:

			Do_Add_User(hDlg);
			return TRUE;

		case IDC_DELETEUSER:

			Do_Delete_User(hDlg);
			return TRUE;

		case IDC_SAVEUSER:

			Do_Save_User(hDlg, TRUE);
			return TRUE;

		}
		break;
	}
	
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK MsgEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command, n;
	char msgno[20];
	struct UserInfo * user;


	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{

	case WM_INITDIALOG:

		for (n = NumberofMessages; n >= 1; n--)
		{
			wsprintf(msgno, "%d", MsgHddrPtr[n]->number);
			SendDlgItemMessage(hDlg, 0, LB_ADDSTRING, 0, (LPARAM)msgno);
		} 

		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "B");
		SendDlgItemMessage(hDlg, IDC_MSGTYPE, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "P");

		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "N");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "Y");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "F");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "K");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "H");
		SendDlgItemMessage(hDlg, IDC_MSGSTATUS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR) "$");

		for (n = 1; n <= NBBBS; n++ )
		{
			CheckDlgButton(hDlg, n + 24, BST_INDETERMINATE);
		}

		for (user = BBSChain; user; user = user->BBSNext)
		{
			SetDlgItemText(hDlg, user->BBSNumber + 24, user->Call);
			CheckDlgButton(hDlg, user->BBSNumber + 24, BST_INDETERMINATE);
		}

		CheckDlgButton(hDlg,105, BST_INDETERMINATE);
		CheckDlgButton(hDlg,106, BST_UNCHECKED);
		CheckDlgButton(hDlg,107, BST_CHECKED);

		return (INT_PTR)TRUE;

	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


	case WM_COMMAND:

		Command = LOWORD(wParam);

		switch (Command)
		{

		case IDOK:
		case IDCANCEL:

			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;

		case 0:

			// Msg Selection Changed

			Do_Msg_Sel_Changed(hDlg);

			return TRUE;

		case IDC_SAVEMSG:

			Do_Save_Msg(hDlg);
			return TRUE;



		}
		break;
	}
	
	return (INT_PTR)FALSE;
}

INT_PTR CALLBACK FwdEditDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Command;
	struct UserInfo * user;

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{

	case WM_INITDIALOG:

		for (user = BBSChain; user; user = user->BBSNext)
		{
			SendDlgItemMessage(hDlg, IDC_BBS, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)user->Call);
		}  

		SetDlgItemInt(hDlg, IDC_MAXSEND, MaxTXSize, FALSE);
		SetDlgItemInt(hDlg, IDC_MAXRECV, MaxRXSize, FALSE);
		SetDlgItemInt(hDlg, IDC_MAXBLOCK, MaxFBBBlockSize, FALSE);

		return (INT_PTR)TRUE;


	case WM_CTLCOLORDLG:

        return (LONG)bgBrush;

    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(0, 0, 0));
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LONG)bgBrush;
    }


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



		case IDC_FWDSAVE:
			
			SaveFWDConfig(hDlg);
			return TRUE;

		}
		break;
	}
	
	return (INT_PTR)FALSE;
}
