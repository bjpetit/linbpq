// Mail and Chat Server for BPQ32 Packet Switch
//
// Message Routing Module

// This code decides where to send a message.

// Private messages are routed by TO and AT if possible, if not they follow the Bull Rules

// Bulls are routed on HA where possible

// Bulls should not be distributed outside their designated area.

#include "stdafx.h"


char * MyElements[20];		// My HA in element format

char MyRouteElements[100];


VOID SetupMyHA()
{
	int Elements = 0;
	char * ptr2;

	strcpy(MyRouteElements, HRoute);
	
	// Split it up

	ptr2 = MyRouteElements + strlen(MyRouteElements) - 1;

	do 
	{
		while ((*ptr2 != '.') && (ptr2 > MyRouteElements))
		{
			ptr2 --;
		}

		if (ptr2 == MyRouteElements)
		{
			// End
	
			MyElements[Elements++] = ptr2;
			break;
		}

		MyElements[Elements++] = ptr2+1;
		*ptr2 = 0;

	} while(Elements < 20);		// Just in case!

	MyElements[Elements++] = BBSName;

}


VOID SetupHAddreses(struct BBSForwardingInfo * ForwardingInfo)
{
	int Count=0;
	char ** HText = ForwardingInfo->Haddresses;

	char * SaveHText, * ptr2;

	ForwardingInfo->HADDRS = zalloc(4);				// always NULL entry on end even if no values

	while(HText[0])
	{
		int Elements = 0;
		ForwardingInfo->HADDRS = realloc(ForwardingInfo->HADDRS, (Count+2)*4);
	
		ForwardingInfo->HADDRS[Count] = zalloc(4);

		SaveHText = _strdup(HText[0]);

		ptr2 = SaveHText + strlen(SaveHText) -1;

		do 
		{
			ForwardingInfo->HADDRS[Count] = realloc(ForwardingInfo->HADDRS[Count], (Elements+2)*4);

			while ((*ptr2 != '.') && (ptr2 > SaveHText))
			{
				ptr2 --;
			}

			if (ptr2 == SaveHText)
			{
				// End
	
				ForwardingInfo->HADDRS[Count][Elements++] = ptr2;
				break;
			}

			ForwardingInfo->HADDRS[Count][Elements++] = ptr2+1;
			*ptr2 = 0;

		} while(TRUE);

		ForwardingInfo->HADDRS[Count][Elements++] = NULL;

		HText++;
		Count++;
	}

	ForwardingInfo->HADDRS[Count] = NULL;
}

int MatchMessagetoBBSList(struct MsgInfo * Msg)
{
	struct UserInfo * bbs;
	struct	BBSForwardingInfo * ForwardingInfo;
	char ATBBS[41];
	char RouteElements[41];
	char * HRoute;
	int Count = 0;
	char * HElements[20] = {NULL};
	int Elements = 0;
	char * ptr2;
	int MyElement = 0;
	int FirstElement = 0;
	BOOL Flood = FALSE;

	strcpy(ATBBS, Msg->via);
	strcpy(RouteElements, Msg->via);

	HRoute = strlop(ATBBS, '.');

	if (HRoute)
	{
		// Split it up

		ptr2 = RouteElements + strlen(RouteElements) - 1;

		do 
		{
			while ((*ptr2 != '.') && (ptr2 > RouteElements))
			{
				ptr2 --;
			}

			if (ptr2 != RouteElements)
				*ptr2++ = 0;
	
			HElements[Elements++] = ptr2;


		} while(Elements < 20 && ptr2 != RouteElements);		// Just in case!
	}

	// if Bull, see if reached right area

	if (Msg->type == 'B')
	{
		int i = 0;
		
		// All elements of Helements must match Myelements

		while (MyElements[i] && HElements[i]) // Until one set runs out
		{
			if (strcmp(MyElements[i], HElements[i]) != 0)
				break;
			i++;
		}

		// if HElements[i+1] = NULL, have matched all

		if (HElements[i+1] == NULL)
			Flood = TRUE;
	}


	// Check againt our HR. If n elememts match, remove (n-1) (by setting start pointer)

	FirstElement = 0;

	while (HElements[FirstElement])
	{
		if (strcmp(HElements[FirstElement], MyElements[FirstElement]) != 0)
			break;

		FirstElement++;
	}

	if (FirstElement) FirstElement--;


	if (Msg->type == 'P' || Flood == 0)
	{
		// P messages are only sent to one BBS, but check the TO and AT of all BBSs before routing on HA

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSToList(Msg, bbs, ForwardingInfo))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSAtList(Msg, bbs, ForwardingInfo, ATBBS))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSHElements(Msg, bbs, ForwardingInfo, ATBBS, &HElements[FirstElement]))
			{
				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
				return 1;
			}
		}

		return FALSE;
	}

	// Flood Bulls go to all matching BBSs, so the order of checking doesn't matter

	
	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		ForwardingInfo = bbs->ForwardingInfo;

		if (CheckBBSToList(Msg, bbs, ForwardingInfo))
		{
			if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
			{
				set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
				ForwardingInfo->MsgCount++;
			}
			Count++;
			continue;
		}

		if (CheckBBSAtList(Msg, bbs, ForwardingInfo, ATBBS))
		{
			if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
			{
				set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
				ForwardingInfo->MsgCount++;
			}
			Count++;
			continue;
		}



		if (CheckBBSHElements(Msg, bbs, ForwardingInfo, ATBBS, &HElements[FirstElement]))
		{
			if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
			{
				set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
				ForwardingInfo->MsgCount++;
			}
			Count++;
		}
	}

	return Count;
}
BOOL CheckABBS(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** Calls;
	char ** HRoutes;
	int i, j;

	if (strcmp(ATBBS, bbs->Call) == 0)					// @BBS = BBS
		return TRUE;

	// Check TO distributions

	if (ForwardingInfo->TOCalls)
	{
		Calls = ForwardingInfo->TOCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}

	// Check AT distributions

	if (ForwardingInfo->ATCalls)
	{
		Calls = ForwardingInfo->ATCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}
	if ((HRoute) &&	(ForwardingInfo->Haddresses))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->Haddresses;

		while(HRoutes[0])
		{
			i = strlen(HRoutes[0]) - 1;
			j = strlen(HRoute) - 1;

			while ((i >= 0) && (j >= 0))				// Until one string rus out
			{
				if (HRoutes[0][i--] != HRoute[j--])	// Compare backwards
					goto next;
			}

			return TRUE;
		next:	
			HRoutes++;
		}
	}


	return FALSE;

}

