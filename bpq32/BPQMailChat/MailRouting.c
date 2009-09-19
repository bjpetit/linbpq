// Mail and Chat Server for BPQ32 Packet Switch
//
// Message Routing Module

// This code decides where to send a message.

// Private messages are routed by TO and AT if possible, if not they follow the Bull Rules

// Bulls are routed on HA where possible

// Bulls should not be distributed outside their designated area.

#include "stdafx.h"

char WW[] = "WW";

char * MyElements[20] = {WW};		// My HA in element format

char MyRouteElements[100];

int MyElementCount;

BOOL ReaddressLocal;
BOOL ReaddressReceived;


struct Continent
{
	char FourCharCode[5];
	char TwoCharCode[3];
};

struct Country
{
	char Country[5];
	char Continent[5];
};


struct Continent Continents[] =
{
   		"EURO",	"EU", // Europe
   		"MEDR",	"EU", // Mediterranean
   		"INDI",	"AS", // Indian Ocean including the Indian subcontinent
   		"MDLE",	"EU", // Middle East
   		"SEAS",	"EU", // South-East Asia
   		"ASIA",	"AS", // The Orient
   		"NOAM",	"NA", // North America (Canada, USA, Mexico)
   		"CEAM",	"NA", // Central America
   		"CARB",	"NA", // Caribbean
   		"SOAM",	"SA", // South America
   		"AUNZ",	"OC", // Australia/New Zealand
   		"EPAC",	"OC", // Eastern Pacific
   		"NPAC",	"OC", // Northern Pacific
   		"SPAC",	"OC", // Southern Pacific
   		"WPAC",	"OC", // Western Pacific
   		"NAFR",	"AF", // Northern Africa
   		"CAFR",	"AF", // Central Africa
   		"SAFR",	"AF", // Southern Africa
   		"ANTR",	"OC", // Antarctica 
};

