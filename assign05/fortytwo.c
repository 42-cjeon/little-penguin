// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#define DEVICE_NAME	"fortytwo"
#define LOGIN		"cjeon"
#define LOGIN_LEN	(ARRAY_SIZE(LOGIN) - 1)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");
MODULE_DESCRIPTION("simple i/o misc device");

static ssize_t ft_read(struct file *filp, char __user *ptr, size_t len, loff_t *off)
{
	return simple_read_from_buffer(ptr, len, off, LOGIN, LOGIN_LEN);
}

static ssize_t ft_write(struct file *filp, const char __user *ptr, size_t len, loff_t *off)
{
	char buf[LOGIN_LEN];

	if (len != LOGIN_LEN)
		return -EINVAL;

	if (copy_from_user(buf, ptr, len))
		return -EFAULT;

	if (memcmp(buf, LOGIN, LOGIN_LEN))
		return -EINVAL;

	return len;
}

static const struct file_operations fops = {
	.owner	=	THIS_MODULE,
	.read	=	ft_read,
	.write	=	ft_write
};

static struct miscdevice ft_dev = {
	.name	=	DEVICE_NAME,
	.minor	=	MISC_DYNAMIC_MINOR,
	.fops	=	&fops,
	.mode	=	0666
};

static int __init fortytwo_init(void)
{
	return misc_register(&ft_dev);
}

static void __exit fortytwo_exit(void)
{
	misc_deregister(&ft_dev);
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);