BOOL CheckBBSToList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo)
{
	char ** Calls;

	// Check TO distributions

	if (ForwardingInfo->TOCalls)
	{
		Calls = ForwardingInfo->TOCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], Msg->to) == 0)	
				return TRUE;

			Calls++;
		}
	}
	return FALSE;
}

BOOL CheckBBSAtList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS)
{
	char ** Calls;

	// Check AT distributions

	if (strcmp(ATBBS, bbs->Call) == 0)			// @BBS = BBS
		return TRUE;

	if (ForwardingInfo->ATCalls)
	{
		Calls = ForwardingInfo->ATCalls;

		while(Calls[0])
		{
			if (strcmp(Calls[0], ATBBS) == 0)	
				return TRUE;

			Calls++;
		}
	}
	return FALSE;
}
/*

BOOL CheckBBSHList(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char * HRoute)
{
	char ** HRoutes;
	int i, j;

	if ((HRoute) &&	(ForwardingInfo->Haddresses))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->Haddresses;

		while(HRoutes[0])
		{
			i = strlen(HRoutes[0]) - 1;
			j = strlen(HRoute) - 1;

			while ((i >= 0) && (j >= 0))				// Until one string rus out
			{
				if (HRoutes[0][i--] != HRoute[j--])	// Compare backwards
					goto next;
			}

			return TRUE;
		next:	
			HRoutes++;
		}
	}
	return FALSE;
}
*/
BOOL CheckBBSHElements(struct MsgInfo * Msg, struct UserInfo * bbs, struct	BBSForwardingInfo * ForwardingInfo, char * ATBBS, char ** HElements)
{
	char *** HRoutes;
	int i, j;

	if ((HRoute) &&	(ForwardingInfo->HADDRS))
	{
		// Match on Routes

		HRoutes = ForwardingInfo->HADDRS;

		while(HRoutes[0])
		{
			i =j = 0;
			
			while (HRoutes[0][i] && HElements[j]) // Until one set runs out
			{
				if (strcmp(HRoutes[0][i++], HElements[j++]) != 0)
					goto next;

			}
			return TRUE;
		next:	
			HRoutes++;
		}
	}
	return FALSE;
}
/*
EU should match fra.eu, but not gbr.eu in gbr.eu

gbr.eu should match #25.gbr.ee, but not #23.gbr.eu in #23.gbr.eu

So, I shouldn't remove EU unless gbr.eu matches/

I ahouldn'r remove gbr.eu unless #23.gbr.eu matches/

Not quite.

How about:

If B and all elements of message HA match our HA, then can flood, else route.

If P message, or B(route) send to bast (ie longest matching HA)

if B (flood) send to all matching HA (having lopped of matching n-1

so B to gbr.eu will go to best for gbr.eu unless in gbr, else will go to all *.gbr













*/