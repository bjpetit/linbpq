#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/i2c-dev.h>

#define SHUNT_RESISTOR                  (0.02)

int main(int argc, char* argv[])
{
    int i2c, bus_addr = -1, i, space;
    char buf[300], counts[3];
    float voltage, current;
    int n;

#include <fcntl.h>
#include <stdio.h>


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
                    
            
    if(argc != 3)
    {
        fputs("Expecting: /dev/i2c-0 0x8\n", stderr);
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
    
       
//    printf("%x %x %x %x %x %x %x %x \n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
    
//    if (buf[0] > 0)
//        printf("Returned count %d\n", read(i2c, buf, buf[0] + 1));
         
 //   for (n =0; n < 8; n++)
    {
 //   	buf[0] = i2c_smbus_read_byte(i2c);
 //   	buf[1] = i2c_smbus_read_byte(i2c);
 //   	buf[2] = i2c_smbus_read_byte(i2c);
 //   	buf[3] = i2c_smbus_read_byte(i2c); 
 //  	buf[4] = i2c_smbus_read_byte(i2c); 
 //  	buf[5] = i2c_smbus_read_byte(i2c);
 //   	buf[6] = i2c_smbus_read_byte(i2c);
//	buf[7] = i2c_smbus_read_byte(i2c);

//	printf("%x %x %x %x %x %x %x %x \n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	
	}


//	i2c_smbus_write_byte(i2c, 0xc0);
//	i2c_smbus_write_byte(i2c, 0x00);
//	i2c_smbus_write_byte(i2c, 0xaa);
//	i2c_smbus_write_byte(i2c, 0xab);	
//	i2c_smbus_write_byte(i2c, 0xab);
//	i2c_smbus_write_byte(i2c, 0xaa);
//	i2c_smbus_write_byte(i2c, 0xc0);

	
    for (i = 0; i < 256; i++)
        buf[i] = i;

	//while(1)
	{

	n = 256;
	i = 0;
	
	while (n > 0)

	{
		int c, wc, x;
		
		c = read(i2c, counts, 2);
		
		if (c == 255)
            perror("Read");
                           
                           		
	     printf("Returned count %d\n", c);
		   
	     space = 32 - counts[1];
		 
 	     printf("%d %d Space %d n %d\n", counts[0], counts[1], space, n);  
		 
		 if (buf[1] == 255)
                 {
                  return 0;
                }
		if (space)
		{      
		if (n > space)
		{
			wc = 0;
			for (x = 0; x < space; x++)
			{
				c = write(i2c, &buf[i+x], 1);
				printf("%d ", wc);
				if (c != 1)
				{

					perror("Write");
					return 0;
				}
				wc += c;
			}
				
  			printf("Write count %d\n", wc);

			if (wc != space)
			{
                perror("Write Block");
                return 0;
            }
			i += space;
			n -= space;
		}
		else
		{
			wc = write(i2c, &buf[i], n);
				
  			printf("Write count %d\n", wc);

			if (wc != n)
                       {
                                 perror("Write");
                                 return 0;
                         }
                                                        
			i += n;
			n = 0;
		}
		}
		else
			usleep(10000);

		
	}
	}
	return 0;                                                                  
}
                                                                                                                                                                                                            
