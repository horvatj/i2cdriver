ifneq ($(KERNELRELEASE),)
obj-m := \
	hello.o \
	simplei2c.o
else
KDIR ?= /lib/modules/`uname -r`/build

default:
	reset
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

endif
