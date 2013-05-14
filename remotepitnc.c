/*
 * repotepitnc.c
 *
 * code to allow a tnc connected to the i2c bus to be accesses orver a udp link from a BPQ32 Node


 * Based loosely on mkiss from the ax-utils package
*/

#include <syslog.h> 
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
#include <netax25/daemon.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/syslog.h>
#include <linux/i2c-dev.h>
#include <signal.h>
#include <sys/select.h>
#include <getopt.h>
#include <sys/time.h>

#define FEND 0xc0
#define FESC		0333	/* Frame Escape			(0xDB)	*/
#define TFEND		0334	/* Transposed Frame End		(0xDC)	*/
#define TFESC		0335	/* Transposed Frame Escape	(0xDD)	*/

#define G8BPQ_CRC 1
#define ACKREQ 0x0c

#define SIZE 4096

/*
 * Keep these off the stack.
 */
 
#define VERSION "1"
 
static int address;
static char progname[80];

static unsigned char ibuf[SIZE];	/* buffer for input operations	*/
static unsigned char obuf[SIZE];	/* buffer for kiss_tx()		*/

static int crc_errors		= 0;
static int invalid_ports	= 0;
static int return_polls		= 0;

static char *usage_string	= "usage: remotepitnc [-p interval] [-l] [-v] i2bus i2cdevice udpport\n";

static int dump_report		= FALSE;

static int logging = TRUE;   // = FALSE;
static int crcflag = G8BPQ_CRC;			// Always use G8BPQ CRC
static int pollspeed = 1;				// Need to Poll

struct sockaddr_storage peer_addr;
socklen_t peer_addr_len;

static struct iface *pty;
static struct iface *tty;

struct iface
{
	char	*name;				/* Interface name (/dev/???)	*/
	int		device;				/* i2c device */
	int		fd;					/* File descriptor		*/
	int		escaped;			/* FESC received?		*/
	unsigned char	crc;		/* Incoming frame crc		*/
	unsigned char	obuf[SIZE];	/* TX buffer			*/
	unsigned char	*optr;		/* Next byte to transmit	*/
	unsigned int	errors;		/* KISS protocol error count	*/
	unsigned int	nondata;	/* Non-data frames rx count	*/
	unsigned int	rxpackets;	/* RX frames count		*/
	unsigned int	txpackets;	/* TX frames count		*/
	unsigned long	rxbytes;	/* RX bytes count		*/
	unsigned long	txbytes;	/* TX bytes count		*/
};

static int kiss_rx(struct iface *ifp, unsigned char c, int usecrc);
static int kiss_tx(int fd, int port, unsigned char *s, int len, int usecrc);
static void report(struct iface *tty, struct iface *pty, int numptys);

static int poll(int fd)
{
	unsigned int retval;
	int len;
	
	retval = i2c_smbus_read_byte(fd);
	
	//	Returns POLL (0x0e) if nothing to receive, otherwise the control byte of a frame
	
	if (retval == -1)	 		// Read failed		
  	{
		perror("i2mkiss: poll failed");	 
	
		if (logging)
			report(tty, pty, 1);
	
		exit(0);

	}
		
	if (retval == 0x0e)
		return 0;
		
	printf("%x ", retval);
	
	
	// 	Read message up to a FEND
		
	while (TRUE)
	{
		len = kiss_rx(tty, retval, crcflag);
		
		if (len == -1)
		return 0;		// Bad Packet
			
		if (len)
		{			
			// We have a complete KISS frame from TNC
						
			kiss_tx(pty->fd, 0, tty->obuf, len, 0);
			pty->txpackets++;
			pty->txbytes += len;
		
			printf("Packet Received");
			return 0;
		}
		
		usleep(1000);
		
		retval = i2c_smbus_read_byte(fd);

		printf("%x ", retval);	
	
		if (retval == -1)	 		// Read failed		
	  	{
			perror("i2mkiss: poll failed in packet loop");	
				
			if (logging)
				report(tty, pty, 1);
	
			exit(0);
		}
	}

}

static int put_ubyte(unsigned char* s, unsigned char * crc, unsigned char c, int usecrc)
{ 
  	int len = 1;

  	if (c == FEND) { 
		*s++ = FESC;
		*s++ = TFEND;
		len++;
  	} else { 
		*s++ = c;
		if (c == FESC) {
			*s++ = TFESC;
			len++;
		}
	}

	switch (usecrc) {
	case G8BPQ_CRC:
		*crc ^= c;	/* Adjust checksum */
		break;
	}

	return len;
}