struct Country Countries[] = 
{			
		"AFG", "****",		// Afghanistan
		"ALA", "****",		// Åland Islands
		"ALB", "****",		// Albania
		"DZA", "****",		// Algeria
		"ASM", "****",		// American Samoa
		"AND", "EURO",		// Andorra
		"AGO", "****",		// Angola
		"AIA", "****",		// Anguilla
		"ATG", "****",		// Antigua and Barbuda
		"ARG", "SOAM",		// Argentina
		"ARM", "****",		// Armenia
		"ABW", "****",		// Aruba
		"AUS", "AUNZ",		// Australia
		"AUT", "****",		// Austria
		"AZE", "****",		// Azerbaijan
		"BHS", "****",		// Bahamas
		"BHR", "****",		// Bahrain
		"BGD", "****",		// Bangladesh
		"BRB", "****",		// Barbados
		"BLR", "****",		// Belarus
		"BEL", "EURO",		// Belgium
		"BLZ", "****",		// Belize
		"BEN", "****",		// Benin
		"BMU", "****",		// Bermuda
		"BTN", "****",		// Bhutan
		"BOL", "****",		// Bolivia (Plurinational State of)
		"BIH", "****",		// Bosnia and Herzegovina
		"BWA", "****",		// Botswana
		"BRA", "****",		// Brazil
		"VGB", "****",		// British Virgin Islands
		"BRN", "****",		// Brunei Darussalam
		"BGR", "****",		// Bulgaria
		"BFA", "****",		// Burkina Faso
		"BDI", "****",		// Burundi
		"KHM", "****",		// Cambodia
		"CMR", "****",		// Cameroon
		"CAN", "NOAM",		// Canada
		"CPV", "****",		// Cape Verde
		"CYM", "****",		// Cayman Islands
		"CAF", "****",		// Central African Republic
		"TCD", "****",		// Chad
		"CHL", "****",		// Chile
		"CHN", "****",		// China
		"HKG", "****",		// Hong Kong Special Administrative Region of China
		"MAC", "****",		// Macao Special Administrative Region of China
		"COL", "****",		// Colombia
		"COM", "****",		// Comoros
		"COG", "****",		// Congo
		"COK", "****",		// Cook Islands
		"CRI", "****",		// Costa Rica
		"CIV", "****",		// Côte d'Ivoire
		"HRV", "EURO",		// Croatia
		"CUB", "****",		// Cuba
		"CYP", "****",		// Cyprus
		"CZE", "EURO",		// Czech Republic
		"PRK", "****",		// Democratic People's Republic of Korea
		"COD", "****",		// Democratic Republic of the Congo
		"DNK", "EURO",		// Denmark
		"DJI", "****",		// Djibouti
		"DMA", "****",		// Dominica
		"DOM", "****",		// Dominican Republic
		"ECU", "****",		// Ecuador
		"EGY", "****",		// Egypt
		"SLV", "****",		// El Salvador
		"GNQ", "****",		// Equatorial Guinea
		"ERI", "****",		// Eritrea
		"EST", "EURO",		// Estonia
		"ETH", "****",		// Ethiopia
		"FRO", "EURO",		// Faeroe Islands
		"FLK", "****",		// Falkland Islands (Malvinas)
		"FJI", "****",		// Fiji
		"FIN", "EURO",		// Finland
		"FRA", "EURO",		// France
		"GUF", "****",		// French Guiana
		"PYF", "****",		// French Polynesia
		"GAB", "****",		// Gabon
		"GMB", "****",		// Gambia
		"GEO", "****",		// Georgia
		"DEU", "EURO",		// Germany
		"GHA", "****",		// Ghana
		"GIB", "EURO",		// Gibraltar
		"GRC", "EURO",		// Greece
		"GRL", "****",		// Greenland
		"GRD", "****",		// Grenada
		"GLP", "****",		// Guadeloupe
		"GUM", "****",		// Guam
		"GTM", "****",		// Guatemala
		"GGY", "EURO",		// Guernsey
		"GIN", "****",		// Guinea
		"GNB", "****",		// Guinea-Bissau
		"GUY", "****",		// Guyana
		"HTI", "****",		// Haiti
		"VAT", "EURO",		// Holy See
		"HND", "****",		// Honduras
		"HUN", "EURO",		// Hungary
		"ISL", "EURO",		// Iceland
		"IND", "****",		// India
		"IDN", "****",		// Indonesia
		"IRN", "****",		// Iran (Islamic Republic of)
		"IRQ", "****",		// Iraq
		"IRL", "EURO",		// Ireland
		"IMN", "EURO",		// Isle of Man
		"ISR", "****",		// Israel
		"ITA", "EURO",		// Italy
		"JAM", "****",		// Jamaica
		"JPN", "****",		// Japan
		"JEY", "EURO",		// Jersey
		"JOR", "****",		// Jordan
		"KAZ", "****",		// Kazakhstan
		"KEN", "****",		// Kenya
		"KIR", "****",		// Kiribati
		"KWT", "****",		// Kuwait
		"KGZ", "****",		// Kyrgyzstan
		"LAO", "****",		// Lao People's Democratic Republic
		"LVA", "EURO",		// Latvia
		"LBN", "****",		// Lebanon
		"LSO", "****",		// Lesotho
		"LBR", "****",		// Liberia
		"LBY", "****",		// Libyan Arab Jamahiriya
		"LIE", "EURO",		// Liechtenstein
		"LTU", "EURO",		// Lithuania
		"LUX", "EURO",		// Luxembourg
		"MDG", "****",		// Madagascar
		"MWI", "****",		// Malawi
		"MYS", "****",		// Malaysia
		"MDV", "****",		// Maldives
		"MLI", "****",		// Mali
		"MLT", "EURO",		// Malta
		"MHL", "****",		// Marshall Islands
		"MTQ", "****",		// Martinique
		"MRT", "****",		// Mauritania
		"MUS", "****",		// Mauritius
		"MYT", "****",		// Mayotte
		"MEX", "****",		// Mexico
		"FSM", "****",		// Micronesia (Federated States of)
		"MCO", "EURO",		// Monaco
		"MNG", "****",		// Mongolia
		"MNE", "****",		// Montenegro
		"MSR", "****",		// Montserrat
		"MAR", "****",		// Morocco
		"MOZ", "****",		// Mozambique
		"MMR", "****",		// Myanmar
		"NAM", "****",		// Namibia
		"NRU", "****",		// Nauru
		"NPL", "****",		// Nepal
		"NLD", "EURO",		// Netherlands
		"ANT", "****",		// Netherlands Antilles
		"NCL", "****",		// New Caledonia
		"NZL", "AUNZ",		// New Zealand
		"NIC", "****",		// Nicaragua
		"NER", "****",		// Niger
		"NGA", "****",		// Nigeria
		"NIU", "****",		// Niue
		"NFK", "****",		// Norfolk Island
		"MNP", "****",		// Northern Mariana Islands
		"NOR", "EURO",		// Norway
		"PSE", "****",		// Occupied Palestinian Territory
		"OMN", "****",		// Oman
		"PAK", "****",		// Pakistan
		"PLW", "****",		// Palau
		"PAN", "****",		// Panama
		"PNG", "****",		// Papua New Guinea
		"PRY", "****",		// Paraguay
		"PER", "****",		// Peru
		"PHL", "****",		// Philippines
		"PCN", "****",		// Pitcairn
		"POL", "EURO",		// Poland
		"PRT", "EURO",		// Portugal
		"PRI", "****",		// Puerto Rico
		"QAT", "****",		// Qatar
		"KOR", "****",		// Republic of Korea
		"MDA", "****",		// Republic of Moldova
		"REU", "****",		// Réunion
		"ROU", "****",		// Romania
		"RUS", "****",		// Russian Federation
		"RWA", "****",		// Rwanda
		"BLM", "****",		// Saint-Barthélemy
		"SHN", "****",		// Saint Helena
		"KNA", "****",		// Saint Kitts and Nevis
		"LCA", "****",		// Saint Lucia
		"MAF", "****",		// Saint-Martin (French part)
		"SPM", "****",		// Saint Pierre and Miquelon
		"VCT", "****",		// Saint Vincent and the Grenadines
		"WSM", "****",		// Samoa
		"SMR", "****",		// San Marino
		"STP", "****",		// Sao Tome and Principe
		"SAU", "****",		// Saudi Arabia
		"SEN", "****",		// Senegal
		"SRB", "EURO",		// Serbia
		"SYC", "****",		// Seychelles
		"SLE", "****",		// Sierra Leone
		"SGP", "****",		// Singapore
		"SVK", "EURO",		// Slovakia
		"SVN", "EURO",		// Slovenia
		"SLB", "****",		// Solomon Islands
		"SOM", "****",		// Somalia
		"ZAF", "****",		// South Africa
		"ESP", "EURO",		// Spain
		"LKA", "****",		// Sri Lanka
		"SDN", "****",		// Sudan
		"SUR", "****",		// Suriname
		"SJM", "****",		// Svalbard and Jan Mayen Islands
		"SWZ", "****",		// Swaziland
		"SWE", "EURO",		// Sweden
		"CHE", "EURO",		// Switzerland
		"SYR", "****",		// Syrian Arab Republic
		"TJK", "****",		// Tajikistan
		"THA", "****",		// Thailand
		"MKD", "EURO",		// The former Yugoslav Republic of Macedonia
		"TLS", "****",		// Timor-Leste
		"TGO", "****",		// Togo
		"TKL", "****",		// Tokelau
		"TON", "****",		// Tonga
		"TTO", "****",		// Trinidad and Tobago
		"TUN", "****",		// Tunisia
		"TUR", "****",		// Turkey
		"TKM", "****",		// Turkmenistan
		"TCA", "****",		// Turks and Caicos Islands
		"TUV", "****",		// Tuvalu
		"UGA", "****",		// Uganda
		"UKR", "****",		// Ukraine
		"ARE", "****",		// United Arab Emirates
		"GBR", "EURO",		// United Kingdom of Great Britain and Northern Ireland
		"TZA", "****",		// United Republic of Tanzania
		"USA", "NOAM",		// United States of America
		"VIR", "****",		// United States Virgin Islands
		"URY", "****",		// Uruguay
		"UZB", "****",		// Uzbekistan
		"VUT", "****",		// Vanuatu
		"VEN", "****",		// Venezuela (Bolivarian Republic of)
		"VNM", "****",		// Viet Nam
		"WLF", "****",		// Wallis and Futuna Islands
		"ESH", "****",		// Western Sahara
		"YEM", "****",		// Yemen
		"ZMB", "SAFR",		// Zambia
		"ZWE", "SAFR"};		// Zimbabwe

