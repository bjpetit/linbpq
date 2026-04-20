#	LinBPQ Makefile

#	To exclude i2c support run make noi2c

OBJS = pngwtran.o pngrtran.o pngset.o pngrio.o pngwio.o pngtrans.o pngrutil.o pngwutil.o\
 pngread.o pngwrite.o png.o pngerror.o pngget.o pngmem.o APRSIconData.o AISCommon.o\
 upnp.o APRSStdPages.o HSMODEM.o WinRPR.o KISSHF.o TNCEmulators.o bpqhdlc.o SerialPort.o\
 adif.o WebMail.o utf8Routines.o VARA.o LzFind.o Alloc.o LzmaDec.o LzmaEnc.o LzmaLib.o \
 Multicast.o ARDOP.o IPCode.o FLDigi.o linether.o CMSAuth.o APRSCode.o BPQtoAGW.o KAMPactor.o\
 AEAPactor.o HALDriver.o MULTIPSK.o BBSHTMLConfig.o ChatHTMLConfig.o BBSUtilities.o bpqaxip.o\
 BPQINP3.o BPQNRR.o cMain.o Cmd.o CommonCode.o HTMLCommonCode.o compatbits.o config.o datadefs.o \
 FBBRoutines.o HFCommon.o Housekeeping.o HTTPcode.o kiss.o L2Code.o L3Code.o L4Code.o lzhuf32.o \
 MailCommands.o MailDataDefs.o LinBPQ.o MailRouting.o MailTCP.o MBLRoutines.o md5.o Moncode.o \
 NNTPRoutines.o RigControl.o TelnetV6.o WINMOR.o TNCCode.o UZ7HODrv.o WPRoutines.o \
 SCSTrackeMulti.o SCSPactor.o SCSTracker.o HanksRT.o  UIRoutines.o AGWAPI.o AGWMoncode.o \
 DRATS.o FreeDATA.o base64.o Events.o nodeapi.o mailapi.o mqtt.o RHP.o NETROMTCP.o

# Configuration:

#Default to Linux
	CC ?= gcc    
	LDFLAGS ?= -Xlinker -Map=output.map -lrt 
	BASE_CFLAGS ?= -DLINBPQ -MMD -fcommon -fasynchronous-unwind-tables
	DEBUG_FLAGS ?= -g
	OPT_FLAGS ?=
	WARN_FLAGS ?=
	SAN_FLAGS ?=

.PHONY: all debug release sanitize strict help nomqtt noi2c setcap clean

all: CFLAGS += $(BASE_CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(SAN_FLAGS) $(EXTRA_CFLAGS)	
all: LDLIBS += -lpaho-mqtt3a -ljansson -lminiupnpc -lm -lz -lpthread -lconfig -lpcap                       
all: linbpq

#other OS

OS_NAME = $(shell uname -s)
ifeq ($(OS_NAME),NetBSD)
	CC = cc
    EXTRA_CFLAGS = -DFREEBSD -DNOMQTT -I/usr/pkg/include 
    LDFLAGS =  -Xlinker -Map=output.map -Wl,-R/usr/pkg/lib -L/usr/pkg/lib -lrt -lutil -lexecinfo

all: CFLAGS += $(BASE_CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(SAN_FLAGS) $(EXTRA_CFLAGS)	
all: LDLIBS += -lminiupnpc -lm -lz -lpthread -lconfig -lpcap                    
all: linbpq


endif
ifeq ($(OS_NAME),FreeBSD)
    CC = cc
    EXTRA_CFLAGS = -DFREEBSD -DNOMQTT -I/usr/local/include
    LDFLAGS = -Xlinker -Map=output.map -L/usr/local/lib -lrt -liconv -lutil -lexecinfo

all: CFLAGS += $(BASE_CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(SAN_FLAGS) $(EXTRA_CFLAGS)	
all: LDLIBS +=  -lminiupnpc -lm -lz -lpthread -lconfig -lpcap	                       
all: linbpq

endif

ifeq ($(OS_NAME),Darwin)
	CC = gcc	                       
	EXTRA_CFLAGS = -DMACBPQ -DNOMQTT 
	LDFLAGS = -liconv

all: CFLAGS += $(BASE_CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(SAN_FLAGS) $(EXTRA_CFLAGS)	
all: LDLIBS += -lminiupnpc -lm -lz -lpthread -lconfig -lpcap                     
all: linbpq
endif

$(info OS_NAME is $(OS_NAME))



debug: DEBUG_FLAGS = -g3 -O0
debug: OPT_FLAGS =
debug: SAN_FLAGS =
debug: all

release: DEBUG_FLAGS =
release: OPT_FLAGS = -O2 -DNDEBUG
release: SAN_FLAGS =
release: all

sanitize: DEBUG_FLAGS = -g
sanitize: OPT_FLAGS = -O1 -fno-omit-frame-pointer
sanitize: SAN_FLAGS = -fsanitize=address,undefined
sanitize: LDFLAGS += -fsanitize=address,undefined
sanitize: all

strict: WARN_FLAGS = -Wall -Wextra -Wformat=2 -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -Werror
strict: DEBUG_FLAGS = -g
strict: OPT_FLAGS = -O1
strict: SAN_FLAGS =
strict: all

nomqtt: CFLAGS += $(BASE_CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(SAN_FLAGS) $(EXTRA_CFLAGS) -rdynamic -DNOMQTT
nomqtt: LDLIBS += -lminiupnpc -lm -lz -lpthread -lconfig -lpcap   
nomqtt: linbpq

noi2c: CFLAGS += $(BASE_CFLAGS) $(DEBUG_FLAGS) $(OPT_FLAGS) $(WARN_FLAGS) $(SAN_FLAGS) $(EXTRA_CFLAGS) -DNOI2C -rdynamic
noi2c: LDLIBS += -lpaho-mqtt3a -ljansson -lminiupnpc -lm -lz -lpthread -lconfig -lpcap                        
noi2c: linbpq


linbpq: $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -o linbpq

setcap: all
	sudo setcap "CAP_NET_ADMIN=ep CAP_NET_RAW=ep CAP_NET_BIND_SERVICE=ep" linbpq		

help:
	@echo "LinBPQ build targets:"
	@echo "  all       - default Linux/OS build flags"
	@echo "  debug     - debug symbols, no optimization"
	@echo "  release   - optimized build with -DNDEBUG"
	@echo "  sanitize  - ASan+UBSan build"
	@echo "  strict    - warnings-as-errors build profile"
	@echo "  nomqtt    - build without MQTT"
	@echo "  noi2c     - build without i2c support"
	@echo "  setcap    - apply Linux capabilities to linbpq"
	@echo "  clean     - remove binary, objects, dep files"
	@echo "Options:"
	@echo "  USE_DEPS=1   include generated .d files"
	@echo "  CC=clang     use a different compiler"

USE_DEPS ?= 0
ifneq ($(USE_DEPS),0)
DEPFILES = $(patsubst %.o,%.d,$(wildcard $(OBJS)))
-include $(DEPFILES)
endif

clean :
	rm -f *.d
	rm -f linbpq $(OBJS)

