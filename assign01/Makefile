PWD := $(CURDIR)

KMOD_ROOT := /lib/modules/$(shell uname -r)
EXTRA_MAKE_FLAG := LLVM=1

all:
	$(MAKE) $(EXTRA_MAKE_FLAG) -C $(KMOD_ROOT)/build M=$(PWD) modules

clean:
	$(MAKE) $(EXTRA_MAKE_FLAG) -C $(KMOD_ROOT)/build M=$(PWD) clean