static int kiss_tx(int fd, int port, unsigned char *s, int len, int usecrc)
{
	unsigned char *ptr = obuf;
	unsigned char c, cmd;
	unsigned char crc = 0;
	int i;
	int ret = 0;

	cmd = s[0] & 0x0F;

	/* Non-data frames don't get a checksum byte */
	if (usecrc == G8BPQ_CRC && cmd != 0 && cmd != ACKREQ)
		usecrc = FALSE;

	/*
	 * Send an initial FEND character to flush out any
	 * data that may have accumulated in the receiver
	 * due to line noise.
	 */

	*ptr++ = FEND;

	c = *s++;
	c = (c & 0x0F) | (port << 4);
	ptr += put_ubyte(ptr, &crc, c, usecrc);
	
	/*
	 * For each byte in the packet, send the appropriate
	 * character sequence, according to the SLIP protocol.
	 */

	for(i = 0; i < len - 1; i++)
		ptr += put_ubyte(ptr, &crc, s[i], usecrc);

	/*
	 * Now the checksum...
	 */
	switch (usecrc) {
	case G8BPQ_CRC:
		c = crc & 0xFF;
		ptr += put_ubyte(ptr, &crc, c, usecrc);
		break;
	}
	
	*ptr++ = FEND;
	
	if (fd == pty->fd)
       return sendto(fd, obuf, ptr - obuf, 0, (struct sockaddr *) &peer_addr, peer_addr_len);
 
	for (i = 0; i < ptr - obuf; i++)
	{
		ret += i2c_smbus_write_byte(fd, obuf[i]);
		usleep(1000);
	}
	
	return ret;
}

static int kiss_rx(struct iface *ifp, unsigned char c, int usecrc)
{
	int len;

	switch (c)
	{
	case FEND:
	
		len = ifp->optr - ifp->obuf;

		if (len != 0 && ifp->escaped)
		{
		 	/* protocol error...	*/
			len = 0;		/* ...drop frame	*/
			syslog(LOG_INFO, "Dropping Frame\n");

			ifp->errors++;
		}
		
		if (len != 0)
		{
			if (usecrc)
			{
				if ((ifp->crc & 0xFF) != 0)
				{
					syslog(LOG_INFO, "CRC Error %d\n", ifp->crc);
					/* checksum failed...	*/
					/* ...drop frame	*/
					len = 0;
					crc_errors++;
				}
				 else
				{
					/* delete checksum byte	*/
					len--;
				}				 
			}
		}
		if (len != 0)
		{ 
			ifp->rxpackets++;
			ifp->rxbytes += len;
		}
		/*
		 * Clean up.
		 */
		ifp->optr = ifp->obuf;
		ifp->crc = 0;
		ifp->escaped = FALSE;
		return len;
		
	case FESC:
		ifp->escaped = TRUE;
		return 0;
		
	case TFESC:
		if (ifp->escaped) {
			ifp->escaped = FALSE;
			c = FESC;
		}
		break;
	case TFEND:
		if (ifp->escaped) {
			ifp->escaped = FALSE;
			c = FEND;
		}
		break;
	default:
		if (ifp->escaped) {		/* protocol error...	*/
			ifp->escaped = FALSE;
			ifp->errors++;
		}
		break;
	}

	*ifp->optr++ = c;

	if (usecrc)
		ifp->crc ^= c;
		
	len = ifp->optr - ifp->obuf;
	
	if (len > SIZE)
	{
		ifp->optr--;
		syslog(LOG_INFO, "Frame too long\n");
		return -1;
	}
		
	return 0;
}

static void sigterm_handler(int sig)
{
	if (logging) {
		syslog(LOG_INFO, "terminating on SIGTERM\n");
		report(tty, pty, 1);
		closelog();
	}
		
	exit(0);
}

static void sigusr1_handler(int sig)
{
	signal(SIGUSR1, sigusr1_handler);
	dump_report = TRUE;
}

static void report(struct iface *tty, struct iface *pty, int numptys)
{
	long t;

	time(&t);
	syslog(LOG_INFO, "version %s.", VERSION);
	syslog(LOG_INFO, "Status report at %s", ctime(&t));	      
	syslog(LOG_INFO, "Poll interval %d00ms", pollspeed);
	syslog(LOG_INFO, "i2cinterface is %s/%x (fd=%d)", tty->name, tty->device, tty->fd);
	syslog(LOG_INFO, "pty is %s (fd=%d)", pty->name, pty->fd);
	syslog(LOG_INFO, "Checksum errors: %d", crc_errors);
	syslog(LOG_INFO, "Invalid ports: %d", invalid_ports);
	syslog(LOG_INFO, "Returned polls: %d", return_polls);
	syslog(LOG_INFO, "Interface   TX frames TX bytes  RX frames RX bytes  Errors");
	syslog(LOG_INFO, "%-11s %-9u %-9lu %-9u %-9lu %u",
	       tty->name,
	       tty->txpackets, tty->txbytes,
	       tty->rxpackets, tty->rxbytes,
	       tty->errors);

	syslog(LOG_INFO, "%-11s %-9u %-9lu %-9u %-9lu %u",
	       pty->name,
	       pty->txpackets, pty->txbytes,
		   pty->rxpackets, pty->rxbytes,
		   pty->errors);
	
	return;
}

