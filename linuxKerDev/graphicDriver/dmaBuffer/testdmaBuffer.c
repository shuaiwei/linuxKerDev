#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include "regdmaBuffer.h"	
#include "macrodmaBuffer.h"
#define KYOUKO2_CONTROL_SIZE (65536)

struct u_kyouko_device{
	unsigned int *u_control_base;
	unsigned int *u_card_ram_base;
}kyouko2;

typedef struct{
	unsigned int stride:5;
	unsigned int has_v4:1;
	unsigned int has_c3:1;
	unsigned int has_c4:1;
	unsigned int unused:4;
	unsigned int prim_type:2;
	unsigned int count:10;
	unsigned int opcode:8;
}Kyouko2_dma_hdr;

Kyouko2_dma_hdr hdr = {
	.stride = 0x5, 
	.has_v4 = 0x0,
	.has_c3 = 0x1,
	.has_c4 = 0x0,
	.unused = 0x0,
	.prim_type = 1,
	.count = 3*2,
	.opcode = 0x14,
};
int main()
{
	int fd,ret,result,i,j;
	float randws;
	unsigned long dmaBuffer0 = 0;
	unsigned long dmaBuffer1 = 37*4;
	float *dmaBuffer0f = 0;
	float *dmaBuffer1f = 0;
	fd = open("/dev/kyouko2", O_RDWR);
	
	ret = ioctl(fd,VMODE,GRAPHICS_ON);
	ret = ioctl(fd,SYNC);
	ret = ioctl(fd,BIND_DMA,&dmaBuffer0);
	printf("dmabuffer0 address:0x%lx\n",dmaBuffer0);
	memcpy((float *)dmaBuffer0,&hdr,sizeof(hdr));		
	dmaBuffer0f = (float *)dmaBuffer0 + 1;	
	srand(time(NULL));
	for(i = 0; i < 16; i++)
	{
		for(j = 0; j < 18*2; j++)
		{
			*dmaBuffer0f++ = rand() / (float)RAND_MAX;
			*dmaBuffer0f++ = rand() / (float)RAND_MAX;		
			*dmaBuffer0f++ = rand() / (float)RAND_MAX;
			*dmaBuffer0f++ = -1 + 2.0 * rand() / (float)RAND_MAX;	
			*dmaBuffer0f++ = -1 + 2.0 * rand() / (float)RAND_MAX;
			*dmaBuffer0f++ = -1 + 2.0 * rand() / (float)RAND_MAX;
			*dmaBuffer0f++ = -1 + 2.0 * rand() / (float)RAND_MAX;
		}
		ret = ioctl(fd,FLUSH);
		ret = ioctl(fd,START_DMA,&dmaBuffer1);	
		printf("dmabuffer %d address:0x%lx\n",i,dmaBuffer1);	
		memcpy((float *)dmaBuffer1,&hdr,sizeof(hdr));		
		dmaBuffer0f = (float *)dmaBuffer1 + 1;
		dmaBuffer1 = 37;	
	}

	sleep(3);
	ret = ioctl(fd,VMODE,GRAPHICS_OFF);	
	close(fd);
	return 0;
}
