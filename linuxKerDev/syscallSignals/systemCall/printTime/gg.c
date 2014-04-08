#include<stdio.h>
#include<errno.h>
#include<sys/syscall.h>
#include<asm/unistd.h>

//_syscall1(int,goober,long*,ptr)
int goober(long* ptr)
{
	return syscall(312,ptr);
}

int main()
{
	long time = 0;
	//syscall(312,&time);
	goober(&time);
	printf("time:%ld\n",time);
}
