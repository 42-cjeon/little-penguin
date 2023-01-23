NAME = fortytwo.ko

PWD := $(CURDIR)

KMOD_ROOT = /lib/modules/$(shell uname -r)

$(NAME):
	$(MAKE) -C $(KMOD_ROOT)/build M=$(PWD) modules

all : $(NAME)

clean:
	$(MAKE) -C $(KMOD_ROOT)/build M=$(PWD) clean
