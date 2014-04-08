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
#include "dataTypes.h"

MODULE_LICENSE("Proprietary");
MODULE_AUTHOR("RA_SH");


static int graphics_on = 0;
static int dma_mapped = 0;

struct kyouko_device_struc kyouko2;

void K_WRITE_REG(unsigned int reg, unsigned int value) {
	udelay(1);
	*(kyouko2.k_control_base + (reg>>2)) = value;
}

unsigned int K_READ_REG(unsigned int reg) {
	unsigned int value;
	udelay(1);
	rmb();
	value = *(kyouko2.k_control_base + (reg>>2));
	return value;
}

void SYNC(void) {
	while(K_READ_REG(FIFO_DEPTH) > 0 ) { }
}


int kyouko2_probe(struct pci_dev * pci_dev, const struct pci_device_id * pci_id){
	kyouko2.p_control_base = pci_resource_start(pci_dev,1);
	kyouko2.p_card_ram_base = pci_resource_start(pci_dev,2);
	kyouko2.p_pci_dev = pci_dev;	
	
	return 0;
}

void kyouko2_remove(struct pci_dev* dev) {
}

struct pci_driver kyouko2_pci_drv = {
	.name = "kyouko2_Driver",
	.id_table = kyouko2_dev_ids,
	.probe = kyouko2_probe,
	.remove = kyouko2_remove,
};

int kyouko2_open(struct inode *inode, struct file* fp) {

	printk(KERN_ALERT "Opened Device\n");
	kyouko2.k_control_base = ioremap_nocache(kyouko2.p_control_base, KYOUKO_CONTROL_SIZE);
	kyouko2.k_card_ram_base = ioremap_nocache(kyouko2.p_card_ram_base, KYOUKO_CONTROL_SIZE);

	return 0;
}

int kyouko2_release(struct inode* inode, struct file* fp) {
	
	unsigned int i = 0;
	int ret = 0;
	if(1 == dma_mapped) {
		K_WRITE_REG(INTERRUPT, 0x00);
		free_irq(kyouko2.p_pci_dev->irq, &kyouko2);
		pci_disable_msi(kyouko2.p_pci_dev);
		for(i = 0; i < NUM_BUFS; ++i) {
			ret = do_munmap(current->mm, kyouko2.userVirtAddr[i], 124*1024);
			if(0 != ret)
				printk(KERN_ALERT "Not unmapped %d\n",i);
			pci_free_consistent(kyouko2.p_pci_dev, 124*1024, kyouko2.DMA[i].DMA_buffer, kyouko2.dma_handle[i]);
		}
		dma_mapped = 0;
		K_WRITE_REG(REBOOT, 1);
	}

	iounmap((void*)kyouko2.k_control_base);
	iounmap((void*)kyouko2.k_card_ram_base);
	printk(KERN_ALERT "BUH BYEE\n");

	return 0;
}