char ** AliasText;
struct ALIAS ** Aliases;

/*struct ALIAS Aliases[] =
{
	"AMSAT",	"WW",
	"USBBS",	"USA",
	"ALLUS",	"USA"};
*/

int NumberofContinents = sizeof(Continents)/sizeof(Continents[1]);
int NumberofCountries = sizeof(Countries)/sizeof(Countries[1]);

struct Continent * FindContinent(char * Name)
{
	int i;
	struct Continent * Cont;

	for(i=0; i< NumberofContinents; i++)
	{
		Cont = &Continents[i];
		
		if ((_stricmp(Name, Cont->FourCharCode) == 0) || (_stricmp(Name, Cont->TwoCharCode) == 0))
			return Cont;

	}

	return NULL;
}

struct Country * FindCountry(char * Name)
{
	int i;
	struct Country * Cont;

	for(i=0; i< NumberofCountries; i++)
	{
		Cont = &Countries[i];
		
		if (_stricmp(Name, Cont->Country) == 0)
			return Cont;
	}

	return NULL;
}

struct ALIAS * FindAlias(char * Name)
{
	struct ALIAS ** Alias;

	Alias =  Aliases;

	while(Alias[0])
	{
		if (_stricmp(Name, Alias[0]->Dest) == 0)
			return Alias[0];
	
		Alias++;
	}

