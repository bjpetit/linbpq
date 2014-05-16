#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/i2c-dev.h>

#define SHUNT_RESISTOR                  (0.02)

int main(int argc, char* argv[])
{
    int i2c, bus_addr = -1, i;
    char buf[300] = "";
    float voltage, current;
    int n;

#include <fcntl.h>
#include <stdio.h>

/*
int masterfd, slavefd;
char slavedevice[32];


masterfd = posix_openpt(O_RDWR|O_NOCTTY);


if (masterfd == -1
    || grantpt (masterfd) == -1
        || unlockpt (masterfd) == -1
            || (ptsname_r(masterfd, slavedevice, 32)) != 0)
                return -1;
                
                
                printf("slave device is: %s\n", slavedevice);
                
                
                slavefd = open(slavedevice, O_RDWR|O_NOCTTY);
                if (slavefd < 0)
                    return -1;
                    
//sleep(10000);
  */
            
    if(argc != 3)
    {
        fputs("Expecting: /dev/i2c-0 0x0\n", stderr);
        return 1;
    }

    i2c = open(argv[1], O_RDWR);
    if(i2c < 0)
    {
        perror("open");
        return 1;
    }

    bus_addr = strtol(argv[2], 0, 0);
    
    if(ioctl(i2c, I2C_SLAVE, bus_addr) == -1)
    {
        perror("ioctl(I2C_SLAVE)");
        return 1;
    }
    
    fputs("Connected to bus.\n", stdout);
	
	while (1)
	{
    	int rc;
		
    buf[0] = 0;
    
     rc = read(i2c, buf, 2);
    
//     printf("rc %d %x %x \n", rc, buf[0], buf[1]);
     
//     printf("Returned count %d\n", read(i2c, buf, buf[0] + 2));
             
             
       	if (buf[0] > 0)
	{
  	
		printf("rc %d %x %x \n", rc, buf[0], buf[1]);
    
        printf("Returned count %d\n", read(i2c, buf, buf[0] + 2));
         
 //   for (n =0; n < 8; n++)
    
 //   	buf[0] = i2c_smbus_read_byte(i2c);
 //   	buf[1] = i2c_smbus_read_byte(i2c);
 //   	buf[2] = i2c_smbus_read_byte(i2c);
 //   	buf[3] = i2c_smbus_read_byte(i2c); 
 //  	buf[4] = i2c_smbus_read_byte(i2c); 
 //  	buf[5] = i2c_smbus_read_byte(i2c);
 //   	buf[6] = i2c_smbus_read_byte(i2c);
//	buf[7] = i2c_smbus_read_byte(i2c);

	printf("%x %x %x %x %x %x %x %x \n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
        i = 8;
        printf("%x %x %x %x %x %x %x %x \n", buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
        i += 8;
        printf("%x %x %x %x %x %x %x %x \n", buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
        i += 8;
        printf("%x %x %x %x %x %x %x %x \n", buf[i], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
                   
	}
	else
		usleep(100000);

}
//	i2c_smbus_write_byte(i2c, 0xc0);
//	i2c_smbus_write_byte(i2c, 0x00);
//	i2c_smbus_write_byte(i2c, 0xaa);
//	i2c_smbus_write_byte(i2c, 0xab);	
//	i2c_smbus_write_byte(i2c, 0xab);
//	i2c_smbus_write_byte(i2c, 0xaa);
//	i2c_smbus_write_byte(i2c, 0xc0);
	


//    write(i2c, buf, sizeof(buf));

    return 1;                                                                   
}
                                                                                                                                                                                                            
