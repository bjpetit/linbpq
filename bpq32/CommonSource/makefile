#	LinBPQ Makefile

OBJS = linether.o TNCEmulators.o CMSAuth.o APRSCode.o BPQtoAGW.o KAMPactor.o AEAPactor.o HALDriver.o MULTIPSK.o BBSHTMLConfig.o ChatHTMLConfig.o BBSUtilities.o bpqaxip.o BPQINP3.o BPQNRR.o cMain.o Cmd.o CommonCode.o compatbits.o config.o datadefs.o FBBRoutines.o HFCommon.o Housekeeping.o HTTPcode.o kiss.o L2Code.o L3Code.o L4Code.o lzhuf32.o MailCommands.o MailDataDefs.o MailNode.o MailRouting.o MailTCP.o MBLRoutines.o md5.o Moncode.o NNTPRoutines.o i2cdummy.o RigControl.o TelnetV6.o WINMOR.o TNCCode.o UZ7HODrv.o WPRoutines.o SCSTrackeMulti.o SCSPactor.o SCSTracker.o HanksRT.o  UIRoutines.o 
# Configuration:
CFLAGS = -DLINBPQ -g 
CC = gcc
			
all: linbpq
			
linbpq: $(OBJS)
	gcc $(OBJS) -Xlinker -Map=output.map -lrt -lm /usr/lib/libconfig.a -o linbpq
	sudo setcap "CAP_NET_RAW=ep CAP_NET_BIND_SERVICE=ep" linbpq	

APRSCode.o: CHeaders.h
APRSCode.o: compatbits.h asmstrucs.h bpq32.h kernelresource.h
BBSHTMLConfig.o: compatbits.h bpq32.h
BBSHTMLConfig.o: asmstrucs.h BPQMailChat.h
# BPQMailChat.h includes:
#	resource.h
BPQtoAGW.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
ChatHTMLConfig.o: compatbits.h bpq32.h
ChatHTMLConfig.o: asmstrucs.h BPQMailChat.h
BBSUtilities.o: compatbits.h bpq32.h
BBSUtilities.o: asmstrucs.h BPQMailChat.h
bpqaxip.o: CHeaders.h compatbits.h asmstrucs.h bpq32.h
BPQINP3.o: CHeaders.h compatbits.h asmstrucs.h
BPQMailConfig.o: compatbits.h bpq32.h asmstrucs.h BPQMailChat.h 
BPQNRR.o: CHeaders.h compatbits.h asmstrucs.h
cMain.o: kernelresource.h CHeaders.h compatbits.h asmstrucs.h configstructs.h
Cmd.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h telnetserver.h
CommonCode.o: CHeaders.h compatbits.h
CommonCode.o: asmstrucs.h tncinfo.h
CommonCode.o: kernelresource.h RigControl.h
config.o: CHeaders.h compatbits.h asmstrucs.h
config.o: configstructs.h
datadefs.o: compatbits.h asmstrucs.h
FBBRoutines.o: compatbits.h bpq32.h
FBBRoutines.o: asmstrucs.h BPQMailChat.h
HALDriver.o:  CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
HanksRT.o: compatbits.h bpq32.h
HanksRT.o: asmstrucs.h bpqchat.h
ChatHTMLConfig.o: bpqchat.h
HFCommon.o: CHeaders.h
HFCommon.o: compatbits.h asmstrucs.h
HFCommon.o: tncinfo.h RigControl.h
HFCommon.o: bpq32.h
Housekeeping.o: compatbits.h bpq32.h
Housekeeping.o: asmstrucs.h BPQMailChat.h
HTTPcode.o: CHeaders.h compatbits.h
HTTPcode.o: asmstrucs.h tncinfo.h
HTTPcode.o: kernelresource.h RigControl.h
HTTPcode.o: bpq32.h
HTTPcode.o: telnetserver.h 
i2cdummy.o: CHeaders.h compatbits.h
i2cdummy.o: asmstrucs.h
AEAPactor.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
KAMPactor.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
HALDRIVER.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
MULTIPSK.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
kiss.o: CHeaders.h compatbits.h asmstrucs.h
kiss.o: kiss.h
L2Code.o: CHeaders.h compatbits.h asmstrucs.h
L3Code.o: CHeaders.h compatbits.h asmstrucs.h
L4Code.o: CHeaders.h compatbits.h asmstrucs.h
LINBPQ.o: CHeaders.h compatbits.h asmstrucs.h
LINBPQ.o: Versions.h
lzhuf32.o: compatbits.h bpq32.h
lzhuf32.o: asmstrucs.h BPQMailChat.h
MailCommands.o: compatbits.h bpq32.h
MailCommands.o: asmstrucs.h BPQMailChat.h
MailDataDefs.o: compatbits.h bpq32.h
MailDataDefs.o: asmstrucs.h BPQMailChat.h
MailNode.o: CHeaders.h compatbits.h
MailNode.o: asmstrucs.h BPQMailChat.h
MailNode.o: Versions.h
MailRouting.o: compatbits.h bpq32.h
MailRouting.o: asmstrucs.h BPQMailChat.h

MailTCP.o: compatbits.h bpq32.h
MailTCP.o: asmstrucs.h BPQMailChat.h
MBLRoutines.o: compatbits.h bpq32.h
MBLRoutines.o: asmstrucs.h BPQMailChat.h md5.h
Moncode.o: CHeaders.h compatbits.h
Moncode.o: asmstrucs.h tncinfo.h RigControl.h
NNTPRoutines.o: compatbits.h bpq32.h asmstrucs.h BPQMailChat.h
RigControl.o: CHeaders.h compatbits.h
RigControl.o: asmstrucs.h tncinfo.h
RigControl.o: RigControl.h
RigControl.o: bpq32.h
SCSPactor.o:  CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
SCSTrackeMulti.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
SCSTracker.o:  CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
TelnetV6.o: kernelresource.h CHeaders.h
TelnetV6.o: compatbits.h asmstrucs.h
TelnetV6.o: tncinfo.h telnetserver.h
TNCCode.o: CHeaders.h compatbits.h
TNCCode.o: asmstrucs.h
UIRoutines.o: compatbits.h bpq32.h
UIRoutines.o: asmstrucs.h BPQMailChat.h
UZ7HODrv.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
V4.o:  CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
WINMOR.o: CHeaders.h compatbits.h asmstrucs.h tncinfo.h RigControl.h bpq32.h
WPRoutines.o: compatbits.h bpq32.h asmstrucs.h BPQMailChat.h