	return NULL;
}

#ifdef NEWROUTING



VOID SetupMyHA()
{
	int Elements = 1;					// Implied WW on front
	char * ptr2;
	struct Continent * Continent;
	struct Country * Country;

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
	
			MyElements[Elements++] = _strdup(ptr2);
			break;
		}

		MyElements[Elements++] = _strdup(ptr2+1);
		*ptr2 = 0;

	} while(Elements < 20);		// Just in case!

	MyElements[Elements++] = _strdup(BBSName);

	MyElementCount = Elements;

	if (MyElements[1])
	{
		if (strlen(MyElements[1]) == 2)
		{
			// Convert to 4 char Continent;
			Continent = FindContinent(MyElements[1]);
			if (Continent)
			{
				free(MyElements[1]);
				MyElements[1] = _strdup(Continent->FourCharCode);
			}
		}
	}
}

VOID SetupFwdAliases()
{
	char ** Text = AliasText;
	char * Dest, * Alias;
	int Count = 0;
	char seps[] = " ,:/";

	Aliases = zalloc(sizeof(struct ALIAS));

	if (Text)
	{
		while(Text[0])
		{
			Aliases = realloc(Aliases, (Count+2)* sizeof(struct ALIAS));
			Aliases[Count] = zalloc(sizeof(struct ALIAS));

			Dest = _strdup(Text[0]);		// strtok changes it

			Dest = strtok_s(Dest, seps, &Alias);

			Aliases[Count]->Dest =  Dest; 
			Aliases[Count]->Alias = Alias;

			Count++;
			Text++;
		}
		Aliases[Count] = NULL;
	}
}



VOID SetupHAddreses(struct BBSForwardingInfo * ForwardingInfo)
{
	int Count=0;
	char ** HText = ForwardingInfo->Haddresses;
	char * SaveHText, * ptr2;
	char * TopElement;
	char * Num;
	struct Continent * Continent;
	struct Country * Country;


	ForwardingInfo->HADDRS = zalloc(4);				// always NULL entry on end even if no values
	ForwardingInfo->HADDROffet = zalloc(4);			// always NULL entry on end even if no values

	while(HText[0])
	{
		int Elements = 1;
		ForwardingInfo->HADDRS = realloc(ForwardingInfo->HADDRS, (Count+2)*4);
		ForwardingInfo->HADDROffet = realloc(ForwardingInfo->HADDROffet, (Count+2)*4);
	
		ForwardingInfo->HADDRS[Count] = zalloc(4);

		SaveHText = _strdup(HText[0]);
		Num = strlop(SaveHText, ',');

		ptr2 = SaveHText + strlen(SaveHText) -1;

		ForwardingInfo->HADDRS[Count][0] = _strdup(WW);

		if (strcmp(HText[0], "WW") != 0)
		{

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
	
				ForwardingInfo->HADDRS[Count][Elements++] = _strdup(ptr2);
				break;
			}

			ForwardingInfo->HADDRS[Count][Elements++] = _strdup(ptr2+1);
			*ptr2 = 0;

		} while(TRUE);
		}

		ForwardingInfo->HADDRS[Count][Elements++] = NULL;

		// If the route is not a complete HR (ie starting with a continent)
		// Add elemets from BBS HA to complete it.
		// (How do we know how many??
		// ?? Still ont sure about this ??
		// What i was trying to do was end up with a full HR, but knowing how many elements to use.
		// Far simpler to config it, but can users cope??
		//	Will config for testing. HA, N

