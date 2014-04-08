#include <stdio.h>
#include <asm/unistd.h>
#include <sys/errno.h>
_syscall0(void, deepsleep);

main() {
	printf("goodnight,Irene\n");
	deepsleep();
	printf("oops ... woke up!\n");
}
