// Project specific configuration for Teensy

// At the moment only Packet, ARDOP or SOUNDCARD

// These tests assume a modified platform.txt
// If you haven't done that you will need to define ARDOP PACKET or SOUNDCARD yourself

#ifdef PROJECT_SoundCard  
#define SOUNDCARD
#endif

#ifdef PROJECT_ARDOP_Teensy
#define ARDOP
#endif

#ifdef PROJECT_ARDOP2_Teensy
#define ARDOP
#endif

#ifdef PROJECT_ARDOPOFDM_Teensy
#define ARDOP
#define _OFDM
#endif

#ifdef PROJECT_SM_Teensy
#define PACKET
#endif

#ifdef PROJECT_ARDOPWithPacket
#define ARDOP
#endif

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

