#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#define DEVICE_NAME "fortytwo"

#define LOGIN "cjeon"
#define LOGIN_END ARRAY_SIZE(LOGIN)

static ssize_t ft_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t ft_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations fops = { .owner = THIS_MODULE,
				       .read = ft_read,
				       .write = ft_write };

static struct miscdevice ft_dev = { .minor = MISC_DYNAMIC_MINOR,
				    .name = DEVICE_NAME,
				    .fops = &fops,
				    .mode = 0666 };

static ssize_t ft_read(struct file *filp, char __user *ptr, size_t len,
		       loff_t *off)
{
	size_t read_size = len;
	
    if (*off == LOGIN_END) {
		*off = 0;
		return 0;
	}

	if (*off + len > LOGIN_END)
		read_size = LOGIN_END - *off;

	if (copy_to_user(ptr, LOGIN, read_size))
		return -EFAULT;

	*off += read_size;
	return read_size;
}

static ssize_t ft_write(struct file *filp, const char __user *ptr, size_t len,
			loff_t *off)
{
	char buf[LOGIN_END - 1];

	if (len != LOGIN_END - 1)
		return -EINVAL;

	if (copy_from_user(buf, ptr, len))
		return -EFAULT;

	if (strncmp(buf, LOGIN, LOGIN_END - 1))
		return -EINVAL;

	return len;
}

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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");
MODULE_DESCRIPTION("simple i/o misc device");