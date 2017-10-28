// Project specific configuration for Teensy

// At the moment only Packet, ARDOP or SOUNDCARD

//#define PACKET
#define ARDOP

//#define SOUNDCARD
// Standard definitions

#define TEENSY

#ifdef ARDOP
#include "TeensyConfigARDOP.h"
#endif

#ifdef PACKET
#include "TeensyConfigPacket.h"
#endif

#ifdef SOUNDCARD
#include "TeensyConfigSoundCard.h"
#endif

