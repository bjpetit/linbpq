/*
 * 1.00 xx/08/2012 John Wiseman G8BPQ
*/


#define VERSION "0.0.0.1"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>

#include <linux/i2c-dev.h>


#define G8BPQ_CRC	1

#define	SIZE		4096

#define FEND		0300	/* Frame End			(0xC0)	*/
#define FESC		0333	/* Frame Escape			(0xDB)	*/
#define TFEND		0334	/* Transposed Frame End		(0xDC)	*/
#define TFESC		0335	/* Transposed Frame Escape	(0xDD)	*/

#define TRUE 1
#define FALSE 0



/*
 * Keep these off the stack.
 */
 
 
static int address;
static char progname[80];

static unsigned char ibuf[SIZE];	/* buffer for input operations	*/

static char *usage_string	= "usage: pitnc_getparams i2bus i2cdevice\n";

int fd;
FILE * file;

int i2c = 0;

#define HANDLE int

int main(int argc, char *argv[])
{
	unsigned char *icp;
	int retval, size, len;
    char i2cname [20];
	int fends = 0;
    int count = 0;
	int bus;    
	int ptr = 0;
	int sum = 0;
	
	while ((size = getopt(argc, argv, ":v")) != -1) 
	{
		switch (size) 
		{
	
		case 'v':
			printf("%s\n", VERSION);
			return 1;
		case ':':
		case '?':
			fprintf(stderr, usage_string);
			return 1;
		}
	}

	// Allow 2 params 
	
	if ((argc - optind) != 2 )
	{
		fprintf(stderr, usage_string);
		return 1;
	}
	
	bus = strtol(argv[optind], 0, 0);
	address = strtol(argv[optind + 1], 0, 0);

	if (address > 0)
		i2c = TRUE;	

	if (i2c)
	{
		sprintf(i2cname, "/dev/i2c-%s", argv[optind]);
		fd = open(i2cname, O_RDWR);
                                              
		if (fd < 0)
		{
			fprintf(stderr, "%s: Cannot find i2c bus %s\n", progname, i2cname);
			exit(1);
		}
	
 
 		retval = ioctl(fd,  I2C_SLAVE, address);	
		if(retval == -1)
		{
			fprintf(stderr, "%s: Cannot open i2c device %x\n", progname, address);
			exit (1);
		}
	
	}
	else
	{
		fd = OpenCOMPort("/dev/ttyAMA0", 19200);
	}
	
	usleep(100000);
	
	// Send request for params
	
	if (i2c)
	{
		i2c_smbus_write_byte(fd, FEND);
		i2c_smbus_write_byte(fd, FEND);
        i2c_smbus_write_byte(fd, 15); 
		i2c_smbus_write_byte(fd, 1);
		i2c_smbus_write_byte(fd, FEND);
		
	}
	else
	{
		unsigned char Block[5];
		
		while (ReadCOMBlock(fd, Block, 1))
		{
			printf("%x \n", Block[0]);
		}
		
		Block[0] = FEND;
		Block[1] = FEND;
		Block[2] = 15;
		Block[3] = 1;
		Block[4] = FEND;
		
		write(fd, Block, 4);
	}
	
	while (TRUE)
	{
		if (i2c)
			retval = i2c_smbus_read_byte(fd);
		else
		{
			unsigned char Block[4];
			int n = ReadCOMBlock(fd, Block, 1);
			
			if (n == 0)
				goto nochar;
				
			retval = Block[0];
		}	
	
		if (retval == -1)	 		// Read failed		
  		{
			perror("poll failed");	 	
			exit(0);
		}
	
		
		if (retval == 0x0e)
			continue;
		
		ibuf[ptr++] = retval;
		sum ^= retval;
		
		
		if (ptr >= SIZE)
			ptr = SIZE;
			
			
		if (retval == FEND)
		{
			fends++;
			
			if (fends == 1)
				sum = ptr = 0;
				
			if (fends == 2)
			{
				int i;
				
				sum ^= FEND;
				if (sum != 0)
					printf("** Checksum Error - Sum = %x Should be Zero\n\n", sum);
					
				printf("\n");
				printf("01 TXDelay - Zero means use ADC %3d\n", ibuf[2]);
				printf("02 Persistance                  %3d\n", ibuf[3]);
				printf("03 Slottime (in 10 mS)          %3d\n", ibuf[4]);
				printf("04 TXTail                       %3d\n", ibuf[5]);
				printf("05 Full Duplex - Not used       %3d\n", ibuf[6]);
				printf("06 Our Channel (Hex)             %02x\n", ibuf[7]);
				printf("07 I2C Address (0 = async) Hex   %02x\n", ibuf[8]);
				printf("   ADC Value                    %3d\n", ibuf[9]);
				
				
				for (i = 0; i < ptr; i++)
					printf("%x ", ibuf[i]);	
							
				printf("sum %x \n", sum);
				exit(0);

			}
		}
nochar:		
		usleep(10000);
		count++;
		
		if (count > 250)
		{
			printf("retrying...\n");
			count = 0;
	
			if (i2c)
			{
				i2c_smbus_write_byte(fd, FEND);
				i2c_smbus_write_byte(fd, FEND);
       	 		i2c_smbus_write_byte(fd, 15); 
				i2c_smbus_write_byte(fd, 1);
				i2c_smbus_write_byte(fd, FEND);
		
			}
			else
			{

				unsigned char Block[5];
				
				Block[0] = FEND;
				Block[1] = FEND;
				Block[2] = 15;
				Block[3] = 1;
				Block[4] = FEND;
		
				write(fd, Block, 4);;
			}
		}
	}

	return 1;
}

static struct speed_struct
{
	int	user_speed;
	speed_t termios_speed;
} speed_table[] = {
	{300,         B300},
	{600,         B600},
	{1200,        B1200},
	{2400,        B2400},
	{4800,        B4800},
	{9600,        B9600},
	{19200,       B19200},
	{38400,       B38400},
	{57600,       B57600},
	{115200,      B115200},
	{-1,          B0}
};
 

HANDLE OpenCOMPort(char * Port, int speed)
{
	char buf[100];

	//	Linux Version.

	int fd;
	int hwflag = 0;
	u_long param=1;
	struct termios term;
	struct speed_struct *s;


	if ((fd = open(Port, O_RDWR | O_NDELAY)) == -1)
	{
		perror("Com Open Failed");
		exit (0);
	}

	// Validate Speed Param

	for (s = speed_table; s->user_speed != -1; s++)      
		if (s->user_speed == speed)
			break;

   if (s->user_speed == -1)
   {
	   fprintf(stderr, "tty_speed: invalid speed %d\n", speed);
	   exit (0);
   }
   
   if (tcgetattr(fd, &term) == -1)
   {
	   perror("tty_speed: tcgetattr");
	   exit (0);
   }

   	cfmakeraw(&term);
	cfsetispeed(&term, s->termios_speed);
	cfsetospeed(&term, s->termios_speed);

	if (tcsetattr(fd, TCSANOW, &term) == -1)
	{
		perror("tty_speed: tcsetattr");
		return FALSE;
	}

	ioctl(fd, FIONBIO, &param);

	return fd;
}

int ReadCOMBlock(HANDLE fd, char * Block, int MaxLength)
{
	int Length;
	
	Length = read(fd, Block, MaxLength);

	if (Length < 0)
	{
		if	(errno != 11)					// Would Block
		{
			perror("read");
			printf("%d\n", errno);
		}
		return 0;
	}

	return Length;
}

