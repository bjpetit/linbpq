// Project specific configuration for Teensy

// At the moment only Packet or ARDOP 

//#define PACKET
#define ARDOP

// Standard definitions

#define TEENSY

#ifdef ARDOP
#include "TeensyConfigARDOP.h"
#endif

#ifdef PACKET
#include "TeensyConfigPacket.h"
#endif
