#include<asm/unistd.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/errno.h>
_syscall2(int,smunch,pid_t,pid,unsigned long long,bitpattern);
int main(int argc,char** argv)
{
	unsigned long long bitpattern = (1 << 8) | (1 << 2);
	int result;
	pid_t pid = atoi(argv[1]);
	result = smunch(pid,bitpattern);
	printf("result: %d\n",result);
	return 0;
}
