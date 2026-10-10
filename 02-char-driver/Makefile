ifneq ($(KERNELRELEASE),)
obj-m := sigbuf.o
else

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: modules sigbuf_test

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

sigbuf_test: sigbuf_test.c sigbuf_ioctl.h
	$(CC) -Wall -Wextra -O2 -o $@ $<

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f sigbuf_test

.PHONY: all modules clean
endif
