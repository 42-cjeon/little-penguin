NAME = hello.ko

PWD := $(CURDIR)

KMOD_ROOT = /lib/modules/$(shell uname -r)

$(NAME):
	$(MAKE) -C $(KMOD_ROOT)/build M=$(PWD) modules

all : $(NAME)

clean:
	$(MAKE) -C $(KMOD_ROOT)/build M=$(PWD) clean

install: $(NAME)
	$(MAKE) -C $(KMOD_ROOT)/build M=$(PWD) modules_install

uninstall:
	modprobe -r $(basename $(NAME))
	find $(KMOD_ROOT)/extra -name cjjjkjkjk$(NAME)* -print -delete
	depmod
