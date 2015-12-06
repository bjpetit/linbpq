// ARDOP TNC Host Interface
//

#include "ARDOPC.h"

void QueueCommandToHost(char * Cmd)
{
	printf("Command to Host %s\n", Cmd);
}



void AddTagToDataAndSendToHost(char * Msg, char * Type, int Len)
{
	Msg[Len] = 0;
	printf("RX Data %s %s\n", Type, Msg);
}


