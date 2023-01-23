#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define SUCCESS 0

int __init hello_init(void)
{
	pr_info("Hello world !\n");

	return SUCCESS;
}

void __exit hello_exit(void)
{
	pr_info("Cleaning up module.\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cjeon@student.42seoul.kr");
MODULE_DESCRIPTION("simple `hello, world` module.");