#include<stdio.h>
#include<sys/syscall.h>
#include<errno.h>
_syscall1(int,goober,long*,ptr)
int main()
{
	long time;
	goober(&time);
	printf("%ld\n",time);
	return 0;
}
