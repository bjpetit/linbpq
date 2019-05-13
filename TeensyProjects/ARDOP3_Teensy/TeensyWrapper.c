//
//	Makes keeping Windows, Linux and Teensy version in step easier

#define TEENSY
#define ARDOP
#define PTC

#include "TeensyConfig.h"

#include "../../ARDOP3/ARDOPC.c"
#include "../../ARDOP3/ardopSampleArrays.c"
#include "../../ARDOP3/ARQ.c"
#include "../../ARDOP3/berlekamp.c"
#include "../../ARDOP3/BusyDetect.c"
#include "../../ARDOP3/FEC.c"
#include "../../ARDOP3/FFT.c"
#include "../../ARDOP3/KISSModule.c"
#include "../../ARDOP3/galois.c"
#include "../../ARDOP3/HostInterface.c"
#include "../../ARDOP3/Modulate.c"
#include "../../ARDOP3/rs.c"
#include "../../ARDOP3/SCSHostInterface.c"
#include "../../ARDOP3/SoundInput.c"
//#include "../../ARDOP3/direwolf/demod_afsk.c"
//#include "../../ARDOP3/direwolf/dsp.c"
//#include "../../ARDOP3/direwolf/hdlc_rec.c"
#include "../../ARDOP3/afskModule.c"
//#include "../../ARDOP3/costab.c"
//#include "../../ARDOP3/modem.c"
#include "../../ARDOP3/pktARDOP.c"
#include "../../ARDOP3/pktSession.c"
#include "../../ARDOP3/Viterbi.c"
#include "../../ARDOP3/ackbycarrier.c"
