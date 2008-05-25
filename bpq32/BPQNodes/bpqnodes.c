
#include <stdio.h>

__declspec( dllimport ) int __stdcall SaveNodes ();

main ()
{
	printf("BPQ32 Save Node Tables Version 1.0 November 2006\n\n");

	SaveNodes();

	printf("Save Nodes Complete\n");

	return 0;

}
