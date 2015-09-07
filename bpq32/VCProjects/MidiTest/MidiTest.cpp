// MidiTest.cpp : Defines the entry point for the console application.
//


/* For Neil's system, we need UA-FX4 as Input and LoopBE as output.

His Midi app then uses LoopBe as input

Device Numbers are on command line 
*/

#include "stdafx.h"

UINT sendMIDIEvent(HMIDIOUT hmo, BYTE bStatus, BYTE bData1, BYTE bData2) ;

int inCount, outCount, ret;

MIDIINCAPS MidiInCaps;
MIDIOUTCAPS MidiOutCaps;

HMIDIIN hMidiIn;
HMIDIOUT hMidiOut;

BYTE P1 = 0, P2 = 0, P3 = 0;
BOOL Flag = FALSE;

int Invert[120];


void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch(wMsg) {
	case MIM_OPEN:
		printf("wMsg=MIM_OPEN\n");
		break;
	case MIM_CLOSE:
		printf("wMsg=MIM_CLOSE\n");
		break;
	case MIM_DATA:

		if (dwParam1 != 0xfe)		// Ignore Keepalives
		{
//			printf("wMsg=MIM_DATA, dwInstance=%08x, dwParam1=%08x, dwParam2=%08x\n", dwInstance, dwParam1, dwParam2);
			P1 = (BYTE)dwParam1 & 255;
			P2 = (BYTE)(dwParam1 >> 8) & 255;
//			printf("%d > %d\n", P2, Invert[P2]);
			
			//	Reverse Key down or Key up events

			if ((dwParam1 & 0xff) == 0x80 || (dwParam1 & 0xff) == 0x90 )
				P2 = Invert[P2];

			P3 = (BYTE)(dwParam1 >> 16) & 255;

//			if (P3)
//				P3 = 127;

			//	MS say you shouldn't do this, but it seems to work and makes the code
			//	much simpler and faster

			ret = sendMIDIEvent(hMidiOut, P1, P2, P3);

//			Flag = TRUE;
		}
		break;
	case MIM_LONGDATA:
		printf("wMsg=MIM_LONGDATA\n"); 
		break;
	case MIM_ERROR:
		printf("wMsg=MIM_ERROR\n");
		break;
	case MIM_LONGERROR:
		printf("wMsg=MIM_LONGERROR\n");
		break;
	case MIM_MOREDATA:
		printf("wMsg=MIM_MOREDATA\n");
		break;
	default:
		printf("wMsg = unknown\n");
		break;
	}
}

int main(int argc, CHAR* argv[])
{
	int i, InDevice = -1, OutDevice = -1;
	
	for (i = 0; i < 120; i++)
		Invert[i] = 124 - i;

//	if (argc > 1)
//		InDevice = atoi(argv[1]);

//	if (argc > 2)
//		OutDevice = atoi(argv[2]);

	inCount = midiInGetNumDevs();
	outCount = midiOutGetNumDevs();

	printf("Input Devices\n");
	for (i = 0; i < inCount; i++)
	{
		ret = midiInGetDevCaps(i, &MidiInCaps, sizeof(MidiInCaps));

		if (strstr(MidiInCaps.szPname, "UA"))
			InDevice = i;

		if (i == InDevice)
			printf("*%s\n", &MidiInCaps.szPname);
		else
			printf("%s\n", &MidiInCaps.szPname);
	}

	printf("\nOutput Devices\n");

	for (i = 0; i < outCount; i++)
	{
		ret = midiOutGetDevCaps(i, &MidiOutCaps, sizeof(MidiOutCaps));

		if (strstr(MidiOutCaps.szPname, "LoopBe"))
			OutDevice = i;

		if (i == OutDevice)
			printf("*%s\n", &MidiOutCaps.szPname);
		else
			printf("%s\n", &MidiOutCaps.szPname);
	}

	printf("\n");

	ret = midiInOpen(&hMidiIn, InDevice,  (DWORD)(void*)MidiInProc, NULL, CALLBACK_FUNCTION);

	ret = midiOutOpen(&hMidiOut, OutDevice, NULL, NULL, CALLBACK_NULL);

	ret = midiOutSetVolume(hMidiOut, 0xffffffff);

	ret = midiInStart(hMidiIn);

	while (1)
	{
		//	This is only needed if we find we cant call sendMIDIEvent from the input callback

		if (Flag)
		{
			ret = sendMIDIEvent(hMidiOut, P1, P2, P3);
			Flag = FALSE;
			Sleep (10);
		}
		Sleep (1000);
	}
	return 0;
}

UINT sendMIDIEvent(HMIDIOUT hmo, BYTE bStatus, BYTE bData1, BYTE bData2) 
{ 
    union { 
        DWORD dwData; 
        BYTE bData[4]; 
    } u; 
 
    // Construct the MIDI message. 
 
    u.bData[0] = bStatus;  // MIDI status byte 
    u.bData[1] = bData1;   // first MIDI data byte 
    u.bData[2] = bData2;   // second MIDI data byte 
    u.bData[3] = 0; 
 
    // Send the message. 
    return midiOutShortMsg(hmo, u.dwData); 
} 


