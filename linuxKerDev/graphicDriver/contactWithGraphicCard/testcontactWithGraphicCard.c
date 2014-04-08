#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <math.h>
#include <sys/mman.h>

#define KYOUKO2_CONTROL_SIZE (65536)
#define Device_RAM (0x0020)   //From kyouko2 Manual

struct u_kyouko_device{
	unsigned int *u_control_base;
}kyouko2;

unsigned int U_READ_REG(unsigned int rgister){
	return (*(kyouko2.u_control_base + (rgister>>2)));
}

int main()
{
	int fd;
	int result;
	fd = open("/dev/kyouko2",O_RDWR);
	printf("Open in user space.\n");
	kyouko2.u_control_base = mmap(0, KYOUKO2_CONTROL_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd,0);
	printf("u_control_base: %%%x\n", kyouko2.u_control_base);
	result = U_READ_REG(Device_RAM);
	printf("RAM size in MB is: %d.\n",result);
	close(fd);
	return 0;
}
