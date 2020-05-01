EXTRA_CFLAGS += -g

obj-m := broken_module.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -f .*.dwo *dwo .*.cmd *.ko *.o *.mod.c modules.order Module.symvers
	rm -rf .tmp_versions
