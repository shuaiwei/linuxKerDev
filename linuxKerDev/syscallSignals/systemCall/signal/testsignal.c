#include<stdio.h>
#include<sys/signal.h>
#include<sys/syscall.h>
#include<errno.h>
_syscall1(int,init_sigcounter,int, pid)//Has nothing to do with the name of function name
_syscall1(int,get_sigcounter,int, sig)

void happy()
{
	int value;
	int counterSIGUSR1,counterSIGSTOP,counterSIGCONT,counterSIGWINCH;
	counterSIGUSR1 = get_sigcounter(10);
	counterSIGSTOP = get_sigcounter(19);
	counterSIGCONT = get_sigcounter(18);
	counterSIGWINCH = get_sigcounter(28);
	printf("counterSIGUSR1:%d\ncounterSIGSTOP:%d\ncounterSIGCONT:%d\ncounterSIGWINCH:%d\n",counterSIGUSR1,counterSIGSTOP,counterSIGCONT,counterSIGWINCH);}

main()
{
	int pid, ret;
	switch(pid=fork()){
		case 0:
			signal(SIGUSR1,happy);
			while(1){
				printf("child is playing\n");
				sleep(1);
			}
			break;
		default:
			ret = init_sigcounter(pid);
			while(1){
				printf("parent is going to sleep\n");
				sleep(10);
				printf("parent wakes up...checks on child\n");
				ret=kill(pid,SIGUSR1);
				ret=kill(pid,SIGSTOP);
				ret=kill(pid,SIGCONT);
				ret=kill(pid,SIGWINCH);
				printf("kill etc returned\n");
			}
	}
}
		
	
