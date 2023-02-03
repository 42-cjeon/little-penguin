// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/fs.h>

#define LOGIN		"cjeon"
#define LOGIN_LEN	(ARRAY_SIZE(LOGIN) - 1)

static ssize_t id_read(struct file *filp, char __user *ptr, size_t len,
		       loff_t *off)
{
	return simple_read_from_buffer(ptr, len, off, LOGIN, LOGIN_LEN);
}

static ssize_t id_write(struct file *filp, const char __user *ptr, size_t len,
			loff_t *off)
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

const struct file_operations fops_id = {
	.owner = THIS_MODULE,
	.read = id_read,
	.write = id_write,
};
