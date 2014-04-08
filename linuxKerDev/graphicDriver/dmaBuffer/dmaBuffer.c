#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/typecheck.h>
#include <linux/spinlock.h>
#include <asm/system.h>

#include "regdmaBuffer.h"
#include "macrodmaBuffer.h"

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("Shuai Wei");

typedef struct {
	unsigned long* buffer;
	unsigned int count;
	unsigned char isFull;
}dma_Buffer;

struct p_kyouko2_device{
	unsigned long p_control_base;
	unsigned long p_card_ram_base;
	unsigned int * k_control_base;
	unsigned int * k_card_ram_base;
	struct pci_dev * p_pci_dev;

	dma_Buffer dmaBuffer[BUFFERS];
	unsigned long userVirtAddr[BUFFERS];
	dma_addr_t dma_handle[BUFFERS];
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
	kyouko2.k_control_base = ioremap_nocache(kyouko2.p_control_base, KYOUKO2_CONTROL_SIZE);
	kyouko2.k_card_ram_base = ioremap_nocache(kyouko2.p_card_ram_base, KYOUKO2_CONTROL_SIZE);
	printk(KERN_ALERT "Open a device.\n");
	return 0;
}

static int kyouko2_release(struct inode* inode, struct file *fp){
	int i=0, ret=0;
	K_WRITE_REG(0x1000,1);
	iounmap((void*)kyouko2.k_control_base);
	iounmap((void*)kyouko2.k_card_ram_base);

	K_WRITE_REG(ConfigInterrupt, 0x00);

	free_irq(kyouko2.p_pci_dev->irq, &kyouko2);
	pci_disable_msi(kyouko2.p_pci_dev);
	for(i = 0; i < BUFFERS; ++i) {
		ret = do_munmap(current->mm, kyouko2.userVirtAddr[i], 124*1024);
		if(0 != ret)
			printk(KERN_ALERT "Not unmapped %d\n",i);
		pci_free_consistent(kyouko2.p_pci_dev, 124*1024, kyouko2.dmaBuffer[i].buffer, kyouko2.dma_handle[i]);
	}
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
	float value = 0.2;

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
	
	K_WRITE_REG_F(DrawClearColorR,value);
	K_WRITE_REG_F(DrawClearColorG,value);
	K_WRITE_REG_F(DrawClearColorB,value);
	K_WRITE_REG_F(DrawClearColorAlpha,0.0);

	writeflush();

	clearBuf_1();
}
	
static int fill = 0, drain = 0;
static DECLARE_WAIT_QUEUE_HEAD(dma_snooze);
static DEFINE_SPINLOCK(lock);

irqreturn_t dma_interrupt(int irq, void* devid, struct pt_regs* regs) {

	unsigned int flags;
	unsigned long lock_flags;
	flags = K_READ_REG(INFOSTATUS);
	K_WRITE_REG(INFOSTATUS, 0xFFFFFFFF); 
	
	if((flags & 0x2) == 0){
		return (IRQ_NONE);
	}
		
	if((flags & 0x01) == 0x01) 
		printk(KERN_ALERT "Interrupt on device error :/\n");
	
	spin_lock_irqsave(&lock, lock_flags);
	if(fill == drain) {
		if(1 == kyouko2.dmaBuffer[fill].isFull ) {
			//Buffer is full
			kyouko2.dmaBuffer[drain].isFull = 0;
			drain = (drain + 1) % BUFFERS;

			//Write request to DMA registers before calling wakeup
			K_WRITE_REG(STREAMBUFFERAADDRESS, kyouko2.dma_handle[drain]);
			K_WRITE_REG(STREAMBUFFERACONFIG, kyouko2.dmaBuffer[drain].count);

			spin_unlock_irqrestore(&lock, lock_flags);
			wake_up_interruptible(&dma_snooze);
		}
		else {
			printk(KERN_ALERT "Should not come here!! F%d D%d Dirty%d\n", fill, drain, kyouko2.dmaBuffer[drain].isFull);
			spin_unlock_irqrestore(&lock, lock_flags);
		}
	}
	
	//Buffer is not full
	else {
		kyouko2.dmaBuffer[drain].isFull = 0;
		drain = (drain + 1) % BUFFERS;
		//Check to only send in next request if fill != drain
		if(fill != drain) {
		
			sync();
			K_WRITE_REG(STREAMBUFFERAADDRESS, kyouko2.dma_handle[drain]);
			K_WRITE_REG(STREAMBUFFERACONFIG, kyouko2.dmaBuffer[drain].count);
		}
		spin_unlock_irqrestore(&lock, lock_flags);
	}

	return (IRQ_HANDLED);
}

static unsigned int size = 0;

