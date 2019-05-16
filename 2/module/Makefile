obj-m	:= huins.o

KDIR	:= /work/achroimx_kernel
PWD	:= $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	rm -rf *.o
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf *.order
	rm -rf *.symvers
	rm -rf .tmp*
	rm -rf .huins.*