int kyouko2_mmap(struct file* fp, struct vm_area_struct * vma) {
	int ret = 1;
	const int uid = current_fsuid();
	if(uid != 0) {
		printk(KERN_ALERT "Only root users can call mmap! %d\n", uid);
		return -1;
	}
	
	if(0x0 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start,(kyouko2.p_control_base >> PAGE_SHIFT) , vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x80000000 == (vma->vm_pgoff << PAGE_SHIFT))
		ret = io_remap_pfn_range(vma, vma->vm_start,(kyouko2.p_card_ram_base >> PAGE_SHIFT) , vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	
	else if(0x1000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[0] >> PAGE_SHIFT) , vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	
	else if(0x2000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[1] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x3000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[2] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x4000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[3] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x5000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[4] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x6000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[5] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x7000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[6] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else if(0x8000 == (vma->vm_pgoff << PAGE_SHIFT)) { 
		ret = io_remap_pfn_range(vma, vma->vm_start, (kyouko2.dma_handle[7] >> PAGE_SHIFT), vma->vm_end - vma->vm_start, vma->vm_page_prot); 
	}
	else {
	}
	return ret;
}

void setFrame0(void) {
	K_WRITE_REG(FRAME0_BASE, 1024);
	K_WRITE_REG(FRAME0_BASE+0x4, 768);
	K_WRITE_REG(FRAME0_BASE+0x8, 4096);
	K_WRITE_REG(FRAME0_BASE+0xc, 0x0000F888);
	K_WRITE_REG(FRAME0_BASE+0x10, 0);
}

void setDAC0(void) {
	K_WRITE_REG(DAC0_BASE, 1024);
	K_WRITE_REG(DAC0_BASE+0x4, 768);
	K_WRITE_REG(DAC0_BASE+0x8,0);
	K_WRITE_REG(DAC0_BASE+0xc,0);
	K_WRITE_REG(DAC0_BASE+0x10, 0);
}
void setClearColor(void) {
	float color = 0.2;
	K_WRITE_REG(CLEAR_COLOR, *(unsigned int*)&color);	
	K_WRITE_REG(CLEAR_COLOR+0x4, *(unsigned int*)&color);	
	K_WRITE_REG(CLEAR_COLOR+0x8, *(unsigned int*)&color);	
	K_WRITE_REG(CLEAR_COLOR+0xc, 0);	
}

static int fill = 0, drain = 0;
static DECLARE_WAIT_QUEUE_HEAD(dma_snooze);
static DEFINE_SPINLOCK(lock);

irqreturn_t dma_interrupt(int irq, void* devid, struct pt_regs* regs) {

	unsigned int flags;
	unsigned long lock_flags;
	flags = K_READ_REG(INTERRUPT_STATUS);
	K_WRITE_REG(INTERRUPT_STATUS, 0xFFFFFFFF); 
	
	if((flags & BUF_A_FLUSHED) == 0){
		return (IRQ_NONE);
	}
		
	if((flags & 0x01) == 0x01) 
		printk(KERN_ALERT "Stalled :/\n");
	
	spin_lock_irqsave(&lock, lock_flags);
	if(fill == drain) {
		if(1 == kyouko2.DMA[fill].isFull ) {
			//Buffer is full
			kyouko2.DMA[drain].isFull = 0;
			drain = (drain + 1) % NUM_BUFS;

			//Write request to DMA registers before calling wakeup
			K_WRITE_REG(DMA_ADDR_A, kyouko2.dma_handle[drain]);
			K_WRITE_REG(DMA_CONFIG_A, kyouko2.DMA[drain].count);

			spin_unlock_irqrestore(&lock, lock_flags);
			wake_up_interruptible(&dma_snooze);
		}
		else {
			printk(KERN_ALERT "Should not come here!! F%d D%d Dirty%d\n", fill, drain, kyouko2.DMA[drain].isFull);
			spin_unlock_irqrestore(&lock, lock_flags);
		}
	}
	
	//Buffer is not full
	else {
		kyouko2.DMA[drain].isFull = 0;
		drain = (drain + 1) % NUM_BUFS;
		//Check to only send in next request if fill != drain
		if(fill != drain) {
		
			SYNC();
			K_WRITE_REG(DMA_ADDR_A, kyouko2.dma_handle[drain]);
			K_WRITE_REG(DMA_CONFIG_A, kyouko2.DMA[drain].count);
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
		if(1 == kyouko2.DMA[fill].isFull) {
			//Buffer is full. Suspend
			spin_unlock_irqrestore(&lock, flags);
			wait_event_interruptible(dma_snooze, (fill != drain || kyouko2.DMA[fill].isFull == 0) );
		
			//Process waken up now. So queue the request
			spin_lock_irqsave(&lock, flags);
			kyouko2.DMA[fill].isFull = 1;
			kyouko2.DMA[fill].count = numBytes;

			fill = (fill+1) % NUM_BUFS;
			spin_unlock_irqrestore(&lock, flags);
		}
		else {
			//Buffer is empty. So write request to register
			kyouko2.DMA[fill].isFull = 1;
			kyouko2.DMA[fill].count = numBytes;

			K_WRITE_REG(DMA_ADDR_A, (unsigned int)kyouko2.dma_handle[fill]);
			K_WRITE_REG(DMA_CONFIG_A, numBytes);

			fill = (fill+1) % NUM_BUFS;
			spin_unlock_irqrestore(&lock, flags);
		}
	}

	//fill != drain
	else {
		//Space available in buffer. So queue in request
		kyouko2.DMA[fill].count = numBytes;
		kyouko2.DMA[fill].isFull = 1;
		
		fill = (fill+1) % NUM_BUFS;
		spin_unlock_irqrestore(&lock, flags);
	}
	
	//local_irq_restore(flags);
	return;
}

long kyouko2_ioctl(struct file* fp, unsigned int cmd, unsigned long arg) {
	int i = 0;
	long result = 0; 
	unsigned long notCopied = 0;

	switch(cmd) {
		case VMODE:
			if((int)arg == GRAPHICS_ON) {
				setFrame0();
				setDAC0();
				K_WRITE_REG(ACC_REG, 0x40000000);
				SYNC();
				K_WRITE_REG(MODESET,1);
				setClearColor();
				K_WRITE_REG(FLUSH,1);
				SYNC();
				K_WRITE_REG(CLEAR_BUF_REG,1);
				graphics_on = 1;
			}
			else {
				graphics_on = 0;
				SYNC();
				if(1 == dma_mapped) {
					for(i = 0; i < NUM_BUFS; i++) {
						if(1 == kyouko2.DMA[i].isFull)
							printk(KERN_ALERT "LEFT!! for %d\n", i);
						while(1 == kyouko2.DMA[i].isFull);
					}
					printk(KERN_ALERT "The END!! F%d D%d\n", fill, drain );
				}
				else
					K_WRITE_REG(REBOOT, 1);
			}
			break;

		case SYNC_CMD:
			SYNC();
			break;

		case BIND_DMA:
			for(i = 0; i < NUM_BUFS; ++i) {
				kyouko2.DMA[i].DMA_buffer = (TDMA_buffer *)pci_alloc_consistent(kyouko2.p_pci_dev, 124*1024, &kyouko2.dma_handle[i]);	
				if(NULL == kyouko2.DMA[i].DMA_buffer)
					printk(KERN_ALERT "No memory for %dth buffer!!\n", i);
				kyouko2.userVirtAddr[i] = do_mmap(fp, 0, 124*1024, PROT_READ|PROT_WRITE, MAP_SHARED, (unsigned long)((i+1)*0x1000));
				kyouko2.DMA[i].isFull = 0;
				kyouko2.DMA[i].count = 0;
			}

			dma_mapped = 1;
			notCopied = copy_to_user((TDMA_buffer*)arg, &kyouko2.userVirtAddr[0], sizeof(unsigned long*));

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
	
			K_WRITE_REG(INTERRUPT_STATUS, 0xFFFFFFFF);
			K_WRITE_REG(INTERRUPT, BUF_A_FLUSHED);
			break;

		case START_DMA:
			if(0 == dma_mapped) {
				printk(KERN_ALERT "BIND_DMA not done!\n");
				return -1;
			}
 			notCopied = copy_from_user(&size, (const void*)arg, sizeof(unsigned int));						
			if(0 != notCopied)
				printk(KERN_ALERT "Hey %lu bytes not copied\n",notCopied );

			initiateTransfer(size);
			
			//Return next user virtual base addr here	
			notCopied = copy_to_user((TDMA_buffer*)arg, &kyouko2.userVirtAddr[fill], sizeof(unsigned long*));

			if(0 != notCopied)
				printk(KERN_ALERT "Hey %lu bytes not copied\n",notCopied );
			break;

		case FLUSH_CMD:
			K_WRITE_REG(FLUSH,0);
			break;

		default:
			printk(KERN_ALERT "Incorrect IOCTL!!\n");
			break;
	}
	return result;
}

struct file_operations kyouko2_fops = {
	.open = kyouko2_open,
	.release = kyouko2_release,
	.unlocked_ioctl = kyouko2_ioctl,
	.mmap = kyouko2_mmap,
	.owner = THIS_MODULE,
};


struct cdev var;

int my_init_function(void) {

	int ret = 1;
	cdev_init(&var, &kyouko2_fops);
	ret = cdev_add(&var, MKDEV(500, 127), 1);

	if(ret == 0) {
		printk(KERN_ALERT "Module init done\n");
		ret = pci_register_driver(&kyouko2_pci_drv);
		if(0 == ret) {
			printk(KERN_ALERT "Pci register driver done\n");
			ret = pci_enable_device(kyouko2.p_pci_dev);
		}
		if(0 == ret) 
			printk(KERN_ALERT "Pci enable device done\n");

		pci_set_master(kyouko2.p_pci_dev);
	}
	else   printk(KERN_ALERT "Module init failed\n");


	return ret;
}

void my_exit_function(void) {

	pci_unregister_driver(&kyouko2_pci_drv);
	cdev_del(&var);

	printk(KERN_ALERT "Module exiting!\n");
}

module_init(my_init_function);

module_exit(my_exit_function);
