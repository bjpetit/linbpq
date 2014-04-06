#	LinBPQ Makefile

#	To exclude i2c support add -DNOI2C to CFLAGS 

OBJS = IPCode.o FLDigi.o linether.o TNCEmulators.o CMSAuth.o APRSCode.o BPQtoAGW.o KAMPactor.o AEAPactor.o HALDriver.o MULTIPSK.o BBSHTMLConfig.o ChatHTMLConfig.o HTMLCommonCode.o BBSUtilities.o bpqaxip.o BPQINP3.o BPQNRR.o cMain.o Cmd.o CommonCode.o compatbits.o config.o datadefs.o FBBRoutines.o HFCommon.o Housekeeping.o HTTPcode.o kiss.o L2Code.o L3Code.o L4Code.o lzhuf32.o MailCommands.o MailDataDefs.o MailNode.o MailRouting.o MailTCP.o MBLRoutines.o md5.o Moncode.o NNTPRoutines.o i2cdummy.o RigControl.o TelnetV6.o WINMOR.o TNCCode.o UZ7HODrv.o WPRoutines.o SCSTrackeMulti.o SCSPactor.o SCSTracker.o HanksRT.o  UIRoutines.o AGWAPI.o AGWMoncode.o

# Configuration:

#CFLAGS = -DLINBPQ -MMD -g
CFLAGS = -DLINBPQ -MMD -DNOI2C -g 

CC = gcc

vpath %.c ./
vpath %.h ./
vpath %.o ./
			                       
all: linbpq
			
linbpq: $(OBJS)
	gcc $(OBJS) -Xlinker -Map=output.map -lrt -lm -lpthread /usr/lib/libconfig.a -o linbpq
	sudo setcap "CAP_NET_ADMIN=ep CAP_NET_RAW=ep CAP_NET_BIND_SERVICE=ep" linbpq		

-include *.d

clean :
	rm linbpq $(OBJS)

