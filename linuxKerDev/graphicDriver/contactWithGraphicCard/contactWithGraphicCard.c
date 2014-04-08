#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/mod_devicetable.h>

#define PCI_VENDOR_ID_CCORSI (0x1234)
#define PCI_DEVICE_ID_KYOUKO2 (0x1113)
#define KYOUKO2_CONTROL_SIZE (65536)

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Shuai Wei");

/*	To find a real device,  we need to use the PCI bus
 */
struct p_kyouko2_device{
	unsigned long p_control_base;
	unsigned long p_card_ram_base;
	unsigned int* k_control_base;
	unsigned int* k_card_ram_base;
	struct pci_dev* p_pci_dev;
}kyouko2;

static void K_WRITE_REG(unsigned int reg, unsigned int value){
	udelay(1);	
	*(kyouko2.k_control_base + (reg>>2)) = value;
}

static int kyouko2_open(struct inode* inode, struct file *fp){

	//Take physical address as input and return corresponding kernel virtual address as output.
	kyouko2.k_control_base = ioremap_nocache(kyouko2.p_control_base, KYOUKO2_CONTROL_SIZE);
	kyouko2.k_card_ram_base = ioremap_nocache(kyouko2.p_card_ram_base, KYOUKO2_CONTROL_SIZE);
	printk(KERN_ALERT "Open a device.\n");
	return 0;
}

static int kyouko2_release(struct inode* inode, struct file *fp){
	
	/* Immediately reboot the device
	 * Configuration changes will be lost
	 */
	K_WRITE_REG(0x1000,1);
	iounmap((void*)kyouko2.k_control_base);
	iounmap((void*)kyouko2.k_card_ram_base);
	printk(KERN_ALERT "Release a device.\n");
	return 0;
}

static int kyouko2_mmap(struct file *fp, struct vm_area_struct *vma){
	int flag = -1;

	//Take physical address as input and return corresponding user virtual address as output.
	flag = io_remap_pfn_range(vma, vma->vm_start, kyouko2.p_control_base >> PAGE_SHIFT, \
		vma->vm_end-vma->vm_start, vma->vm_page_prot);
	printk(KERN_ALERT "Have entered mmap function.\n");
	return flag;
}	

struct file_operations kyouko2_fops = {
	.open = kyouko2_open,
	.release = kyouko2_release,
	.mmap = kyouko2_mmap,
	.owner = THIS_MODULE
};

struct cdev kyouko2_cdev;

struct pci_device_id kyouko2_dev_ids[]={
	{PCI_DEVICE(PCI_VENDOR_ID_CCORSI,PCI_DEVICE_ID_KYOUKO2)}, //Use them to look for the device.
	{0}
};

/*The probe() hook gets called by the PCI generic code
 *when a matching device is found
 */
static int kyouko2_probe(struct pci_dev *pci_dev, const struct pci_device_id *pci_id){
	
	/*Each PCI device can have up to 6 I/O or memory regions,
	 *described in BAR0 to BAR5.
	 */
	kyouko2.p_control_base = pci_resource_start(pci_dev,1);	//Access the base address of the I/O region	
	kyouko2.p_card_ram_base = pci_resource_start(pci_dev,2);	
	kyouko2.p_pci_dev = pci_dev;
	printk(KERN_ALERT "Have entered probe function.\n");
	printk(KERN_ALERT "p_control_base: %%%lx.\n",kyouko2.p_control_base);
	printk(KERN_ALERT "p_card_ram_base: %%%lx.\n",kyouko2.p_card_ram_base);
	return 0;
}

static void kyouko2_remove(struct pci_dev *pci_dev){
	
}

struct pci_driver kyouko2_pci_driver ={
	.name = "kyouko2_pci_driver",   // show up in /sys/bus/pci/drivers
	.id_table = kyouko2_dev_ids,
	.probe = kyouko2_probe,
	.remove = kyouko2_remove,
};

static int __init shuaiwei_init(void){
	int flag = -1;
	cdev_init(&kyouko2_cdev,&kyouko2_fops);
	flag = cdev_add(&kyouko2_cdev,MKDEV(500,127), 1);
	
	if(flag){
		printk(KERN_ALERT "Inited unsuccessfully!\n");
		return flag;
	}
	else{	
		printk(KERN_ALERT "Inited successfully!\n");

		/*Invoke probe function
		 */
		flag = pci_register_driver(&kyouko2_pci_driver); 
		if(!flag){
			printk(KERN_ALERT "Registration succeeded!\n");
			/*Before touching any device registers, the driver should first 
			 *execute pci_enable_device().
			 */
			flag = pci_enable_device(kyouko2.p_pci_dev);
			if(flag){	
				printk(KERN_ALERT "PCI device enabled unsuccessfully.\n");
				return flag;
			}		
			else 	printk(KERN_ALERT "PCI device enabled successfully\n");

			/* Allow device to initiate DMA transfer 
			 */
			pci_set_master(kyouko2.p_pci_dev);
		}
		else{
			printk(KERN_ALERT "Registration failed!\n");
			return flag;
		}
	}
	return flag;
}

static void __exit shuaiwei_exit(void){
	pci_disable_device(kyouko2.p_pci_dev);
	pci_unregister_driver(&kyouko2_pci_driver);
	cdev_del(&kyouko2_cdev);
	printk(KERN_ALERT "Exiting!");
}
	
module_init(shuaiwei_init);
module_exit(shuaiwei_exit);
