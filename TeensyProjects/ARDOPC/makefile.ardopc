#	ARDOPC Makefile

OBJS = KISSModule.o pktARDOP.o BusyDetect.o i2cDisplay.o ALSASound.o ARDOPC.o ardopSampleArrays.o ARQ.o FFT.o FEC.o HostInterface.o Modulate.o rs.o berlekamp.o galois.o SoundInput.o TCPHostInterface.o

# Configuration:
CFLAGS = -DLINBPQ -MMD -g 
CC = gcc

vpath %.c /mnt/ardopc
vpath %.h /mnt/ardopc
vpath %.o ./
			                       
all: ardopc
			
ardopc: $(OBJS)
	gcc $(OBJS) -Xlinker -Map=output.map -lrt -lm -lpthread -lasound -o ardopc


-include *.d

clean :
	rm ardopc $(OBJS)

