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
#include "regdrawLineTriangle.h"
#include "macrodrawLineTriangle.h"

#define PCI_VENDOR_ID_CCORSI (0x1234)
#define PCI_DEVICE_ID_KYOUKO2 (0x1113)
#define KYOUKO2_CONTROL_SIZE (65536)

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Shuai Wei");

struct p_kyouko2_device{
	unsigned long p_control_base;
	unsigned long p_card_ram_base;
	unsigned int * k_control_base;
	unsigned int * k_card_ram_base;
	struct pci_dev * p_pci_dev;
}kyouko2;

static unsigned int K_READ_REG(unsigned int reg){
	unsigned int value;
	rmb();
	value = *(kyouko2.k_control_base + (reg>>2));
	return value;
}

static void K_WRITE_REG(unsigned int reg, unsigned int value){
	udelay(1);
	*(kyouko2.k_control_base + (reg>>2)) = value;
}

static void K_WRITE_REG_F(unsigned int reg, float value){
	udelay(1);
	*(kyouko2.k_control_base + (reg>>2)) = *(unsigned int*)&value;
}

static int kyouko2_open(struct inode* inode, struct file *fp){
	unsigned long ramSize = K_READ_REG(0x20);
	ramSize = ramSize * (1024 * 1024);
	kyouko2.k_control_base = ioremap_nocache(kyouko2.p_control_base, KYOUKO2_CONTROL_SIZE);
	kyouko2.k_card_ram_base = ioremap_nocache(kyouko2.p_card_ram_base, KYOUKO2_CONTROL_SIZE);
	printk(KERN_ALERT "Open a device.\n");
	return 0;
}

static int kyouko2_release(struct inode* inode, struct file *fp){
	K_WRITE_REG(0x1000,1);
	iounmap((void*)kyouko2.k_control_base);
	iounmap((void*)kyouko2.k_card_ram_base);
	printk(KERN_ALERT "Release a device.\n");
	return 0;
}

static void sync(void){
	while(K_READ_REG(InfoFifoDepth) > 0)
		;
}

static void writeflush(void){
	K_WRITE_REG(RasterFlush,1);
}

static void clearBuf_1(void){
	K_WRITE_REG(RasterClear,1);
}

static void reboot_1(void){
	K_WRITE_REG(ConfigReboot,1);
}	

static void graphicsOn(void){
	//Set Frame
	K_WRITE_REG(FrameColumns,1024);
	K_WRITE_REG(FrameRows,768);
	K_WRITE_REG(FrameRowPitch,4*1024);
	K_WRITE_REG(FramePixelFormat,0xf888);
	K_WRITE_REG(FrameStartAddress,0);

	//Set Dac
	K_WRITE_REG(EncoderWidth,1024);
	K_WRITE_REG(EncoderHeight,768);	
	K_WRITE_REG(EncoderOffsetX,0);	
	K_WRITE_REG(EncoderOffsetY,0);	
	K_WRITE_REG(EncoderFrame,0);

	//Set acceleration
	K_WRITE_REG(ConfigAcceleration,0x40000000);
	
	sync();

	//Modeset
	K_WRITE_REG(ConfigModeset,1);

	//Set clearcolor
	float value = 0.2;
	K_WRITE_REG_F(DrawClearColorR,value);
	K_WRITE_REG_F(DrawClearColorG,value);
	K_WRITE_REG_F(DrawClearColorB,value);
	K_WRITE_REG_F(DrawClearColorAlpha,0.0);

	writeflush();

	clearBuf_1();
}
	
static long kyouko2_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case VMODE:
			if((int)arg == GRAPHICS_ON){
				graphicsOn();
			}
			else{
				sync();
				reboot_1();
			}
			break;
		case SYNC:
			sync();
			break;
		default:
			break;
	}
	return 0;
}

static int kyouko2_mmap(struct file *fp, struct vm_area_struct * vma){
	int flag = -1;
	printk(KERN_ALERT "Entered function mmap!\n");
	if(vma->vm_pgoff << PAGE_SHIFT == 0)
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.p_control_base >> PAGE_SHIFT), vma->vm_end-vma->vm_start, vma->vm_page_prot);
	else if(vma->vm_pgoff << PAGE_SHIFT == 0x80000000)
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.p_card_ram_base >> PAGE_SHIFT), vma->vm_end-vma->vm_start, vma->vm_page_prot);
	else printk(KERN_ALERT "Error in offset of mmap"); 
	return flag;
}

struct file_operations kyouko2_fops = {
	.open = kyouko2_open,
	.release =kyouko2_release,
	.unlocked_ioctl = kyouko2_ioctl,
	.mmap = kyouko2_mmap,
	.owner = THIS_MODULE
};

struct cdev kyouko2_cdev;

struct pci_device_id kyouko2_dev_ids[]={
	{PCI_DEVICE(PCI_VENDOR_ID_CCORSI, PCI_DEVICE_ID_KYOUKO2)},
	{0}
};

static int kyouko2_probe(struct pci_dev *pci_dev, const struct pci_device_id *pci_id){
	kyouko2.p_control_base = pci_resource_start(pci_dev,1);
	printk(KERN_ALERT "p_control_base is:%% %lx\n", kyouko2.p_control_base);
	kyouko2.p_card_ram_base = pci_resource_start(pci_dev,2);
	printk(KERN_ALERT "p_card_ram_base is:%% %lx\n", kyouko2.p_card_ram_base);
	kyouko2.p_pci_dev = pci_dev;
	printk(KERN_ALERT "Entered probe function");
	return 0;
}
static void kyouko2_remove(struct pci_dev *pci_dev){
}

struct pci_driver kyouko2_pci_driver ={
	.name = "kyouko2_pci_driver",
	.id_table = kyouko2_dev_ids,
	.probe = kyouko2_probe,
	.remove = kyouko2_remove,
};

/* The named function will be executed when the module is loaded
 */
static int __init drawLineTriangle_init(void){
	int flag = -1;
	cdev_init(&kyouko2_cdev, &kyouko2_fops);
	flag = cdev_add(&kyouko2_cdev, MKDEV(500,127), 1);
	if(flag){
		printk(KERN_ALERT "Registered te device as a character device unsuccessfully!\n");
		return flag;
	}
	else{
		printk(KERN_ALERT "Registered te device as a character device successfully!\n");
		
		/*Will invoke probe function
		 */
		flag = pci_register_driver(&kyouko2_pci_driver);
		if(flag<=0){
			printk(KERN_ALERT "PCI registration succeeded!\n");
			flag = pci_enable_device(kyouko2.p_pci_dev);
			if(flag){
				printk(KERN_ALERT "PCI device enable unsuccessfully.\n");				
				return flag;
			}
			else printk(KERN_ALERT "PCI device enable successfully.\n");
			//dma
			pci_set_master(kyouko2.p_pci_dev);
		}
		else{
			printk(KERN_ALERT "PCI registration failed.\n");
			return flag;
		}
	}
	return flag;
}

/* The named function will be executed when the module is removed.
 */
static void __exit drawLineTriangle_exit(void){
	pci_unregister_driver(&kyouko2_pci_driver);
	cdev_del(&kyouko2_cdev);
	printk(KERN_ALERT "Exit.\n");
}

module_init(drawLineTriangle_init);
module_exit(drawLineTriangle_exit);

	
