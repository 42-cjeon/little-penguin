PWD := $(CURDIR)

KMOD_ROOT = /lib/modules/$(shell uname -r)

all:
	$(MAKE) -C $(KMOD_ROOT)/build M=$(PWD) modules

clean:
	$(MAKE) -C $(KMOD_ROOT)/build M=$(PWD) clean
