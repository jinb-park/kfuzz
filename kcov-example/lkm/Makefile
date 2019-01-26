obj-m := poc_lkm.o

KVERSION = $(shell uname -r)
#KDIR := /lib/modules/$(KVERSION)/build
KDIR := /home/jinbum/github/linux

ccflags-y += -DKCOV

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(shell pwd) modules

clean:
	rm -rf *.o *.ko *.mod *.symvers *.order *.mod.c .*cmd .tmp*
