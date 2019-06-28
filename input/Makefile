#ubuntu的内核源码树，如果要编译在ubuntu中安装的模块就打开这2个
#KERN_VER = $(shell uname -r)
#KERN_DIR = /lib/modules/$(KERN_VER)/build


# 开发板的linux内核的源码树目录
KERN_DIR = ~/Embedded/kernel/x210_kernel

obj-m	+= x210-key.o

all:
	make -C $(KERN_DIR) M=`pwd` modules
	arm-linux-gcc key_app.c -o key_app.o
	cp *.ko -f ~/Embedded/nfs/rootfs/root/driver
	cp key_app.o -f ~/Embedded/nfs/rootfs/root/driver

.PHONY: clean
clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
