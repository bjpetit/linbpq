/*
 * Copyright (c) 1999 - 2003
 * NetGroup, Politecnico di Torino (Italy)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright 
 * notice, this list of conditions and the following disclaimer in the 
 * documentation and/or other materials provided with the distribution. 
 * 3. Neither the name of the Politecnico di Torino nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */


#include "pcap.h"
#include "windows.h"
#include "Iphlpapi.h"

main()
{
	pcap_if_t *alldevs;
	pcap_if_t *d;
	int i=0;
	char errbuf[PCAP_ERRBUF_SIZE];
	
	/* Retrieve the device list */
	if(pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}
	
	/* Print the list */
	for(d=alldevs; d; d=d->next)
	{
		printf("%d. %s", ++i, d->name);
		if (d->description)
			printf(" (%s)\n", d->description);
		else
			printf(" (No description available)\n");
	}

	if(i==0)
	{
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n");
	}

	xx();

	printf("Press any key to Exit");
	i = _getch();


	return 0;
}
// It is possible for an adapter to have multiple
// IPv4 addresses, gateways, and secondary WINS servers
// assigned to the adapter. 
// Note that this sample code only prints out the 
// first entry for the IP address/mask, gateway,
// and secondary WINS server for each adapter. 

PIP_ADAPTER_INFO pAdapterInfo;
PIP_ADAPTER_INFO pAdapter = NULL;
DWORD dwRetVal = 0;

int xx()
{
	UINT i, ulOutBufLen;

pAdapterInfo = (IP_ADAPTER_INFO *) malloc( sizeof(IP_ADAPTER_INFO) );
ulOutBufLen = sizeof(IP_ADAPTER_INFO);

// Make an initial call to GetAdaptersInfo to get
// the necessary size into the ulOutBufLen variable
if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) != ERROR_SUCCESS) {
  GlobalFree (pAdapterInfo);
  pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen);
}

if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
  pAdapter = pAdapterInfo;
  while (pAdapter) {
    printf("\tAdapter Name: \t%s\n", pAdapter->AdapterName);
    printf("\tAdapter Desc: \t%s\n", pAdapter->Description);
    printf("\tAdapter Addr: \t");
    for (i = 0; i < pAdapter->AddressLength; i++)
        printf("%x%c", pAdapter->Address[i], 
            i == pAdapter->AddressLength - 1 ? '\n' : '-');
    printf("\tIP Address: \t%s\n", pAdapter->IpAddressList.IpAddress.String);
    printf("\tIP Mask: \t%s\n", pAdapter->IpAddressList.IpMask.String);

    printf("\tGateway: \t%s\n", pAdapter->GatewayList.IpAddress.String);
    printf("\t***\n");
    if (pAdapter->DhcpEnabled) {
      printf("\tDHCP Enabled: Yes\n");
      printf("\t\tDHCP Server: \t%s\n", pAdapter->DhcpServer.IpAddress.String);
      printf("\tLease Obtained: %ld\n", pAdapter->LeaseObtained);
    }
    else
      printf("\tDHCP Enabled: No\n");
    
    if (pAdapter->HaveWins) {
      printf("\tHave Wins: Yes\n");
      printf("\t\tPrimary Wins Server: \t%s\n", pAdapter->PrimaryWinsServer.IpAddress.String);
      printf("\t\tSecondary Wins Server: \t%s\n", pAdapter->SecondaryWinsServer.IpAddress.String);
    }
    else
      printf("\tHave Wins: No\n");

    pAdapter = pAdapter->Next;
  }
}
else {
  printf("Call to GetAdaptersInfo failed.\n");
}
return 0;
}