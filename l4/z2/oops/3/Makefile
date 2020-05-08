EXTRA_CFLAGS += -g -Wno-incompatible-pointer-types -Wno-int-to-pointer-cast

obj-m := broken_module.o

KDIR  := /lib/modules/$(shell uname -r)/build
PWD   := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