int main(int argc, char *argv[])
{
	unsigned char *icp;
	int topfd;
	fd_set readfd;
	struct timeval timeout, pollinterval;
	int retval, size, len;
	int masterfd;
	char slavedevice[80];
	int udpport = 0;
	struct sockaddr_in sinx, dest;
	int sock;

	//	We must poll. Default is set to 100 ms, and we must use BPQ CRC
	
	pollinterval.tv_sec = pollspeed / 10;
	pollinterval.tv_usec = (pollspeed % 10) * 100000L;

	while ((size = getopt(argc, argv, "lp:m:v")) != -1) {
		switch (size) {
	
		case 'l':
	        logging = TRUE;
	        break;
		case 'p':
			pollspeed = atoi(optarg);
			pollinterval.tv_sec = pollspeed / 10;
			pollinterval.tv_usec = (pollspeed % 10) * 100000L;
	        break;
		case 'v':
			printf("remotepitnc: %s\n", VERSION);
			return 1;
		case ':':
		case '?':
			fprintf(stderr, usage_string);
			return 1;
		}
	}

	// Need 3 params - bus. device. udpport

	if ((argc - optind) != 3)
	{
		fprintf(stderr, usage_string);
		return 1;
	}

	// Open and configure the i2c interface
	
	if ((tty = calloc(1, sizeof(struct iface))) == NULL) {
		perror("remotepitnc: malloc");
		return 1;
	}

                              
	tty->fd = open(argv[optind], O_RDWR);
	
	fprintf(stderr, "%d %d\n", tty->fd, errno);

	if (tty->fd < 0)
	{
		fprintf(stderr, "%s: Cannot find i2c bus %s, no such file or directory.\n", progname, argv[optind]);
		exit(1);
	}
	
	
	address = strtol(argv[optind + 1], 0, 0);
 
 	retval = ioctl(tty->fd,  I2C_SLAVE, address);
	
	fprintf(stderr, "%d %d\n", retval, errno);
	
	if(retval == -1)
	{
		fprintf(stderr, "%s: Cannot open i2c device %x\n", progname, address);
		exit (1);
	}
 
	tty->name = argv[optind];
	tty->optr = tty->obuf;
	topfd = tty->fd;

	/*
	 * Open and configure the UDP interface
	 */


	if ((pty = calloc(1, sizeof(struct iface))) == NULL) {
		perror("remotepitnc: malloc");
		return 1;
	}


	udpport = strtol(argv[optind + 2], NULL, 0);
 
	pty->fd = sock = socket(AF_INET,SOCK_DGRAM,0);

	fprintf(stderr, "sock %d %d\n", sock, errno);


	sinx.sin_family = AF_INET;
	sinx.sin_addr.s_addr = INADDR_ANY;		
	sinx.sin_port = htons(udpport);
	


	if (bind(sock, (const struct sockaddr *)&sinx, sizeof(sinx)) != 0 )
	{

		//	Bind Failed
			
		perror("Bind Failed for UDP socket");
		return 0;
	}
	
	fprintf(stderr, "Bind to %d OK \n", udpport);

	topfd = (pty->fd > topfd) ? pty->fd : topfd;

	signal(SIGHUP, SIG_IGN);
	signal(SIGUSR1, sigusr1_handler);
	signal(SIGTERM, sigterm_handler);
	
//	if (!daemon_start(FALSE))
//	{
//		fprintf(stderr, "remotepitnc: cannot become a daemon\n");
//		return 1;
//	}

	if (logging) {
		openlog("remotepitnc", LOG_PID, LOG_DAEMON);
		syslog(LOG_INFO, "starting");
	}
	
	// Reset the TNC and wait for completion
	
	i2c_smbus_write_byte(tty->fd, FEND);
	i2c_smbus_write_byte(tty->fd, 9);
	
	sleep(2);
                                
										 
	/*
	 * Loop until an error occurs on a read.
	 */

	while (TRUE)
	{
		FD_ZERO(&readfd);
		FD_SET(pty->fd, &readfd);

		if (pollspeed)
			timeout = pollinterval;

		errno = 0;
		retval = select(topfd + 1, &readfd, NULL, NULL, pollspeed ? &timeout : NULL);

		if (retval == -1) {
			if (dump_report) {
				if (logging)
					report(tty, pty, 1);
				dump_report = FALSE;
				continue;
			} else {
				perror("remotepitnc: select");
				continue;
			}
		}

		/*
		 * Timer expired - let's poll...
		 */
		if (retval == 0 && pollspeed) {
			poll(tty->fd);
			continue;
		}

		//	TTY is i1c device - we get messages from it via poll()
		
		// See if a character has arrived on pty 
		
		if (FD_ISSET(pty->fd, &readfd))
		{
			peer_addr_len = sizeof(struct sockaddr_storage);
			size = recvfrom(pty->fd, ibuf, SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
        
			if (size < 0 && errno != EINTR)
			{
				if (logging)
					syslog(LOG_ERR, "pty->fd: %m\n");
				goto end;
			}
			
			printf("UDP RX %d\n", size);
						
			for (icp = ibuf; size > 0; size--, icp++)
			{
				len = kiss_rx(pty, *icp, FALSE);
				
				if (len)
				{
					// We have a complete KISS frame from Kernel
						
					kiss_tx(tty->fd, 0, pty->obuf, len, crcflag);
					tty->txpackets++;
					tty->txbytes += len;
				}
			}
		}
	}
	
end:
	
	if (logging)
		report(tty, pty, 1);
	
	close(tty->fd);
	close(pty->fd);
		
	return 1;
}
