#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <asm/signal.h>
int smunch(pid_t pid, u64 bit_pattern) {
	struct task_struct *t;
	unsigned long flags;
	
	rcu_read_lock();
	t=find_task_by_vpid(pid); // get the process table entry
	rcu_read_unlock();

	if(!t){
		 printk(KERN_ALERT "could not found process entry table!\n");
		 return -1;
	}
	else  	 printk(KERN_ALERT "found process entry table!\n");
	
	if(!lock_task_sighand(t,&flags))
	{		
		printk(KERN_ALERT "could not acquire a lock!\n");	
		return -1; 
	}

	//if process is not a single process;
	if(!thread_group_empty(t)) { 
		printk(KERN_ALERT "process is not a single thread!\n");
		unlock_task_sighand(t,&flags);
		return -1;
	}

	if((t->exit_state & (EXIT_ZOMBIE | EXIT_DEAD)) && (bit_pattern & (1 << (SIGKILL-1)))) {
		printk(KERN_ALERT "Target is a zombie and SIGKILL is set\n");
		unlock_task_sighand(t, &flags);
		release_task(t);
		return 0;
	}
	else if((t->exit_state & (EXIT_ZOMBIE | EXIT_DEAD)) && (bit_pattern & (1 << (SIGKILL-1)))) {
		printk(KERN_ALERT "ingnore signals that are not a sigkill when the process is a zombie;\n");
	}	
	else if(!(t->exit_state & (EXIT_ZOMBIE | EXIT_DEAD)) && (bit_pattern & (1 << (SIGKILL-1)))) {
		printk(KERN_ALERT "the regular process is unconditionally killed when one of the signals sent is SIGKILL!\n");	
		sigaddset(&t->pending.signal,SIGKILL);
		wake_up_process(t);
	}
	else{
		printk(KERN_ALERT "target is not a zombie, and there is no sigkill signal!\n");
		if(_NSIG_WORDS == 1)
			t->pending.signal.sig[0] |= bit_pattern;
		else{
			t->pending.signal.sig[0] |= (u32)bit_pattern; 
			t->pending.signal.sig[1] |= bit_pattern >> 32;
		} 	
		signal_wake_up(t,0);
	}	
	unlock_task_sighand(t, &flags);
	return 0;
}
