#include <linux/linkage.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <asm-generic/uaccess.h>
asmlinkage int goober(long* ptr)
{
	struct timespec now;
	now =current_kernel_time();
	copy_to_user(ptr,&now.tv_nsec,sizeof(long));
	return 0;
}