/*
		TopElement=ForwardingInfo->HADDRS[Count][0];

		if (strcmp(TopElement, MyElements[0]) == 0)
			goto FullHR;

		if (FindContinent(TopElement))
			goto FullHR;
	
		// Need to add stuff from our HR

		Elements--;

		if (Elements < MyElementCount)
			break;

FullHR:
*/

		ForwardingInfo->HADDROffet[Count] = (Num)? atoi(Num): 0;

		if (ForwardingInfo->HADDRS[Count][1])
		{
			if (strlen(ForwardingInfo->HADDRS[Count][1]) == 2)
			{
				// Convert to 4 char Continent;
				Continent = FindContinent(ForwardingInfo->HADDRS[Count][1]);
				if (Continent)
				{
					free(ForwardingInfo->HADDRS[Count][1]);
					ForwardingInfo->HADDRS[Count][1] = _strdup(Continent->FourCharCode);
				}
			}
		}
		free(SaveHText);
		HText++;
		Count++;
	}

	ForwardingInfo->HADDRS[Count] = NULL;


}

int MatchMessagetoBBSList(struct MsgInfo * Msg, CIRCUIT * conn)
{
	struct UserInfo * bbs;
	struct	BBSForwardingInfo * ForwardingInfo;
	char ATBBS[41];
	char RouteElements[41];
	int Count = 0;
	char * HElements[20] = {NULL};
	int Elements = 0;
	char * ptr2;
	int MyElement = 0;
	int FirstElement = 0;
	BOOL Flood = FALSE;
	char FullRoute[100];
	struct Continent * Continent;
	struct Country * Country;
	struct ALIAS * Alias;


	strcpy(RouteElements, Msg->via);

	// See if a well-known alias

	Alias = FindAlias(RouteElements);
	
	if (Alias)
		strcpy(RouteElements, Alias->Alias);

// Make sure HA is complete (starting at WW)

	if (RouteElements[0] == 0)
		goto NOHA;

	ptr2 = RouteElements + strlen(RouteElements) - 1;

	while ((*ptr2 != '.') && (ptr2 > RouteElements))
	{
		ptr2 --;
	}

	if (ptr2 != RouteElements)
		*ptr2++ = 0;

	if ((strcmp(ptr2, "WW") == 0) || (strcmp(ptr2, "WWW") == 0))
	{
		strcpy(FullRoute, RouteElements);
		goto FULLHA;
	}

	if (FindContinent(ptr2))
	{
		// Just need to add WW

		sprintf_s(FullRoute, sizeof(FullRoute),"%s.WW", RouteElements);
		goto FULLHA;
	}

	Country = FindCountry(ptr2);
	
	if (Country)
	{
		// Just need to add Continent and WW

		sprintf_s(FullRoute, sizeof(FullRoute),"%s.%s.WW", RouteElements, Country->Continent);
		goto FULLHA;
	}

	// Don't know

	strcpy(FullRoute, RouteElements);


FULLHA:

	strcpy(ATBBS, FullRoute);

	strlop(ATBBS, '.');

	if (FullRoute)
	{
		// Split it up

		ptr2 = FullRoute + strlen(FullRoute) - 1;

		do 
		{
			while ((*ptr2 != '.') && (ptr2 > FullRoute))
			{
				ptr2 --;
			}

			if (ptr2 != FullRoute)
				*ptr2++ = 0;
	
			HElements[Elements++] = ptr2;


		} while(Elements < 20 && ptr2 != FullRoute);		// Just in case!
	}

	if (HElements[1])
		{

		if (strlen(HElements[1]) == 2)
		{
			// Convert to 4 char Continent;
			Continent = FindContinent(HElements[1]);
			if (Continent)
			{
//				free(MyElements[1]);
				HElements[1] = _strdup(Continent->FourCharCode);
			}
		}
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

		if (HElements[i] == NULL)
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
	FirstElement = 0;

	if (FirstElement) FirstElement--;

NOHA:

	Logprintf(LOG_BBS, '?', "Routing Trace Type %c %s VIA %s Route On %s %s %s %s %s",
		Msg->type, (Flood) ? "(Flood)":"", Msg->via, HElements[FirstElement],
		HElements[FirstElement+1], HElements[FirstElement+2], HElements[FirstElement+3], HElements[FirstElement+4]);


	if (Msg->type == 'P' || Flood == 0)
	{
		// P messages are only sent to one BBS, but check the TO and AT of all BBSs before routing on HA,
		// and choose the best match on HA

		struct UserInfo * bestbbs = NULL;
		int bestmatch = 0;
		int depth;

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;
			
			if (CheckBBSToList(Msg, bbs, ForwardingInfo))
			{
				Logprintf(LOG_BBS, '?', "Routing Trace TO %s Matches BBS %s", Msg->to, bbs->Call);

				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
					{
						set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
						ForwardingInfo->MsgCount++;
					}
				}
				return 1;
			}
		}

		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;

			// Check AT 

			if (strcmp(ATBBS, bbs->Call) == 0)			// @BBS = BBS		
//			if (CheckBBSAtList(Msg, bbs, ForwardingInfo, ATBBS))
			{
				Logprintf(LOG_BBS, '?', "Routing Trace AT %s Matches BBS %s", ATBBS, bbs->Call);

				if (_stricmp(bbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
				{
					if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
					{
						set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
						ForwardingInfo->MsgCount++;
					}
				}
				return 1;
			}
		}

		// We should choose the BBS with most matching elements (ie match on #23.GBR better that GBR)


		for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
		{		
			ForwardingInfo = bbs->ForwardingInfo;

			depth = CheckBBSHElements(Msg, bbs, ForwardingInfo, ATBBS, &HElements[FirstElement], Flood);

			if (depth)
			{
				Logprintf(LOG_BBS, '?', "Routing Trace HR Matches BBS %s Depth %d", bbs->Call, depth);
		
				if (depth > bestmatch)
				{
					bestmatch = depth;
					bestbbs = bbs;
				}
			}
		}
		if (bestbbs)
		{
			Logprintf(LOG_BBS, '?', "Routing Trace HR Best Match is %s", bestbbs->Call);

			if (_stricmp(bestbbs->Call, BBSName) != 0)			// Dont forward to ourself - already here!
			{
				if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bestbbs->Call) != 0)) // Dont send back
				{
					set_fwd_bit(Msg->fbbs, bestbbs->BBSNumber);
					bestbbs->ForwardingInfo->MsgCount++;
				}
			}
			return 1;
		}

		return FALSE;	// No match
	}

	// Flood Bulls go to all matching BBSs, so the order of checking doesn't matter

	for (bbs = BBSChain; bbs; bbs = bbs->BBSNext)
	{		
		ForwardingInfo = bbs->ForwardingInfo;

		if (CheckBBSToList(Msg, bbs, ForwardingInfo))
		{
			Logprintf(LOG_BBS, '?', "Routing Trace TO %s Matches BBS %s", Msg->to, bbs->Call);

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
				if ((conn == NULL) || (_stricmp(conn->UserPointer->Call, bbs->Call) != 0)) // Dont send back
				{
					set_fwd_bit(Msg->fbbs, bbs->BBSNumber);
					ForwardingInfo->MsgCount++;
				}
			}
			Count++;
			continue;
		}

		if (CheckBBSHElements(Msg, bbs, ForwardingInfo, Msg->via, &HElements[FirstElement], Flood))
		{
			Logprintf(LOG_BBS, '?', "Routing Trace HR %s %s %s %s Matches BBS %s",
				HElements[FirstElement], HElements[FirstElement+1], HElements[FirstElement+2], 
				HElements[FirstElement+3], bbs->Call);
	
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

//	if (strcmp(ATBBS, bbs->Call) == 0)			// @BBS = BBS
//		return TRUE;

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
int CheckBBSHElements(struct MsgInfo * Msg, struct UserInfo * bbs, struct BBSForwardingInfo * ForwardingInfo, char * ATBBS, char ** HElements, int Flood)
{
	char *** HRoutes;
	int i = 0, j, k = 0;
	int bestmatch = 0;

	if (ForwardingInfo->HADDRS)
	{
		// Match on Routes

		HRoutes = ForwardingInfo->HADDRS;
		k=0;

		while(HRoutes[k])
		{
			i = j = 0;
			
			while (HRoutes[k][i] && HElements[j]) // Until one set runs out
			{
				if (strcmp(HRoutes[k][i], HElements[j]) != 0)
					break;
				i++;
				j++;
			}
				
			if (i > bestmatch)

				// if Flooding, only match if all elements match, and elements matching > offset

				if (Flood == 0)
					bestmatch = i;
				else
					if (HElements[j] == 0)
						if (i > ForwardingInfo->HADDROffet[k])
							bestmatch = i;

			k++;
		}
	}
	return bestmatch;
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

#endif

/*
004

Afghanistan

AFG

248

Åland Islands

ALA

008

Albania

ALB

012

Algeria

DZA

016

American Samoa

ASM

020

Andorra

AND

024

Angola

AGO

660

Anguilla

AIA

028

Antigua and Barbuda

ATG

032

Argentina

ARG

051

Armenia

ARM

533

Aruba

ABW

036

Australia

AUS

040

Austria

AUT

031

Azerbaijan

AZE

044

Bahamas

BHS

048

Bahrain

BHR

050

Bangladesh

BGD

052

Barbados

BRB

112

Belarus

BLR

056

Belgium

BEL

084

Belize

BLZ

204

Benin

BEN

060

Bermuda

BMU

064

Bhutan

BTN

068

Bolivia (Plurinational State of)

BOL

070

Bosnia and Herzegovina

BIH

072

Botswana

BWA

076

Brazil

BRA

092

British Virgin Islands

VGB

096

Brunei Darussalam

BRN

100

Bulgaria

BGR

854

Burkina Faso

BFA

108

Burundi

BDI

116

Cambodia

KHM

120

Cameroon

CMR

124

Canada

CAN

132

Cape Verde

CPV

136

Cayman Islands

CYM

140

Central African Republic

CAF

148

Chad

TCD

830

Channel Islands

152

Chile

CHL

156

China

CHN

344

Hong Kong Special Administrative Region of China

HKG

446

Macao Special Administrative Region of China

MAC

170

Colombia

COL

174

Comoros

COM

178

Congo

COG

184

Cook Islands

COK

188

Costa Rica

CRI

384

Côte d'Ivoire

CIV

191

Croatia

HRV

192

Cuba

CUB

196

Cyprus

CYP

203

Czech Republic

CZE

408

Democratic People's Republic of Korea

PRK

180

Democratic Republic of the Congo

COD

208

Denmark

DNK

262

Djibouti

DJI

212

Dominica

DMA

214

Dominican Republic

DOM

218

Ecuador

ECU

818

Egypt

EGY

222

El Salvador

SLV

226

Equatorial Guinea

GNQ

232

Eritrea

ERI

233

Estonia

EST

231

Ethiopia

ETH

234

Faeroe Islands

FRO

238

Falkland Islands (Malvinas)

FLK

242

Fiji

FJI

246

Finland

FIN

250

France

FRA

254

French Guiana

GUF

258

French Polynesia

PYF

266

Gabon

GAB

270

Gambia

GMB

268

Georgia

GEO

276

Germany

DEU

288

Ghana

GHA

292

Gibraltar

GIB

300

Greece

GRC

304

Greenland

GRL

308

Grenada

GRD

312

Guadeloupe

GLP

316

Guam

GUM

320

Guatemala

GTM

831

Guernsey

GGY

324

Guinea

GIN

624

Guinea-Bissau

GNB

328

Guyana

GUY

332

Haiti

HTI

336

Holy See

VAT

340

Honduras

HND

348

Hungary

HUN

352

Iceland

ISL

356

India

IND

360

Indonesia

IDN

364

Iran (Islamic Republic of)

IRN

368

Iraq

IRQ

372

Ireland

IRL

833

Isle of Man

IMN

376

Israel

ISR

380

Italy

ITA

388

Jamaica

JAM

392

Japan

JPN

832
Jersey	JEY
400

Jordan

JOR

398

Kazakhstan

KAZ

404

Kenya

KEN

296

Kiribati

KIR

414

Kuwait

KWT

417

Kyrgyzstan

KGZ

418

Lao People's Democratic Republic

LAO

428

Latvia

LVA

422

Lebanon

LBN

426

Lesotho

LSO

430

Liberia

LBR

434

Libyan Arab Jamahiriya

LBY

438

Liechtenstein

LIE

440

Lithuania

LTU

442

Luxembourg

LUX

450

Madagascar

MDG

454

Malawi

MWI

458

Malaysia

MYS

462

Maldives

MDV

466

Mali

MLI

470

Malta

MLT

584

Marshall Islands

MHL

474

Martinique

MTQ

478

Mauritania

MRT

480

Mauritius

MUS

175

Mayotte	MYT
484

Mexico

MEX

583

Micronesia (Federated States of)

FSM

492

Monaco

MCO

496

Mongolia

MNG

499

Montenegro

MNE

500

Montserrat

MSR

504

Morocco

MAR

508

Mozambique

MOZ

104

Myanmar

MMR

516

Namibia

NAM

520

Nauru

NRU

524

Nepal

NPL

528

Netherlands

NLD

530

Netherlands Antilles

ANT

540

New Caledonia

NCL

554

New Zealand

NZL

558

Nicaragua

NIC

562

Niger

NER

566

Nigeria

NGA

570

Niue

NIU

574

Norfolk Island

NFK

580

Northern Mariana Islands

MNP

578

Norway

NOR

275

Occupied Palestinian Territory

PSE

512

Oman

OMN

586

Pakistan

PAK

585

Palau

PLW

591

Panama

PAN

598

Papua New Guinea

PNG

600

Paraguay

PRY

604

Peru

PER

608

Philippines

PHL

612

Pitcairn

PCN

616

Poland

POL

620

Portugal

PRT

630

Puerto Rico

PRI

634

Qatar

QAT

410

Republic of Korea

KOR

498
Republic of Moldova
MDA
638

Réunion

REU

642

Romania

ROU

643

Russian Federation

RUS

646

Rwanda

RWA

652

Saint-Barthélemy

BLM

654

Saint Helena

SHN

659

Saint Kitts and Nevis

KNA

662

Saint Lucia

LCA

663

Saint-Martin (French part)	MAF
666

Saint Pierre and Miquelon

SPM

670

Saint Vincent and the Grenadines

VCT

882

Samoa

WSM

674

San Marino

SMR

678

Sao Tome and Principe

STP

682

Saudi Arabia

SAU

686

Senegal

SEN

688

Serbia

SRB

690

Seychelles

SYC

694

Sierra Leone

SLE

702

Singapore

SGP

703

Slovakia

SVK

705

Slovenia

SVN

090

Solomon Islands

SLB

706

Somalia

SOM

710

South Africa

ZAF

724

Spain

ESP

144

Sri Lanka

LKA

736

Sudan

SDN

740

Suriname

SUR

744

Svalbard and Jan Mayen Islands

SJM

748

Swaziland

SWZ

752

Sweden

SWE

756

Switzerland

CHE

760

Syrian Arab Republic

SYR

762

Tajikistan

TJK

764

Thailand

THA

807

The former Yugoslav Republic of Macedonia

MKD

626

Timor-Leste

TLS

768

Togo

TGO

772

Tokelau

TKL

776

Tonga

TON

780

Trinidad and Tobago

TTO

788

Tunisia

TUN

792

Turkey

TUR

795

Turkmenistan

TKM

796

Turks and Caicos Islands

TCA

798

Tuvalu

TUV

800

Uganda

UGA

804

Ukraine

UKR

784

United Arab Emirates

ARE

826

United Kingdom of Great Britain and Northern Ireland

GBR

834

United Republic of Tanzania

TZA

840

United States of America

USA

850

United States Virgin Islands

VIR

858

Uruguay

URY

860

Uzbekistan

UZB

548

Vanuatu

VUT

862

Venezuela (Bolivarian Republic of)

VEN

704

Viet Nam

VNM

876

Wallis and Futuna Islands

WLF

732

Western Sahara

ESH

887

Yemen

YEM

894

Zambia

ZMB

716

Zimbabwe

ZWE

*/