# If KERNELRELEASE is defined, we've been invoked from the
# kernel build system and can use its language.
CONFIG_MODULE_SIG=n
ifneq ($(KERNELRELEASE),)
	obj-m := chap3.o
# Otherwise we were called directly from the command
# line; invoke the kernel build system.
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod.c *.order *.symvers *.cmd .chap3*
endif
insmod:
	sudo insmod chap3.ko
rmmod:
	sudo rmmod chap3
