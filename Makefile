ifneq ($(KERNELRELEASE),)

obj-m := \
	hello.o \
	simplei2c.o \
	fxos8700.o

else

KDIR ?= /lib/modules/`uname -r`/build

default:
	reset
	$(MAKE) -C $(KDIR) M=$$PWD
	$(CC) -Wall -lm -g read-fxos8700.c -o read-fxos8700

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

endif
