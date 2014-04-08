#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "regdrawLineTriangle.h"
#include "macrodrawLineTriangle.h"
#define KYOUKO2_CONTROL_SIZE (65536)
#define Device_RAM (0x0020)

struct u_kyouko_device{
	unsigned int *u_control_base;
	unsigned int *u_card_ram_base;
}kyouko2;

unsigned int U_READ_REG(unsigned int rgister){
	return (*(kyouko2.u_control_base + (rgister>>2)));
}

void U_WRITE_REG(unsigned int rgister, unsigned int value){
	 *(kyouko2.u_control_base + (rgister>>2)) = value;
}

void U_WRITE_REG_F(unsigned int rgister, float value){
	 *(kyouko2.u_control_base + (rgister>>2)) = *(unsigned int *)&value;
}

void U_WRITE_REG_FB(unsigned int rgister, unsigned int value){
	 *(kyouko2.u_card_ram_base + rgister) = value;
}


int main()
{
	int fd,i;
	int result;
	long ret = 0;
	
	fd = open("/dev/kyouko2", O_RDWR);
	printf("user space opening\n");
	//physical base address 0 map to user space 
	kyouko2.u_control_base = mmap(0, KYOUKO2_CONTROL_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	printf("u_control_base: %%%x\n", kyouko2.u_control_base);
	result = U_READ_REG(Device_RAM);
	printf("RAM size in MB is: %d\n", result);
	result = result * 1024 * 1024;		
	//physical base address 80000000 map to user space 
	kyouko2.u_card_ram_base = mmap(0, result, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x80000000);
	
	ret = ioctl(fd,VMODE,GRAPHICS_ON);
	ret = ioctl(fd,SYNC);
	for(i=200*1024; i<201*1024;i++)
		U_WRITE_REG_FB(i,0x00ff0000);
		
	U_WRITE_REG(RasterFlush,1);
	ret = ioctl(fd,SYNC);
	U_WRITE_REG(RasterFlush,1);
	ret = ioctl(fd,SYNC);
	sleep(5);

	U_WRITE_REG(RasterPrimitive,1);	
	
	//v1	
	U_WRITE_REG_F(DrawVertexCoordX,-0.5);
	U_WRITE_REG_F(DrawVertexCoordY,-0.5);
	U_WRITE_REG_F(DrawVertexCoordZ,0.0);
	U_WRITE_REG_F(DrawVertexCoordW,1.0);
	
	U_WRITE_REG_F(DrawVertexColorR,0.5);
	U_WRITE_REG_F(DrawVertexColorG,0.5);
	U_WRITE_REG_F(DrawVertexColorB,0.4);	
	U_WRITE_REG_F(DrawVertexColorAlpha,0.0);
	U_WRITE_REG(RasterEmit,0);
	
	//v2
	U_WRITE_REG_F(DrawVertexCoordX,0.5);
	U_WRITE_REG_F(DrawVertexCoordY,-0.5);
	U_WRITE_REG_F(DrawVertexCoordZ,0.0);
	U_WRITE_REG_F(DrawVertexCoordW,1.0);
	
	U_WRITE_REG_F(DrawVertexColorR,0.2);
	U_WRITE_REG_F(DrawVertexColorG,0.3);
	U_WRITE_REG_F(DrawVertexColorB,0.5);	
	U_WRITE_REG_F(DrawVertexColorAlpha,0.0);
	U_WRITE_REG(RasterEmit,0);
	
	//v3
	U_WRITE_REG_F(DrawVertexCoordX,-0.5);
	U_WRITE_REG_F(DrawVertexCoordY,0.5);
	U_WRITE_REG_F(DrawVertexCoordZ,0.0);
	U_WRITE_REG_F(DrawVertexCoordW,1.0);
	
	U_WRITE_REG_F(DrawVertexColorR,0.7);
	U_WRITE_REG_F(DrawVertexColorG,0.5);
	U_WRITE_REG_F(DrawVertexColorB,0.1);	
	U_WRITE_REG_F(DrawVertexColorAlpha,0.0);
	U_WRITE_REG(RasterEmit,0);

	U_WRITE_REG(RasterPrimitive,0);
	U_WRITE_REG(RasterFlush,1);
	ret = ioctl(fd,SYNC);
	U_WRITE_REG(RasterFlush,1);
	ret = ioctl(fd,SYNC);
	sleep(5);
	ret = ioctl(fd,VMODE,GRAPHICS_OFF);	
	
	close(fd);
	return 0;
}
