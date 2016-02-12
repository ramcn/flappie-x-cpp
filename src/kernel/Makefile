CROSS_COMPILE = /usr/bin/arm-linux-gnueabihf-
#OUT_DIR=/home/ojomer/ws/tools/altera/kernel
ifeq ("$(KERNEL)","UBUNTU")
OUT_DIR=/home/ojomer/ws/tools/altera/gsrd/linux-socfpga.ubu14
else
OUT_DIR=/home/ojomer/ws/tools/altera/gsrd/linux-socfpga.git
endif

KDIR ?= $(OUT_DIR)
COMMON = CROSS_COMPILE=$(CROSS_COMPILE) ARCH=arm

#cache-v7.o
#obj-y += cache-v7.o cache-v7.o try.o
obj-m += vio_drv.o
vio_drv-objs :=  vio_drv_1.o 
CFLAGS_vio_drv.o += -Wno-int-to-pointer-cast

default:
	$(MAKE) -C $(KDIR) $(COMMON) M=$$PWD
	$(CROSS_COMPILE)g++ -o test test.c

clean:
	$(MAKE) -C $(KDIR) $(COMMON) M=$$PWD clean
	rm -f *.o

help:
	$(MAKE) -C $(KDIR) $(COMMON) M=$$PWD help

modules:
	$(MAKE) -C $(KDIR) $(COMMON) M=$$PWD modules

module_install:
	$(MAKE) -C $(KDIR) $(COMMON) M=$$PWD module_install
