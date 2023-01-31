NAME = hello.ko

PWD := $(CURDIR)

KMOD_ROOT = /lib/modules/$(shell uname -r)
EXTRA_MAKE_FLAG := LLVM=1

all:
	$(MAKE) $(EXTRA_MAKE_FLAG) -C $(KMOD_ROOT)/build M=$(PWD) modules

clean:
	$(MAKE) $(EXTRA_MAKE_FLAG) -C $(KMOD_ROOT)/build M=$(PWD) clean

install:
	$(MAKE) $(EXTRA_MAKE_FLAG) -C $(KMOD_ROOT)/build M=$(PWD) modules_install

uninstall:
	modprobe -r $(basename $(NAME))
	find $(KMOD_ROOT)/extra -name $(NAME)* -print -delete
	depmod
