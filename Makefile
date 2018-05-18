# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
CONFIG_MODULE_SIG=n
ifneq ($(KERNELRELEASE),)
	#chap4-m	:= local_managment.o chap4.o 
	obj-m	:= chap4.o
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod.c *.order *.symvers *.cmd .chap4* .local_managment*
endif
insmod:
	sudo insmod chap4.ko
rmmod:
	sudo rmmod chap4
