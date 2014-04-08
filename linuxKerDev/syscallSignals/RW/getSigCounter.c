#include<linux/kernel.h>
#include<linux/linkage.h>
#include<linux/sched.h>
#include<asm/current.h>
#include<linux/pid.h>
asmlinkage int get_sigcounter(int signumber)
{
	int counter;
	unsigned long flags;
	lock_task_sighand(current,&flags);
	counter = current->sighand->counters[signumber];
	unlock_task_sighand(current,&flags);
	return counter;
}
