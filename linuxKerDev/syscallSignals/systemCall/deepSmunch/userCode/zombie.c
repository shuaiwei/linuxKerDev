#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
main() {
	int pid;
	switch(pid=fork()) {
		case 0:
			printf("try to get pid %d\n",getpid());
			exit(0);
		default:
			printf("without killing %d\n",getpid());
			while(1) {
				sleep(20);
			}
	}
}
