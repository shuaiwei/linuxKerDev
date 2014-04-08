#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/fs.h>	
#include <linux/cdev.h>
MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("ShuaiWei");

struct cdev kyouko2_cdev;
 
static int kyouko2_open(struct inode* inode, struct file *fp){
	/* The first token to prink() is the message priority level. It ranges from 0 to 7 
	 * (KERN_ALERT ==1), and lower is more urgent. if 1 < i, i is the first integer in 
	 * /proc/sys/kernel/printk, the message is printed on the console
	 */
	printk(KERN_ALERT "Open a device.\n");
	return 0;
}

static int kyouko2_release(struct inode* inode, struct file *fp){
	printk(KERN_ALERT "Release a device.\n");
	return 0;
}

struct file_operations kyouko2_fops = {
	.open = kyouko2_open,
	.release = kyouko2_release,
	.owner = THIS_MODULE
};

static int __init launchModule_init(void){
	int flag = -1;

	/* Register the device as a character device with file operation
	 */ 
	cdev_init(&kyouko2_cdev,&kyouko2_fops);
	flag = cdev_add(&kyouko2_cdev,MKDEV(500,127), 1); //  1: How many
	
	if(flag)		
		printk(KERN_ALERT "Init unsuccessfullly!\n");
	else	printk(KERN_ALERT "Init successfully!\n");
	return flag;
}

/* No return value
 */
static void __exit launchModule_exit(void){
	cdev_del(&kyouko2_cdev);
	printk(KERN_ALERT "Exit successfully.\n");
}
	
module_init(launchModule_init);
module_exit(launchModule_exit);

