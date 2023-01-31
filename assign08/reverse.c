// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Louis Solofrizzo <louis@ne02ptzero.me>");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");
MODULE_DESCRIPTION("simple device which reverses given string.");

static ssize_t reverse_read(struct file *filp, char __user *ptr,
			    size_t len, loff_t *off);
static ssize_t reverse_write(struct file *filp, const char __user *ptr,
			     size_t len, loff_t *off);

static const struct file_operations reverse_fops = {
	.owner = THIS_MODULE,
	.read = &reverse_read,
	.write = &reverse_write,
};

static struct miscdevice reverse_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "reverse",
	.fops = &reverse_fops,
};

static char write_buf[PAGE_SIZE];
static loff_t write_buf_cur;
#define MAX_BUF_LEN ARRAY_SIZE(write_buf)

static DEFINE_MUTEX(write_buf_lock);

static int __init reverse_init(void)
{
	return misc_register(&reverse_device);
}

static void __exit reverse_cleanup(void)
{
	misc_deregister(&reverse_device);
}

static void mem_reverse_copy(char *dst, char *src, size_t len)
{
	size_t left = 0, right = len - 1;

	if (len == 0)
		return;

	while (left < right) {
		dst[right] = src[left];
		dst[left] = src[right];

		left++;
		right--;
	}

	/*
	 * if len is odd number,
	 * than left, right will meet in the middle.
	 * and that part is not copied yet.
	 * copy it.
	 */
	if (left == right)
		dst[left] = src[left];
}

ssize_t reverse_read(struct file *filp, char __user *ptr,
		     size_t len, loff_t *off)
{
	ssize_t ret;
	char *buf = NULL;

	mutex_lock(&write_buf_lock);

	buf = kmalloc_array(write_buf_cur, sizeof(*buf), GFP_KERNEL);
	if (buf == NULL) {
		ret = -ENOMEM;
		goto release_mutex;
	}

	mem_reverse_copy(buf, write_buf, write_buf_cur);
	ret = simple_read_from_buffer(ptr, len, off, buf, write_buf_cur);

	kfree(buf);

release_mutex:
	mutex_unlock(&write_buf_lock);

	return ret;
}

ssize_t reverse_write(struct file *filp, const char __user *ptr,
		      size_t len, loff_t *off)
{
	ssize_t ret;

	if (*off == MAX_BUF_LEN)
		return -ENOSPC;

	mutex_lock(&write_buf_lock);

	ret = simple_write_to_buffer(write_buf, MAX_BUF_LEN, off, ptr, len);
	write_buf_cur = *off;

	mutex_unlock(&write_buf_lock);

	return ret;
}

module_init(reverse_init);
module_exit(reverse_cleanup);
