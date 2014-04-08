#include<linux/kernel.h>
#include<linux/linkage.h>
#include<linux/sched.h>
#include<asm/current.h>
#include<linux/pid.h>
asmlinkage int init_sigcounter(int pid)
{
	struct task_struct *p;
	int i;
	unsigned long flags;
	p = pid_task(find_vpid(pid),PIDTYPE_PID);
	lock_task_sighand(p,&flags);
	for(i = 0; i < 66; i++)
		p->sighand->counters[i] = 0;
	unlock_task_sighand(p,&flags);
	printk(KERN_ALERT "current pid:%d\npid:%d\n",current->pid,pid);
	return 0;
}
	
