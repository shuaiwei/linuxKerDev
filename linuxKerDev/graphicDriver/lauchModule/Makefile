#
#makefile
#
obj-m += launchModule.o

default:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	rm *.ko
	rm *.o
	rm *.mod.c