void initiateTransfer(int numBytes) {

	unsigned long flags = 0;
	//local_irq_save(flags);
	if(0 == numBytes)
		return;

	spin_lock_irqsave(&lock, flags);


	if(fill == drain) {
		if(1 == kyouko2.dmaBuffer[fill].isFull) {
			//Buffer is full. Suspend
			spin_unlock_irqrestore(&lock, flags);
			wait_event_interruptible(dma_snooze, (fill != drain || kyouko2.dmaBuffer[fill].isFull == 0) );
		
			//Process waken up now. So queue the request
			spin_lock_irqsave(&lock, flags);
			kyouko2.dmaBuffer[fill].isFull = 1;
			kyouko2.dmaBuffer[fill].count = numBytes;

			fill = (fill+1) % BUFFERS;
			spin_unlock_irqrestore(&lock, flags);
		}
		else {
			//Buffer is empty. So write request to register
			kyouko2.dmaBuffer[fill].isFull = 1;
			kyouko2.dmaBuffer[fill].count = numBytes;

			K_WRITE_REG(STREAMBUFFERAADDRESS, (unsigned int)kyouko2.dma_handle[fill]);
			K_WRITE_REG(STREAMBUFFERACONFIG, numBytes);

			fill = (fill+1) % BUFFERS;
			spin_unlock_irqrestore(&lock, flags);
		}
	}

	//fill != drain
	else {
		//Space available in buffer. So queue in request
		kyouko2.dmaBuffer[fill].count = numBytes;
		kyouko2.dmaBuffer[fill].isFull = 1;
		
		fill = (fill+1) % BUFFERS;
		spin_unlock_irqrestore(&lock, flags);
	}
	
	//local_irq_restore(flags);
	return;
}



static long kyouko2_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
	int i = 0;
	long result = 0; 
	unsigned long notCopied = 0;
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

		case BIND_DMA:
			for(i = 0; i < BUFFERS; ++i) {
				kyouko2.dmaBuffer[i].buffer = (unsigned long *)pci_alloc_consistent(kyouko2.p_pci_dev, 124*1024, &kyouko2.dma_handle[i]);	
				if(NULL == kyouko2.dmaBuffer[i].buffer)
					printk(KERN_ALERT "No memory for %dth buffer!!\n", i);
				kyouko2.userVirtAddr[i] = do_mmap(fp, 0, 124*1024, PROT_READ|PROT_WRITE, MAP_SHARED, (unsigned long)((i+1)*0x1000));
				kyouko2.dmaBuffer[i].isFull = 0;
				kyouko2.dmaBuffer[i].count = 0;
			}

			notCopied = copy_to_user((unsigned long *)arg, &kyouko2.userVirtAddr[0], sizeof(unsigned long*));

			if(0 != notCopied)
				printk(KERN_ALERT "Hey %lu bytes not copied\n",notCopied );
			fill = 0;
			drain = 0;
	
			result = pci_enable_msi(kyouko2.p_pci_dev);
			if(0 != result)
				printk(KERN_ALERT "Int not enabled!\n");
			result = request_irq(kyouko2.p_pci_dev->irq, (irq_handler_t)dma_interrupt, IRQF_SHARED | IRQF_DISABLED, "dma_interrupt", &kyouko2);
			if(0 != result)
				printk(KERN_ALERT "Int not hooked!\n");
	
			K_WRITE_REG(INFOSTATUS, 0xFFFFFFFF);
			K_WRITE_REG(ConfigInterrupt, 0x2);
			break;

		case START_DMA:
 			notCopied = copy_from_user(&size, (const void*)arg, sizeof(unsigned int));						
			if(0 != notCopied)
				printk(KERN_ALERT "Hey %lu bytes not copied\n",notCopied );

			initiateTransfer(size);
			
			//Return next user virtual base addr here	
			notCopied = copy_to_user((unsigned long*)arg, &kyouko2.userVirtAddr[fill], sizeof(unsigned long*));

			if(0 != notCopied)
				printk(KERN_ALERT "Hey %lu bytes not copied\n",notCopied );
			break;

		case FLUSH:
			writeflush();
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
	else if(0x1000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[0] >> PAGE_SHIFT) , vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x2000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[1] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x3000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[2] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x4000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[3] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x5000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[4] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x6000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[5] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x7000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[6] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x8000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		flag = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[7] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else printk(KERN_ALERT "Should not come here!"); 
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
static int __init dmaBuffer_init(void){
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
static void __exit dmaBuffer_exit(void){
	pci_unregister_driver(&kyouko2_pci_driver);
	cdev_del(&kyouko2_cdev);
	printk(KERN_ALERT "Exit.\n");
}

module_init(dmaBuffer_init);
module_exit(dmaBuffer_exit);

	
