// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mutex.h>

static char foo_buffer[PAGE_SIZE];
static uintptr_t foo_buffer_id;
static loff_t foo_buffer_len;
static loff_t foo_buffer_cur;

#define FOO_BUFFER_MAX_SIZE ARRAY_SIZE(foo_buffer)

static DEFINE_MUTEX(foo_lock);

static int foo_open(struct inode *inode, struct file *filp)
{
	mutex_lock(&foo_lock);

	filp->private_data = (void *)foo_buffer_id;

	spin_lock(&filp->f_lock);
	if (filp->f_mode & FMODE_WRITE) {
		if (filp->f_flags & O_TRUNC) {
			foo_buffer_len = 0;
			foo_buffer_id++;
		}

		foo_buffer_cur = 0;
	}
	spin_unlock(&filp->f_lock);

	mutex_unlock(&foo_lock);

	return 0;
}

static ssize_t foo_read(struct file *filp, char __user *ptr, size_t len,
			loff_t *off)
{
	size_t read_size = len;
	ssize_t ret;

	mutex_lock(&foo_lock);

	/*
	 *  The contents of buffer changed after last read call.
	 *  so read from the begining
	 */
	if ((uintptr_t)filp->private_data != foo_buffer_id) {
		*off = 0;
		filp->private_data = (void *)foo_buffer_id;
	}

	if (*off == foo_buffer_len) {
		ret = 0;
		goto release;
	}

	if (*off + len > foo_buffer_len)
		read_size = foo_buffer_len - *off;

	if (copy_to_user(ptr, foo_buffer + *off, read_size)) {
		ret = -EFAULT;
		goto release;
	}

	*off += read_size;
	ret = read_size;

release:
	mutex_unlock(&foo_lock);

	return ret;
}

static ssize_t foo_write(struct file *filp, const char __user *ptr, size_t len,
			 loff_t *off)
{
	size_t write_size = len;
	ssize_t ret;

	if (len == 0)
		return 0;

	mutex_lock(&foo_lock);

	if (foo_buffer_cur == FOO_BUFFER_MAX_SIZE) {
		ret = -ENOSPC;
		goto release;
	}

	if (foo_buffer_cur == 0)
		foo_buffer_id++;

	spin_lock(&filp->f_lock);
	if (filp->f_mode & FMODE_WRITE && filp->f_flags & O_APPEND)
		foo_buffer_cur = foo_buffer_len;
	spin_unlock(&filp->f_lock);

	if (foo_buffer_cur + len > FOO_BUFFER_MAX_SIZE)
		write_size = FOO_BUFFER_MAX_SIZE - foo_buffer_cur;

	if (copy_from_user(foo_buffer + foo_buffer_cur, ptr, write_size)) {
		ret = -EFAULT;
		goto release;
	}

	foo_buffer_cur += write_size;
	foo_buffer_len = max(foo_buffer_len, foo_buffer_cur);
	ret = write_size;

release:
	mutex_unlock(&foo_lock);

	return ret;
}

const struct file_operations fops_foo = {
	.owner = THIS_MODULE,
	.open = foo_open,
	.read = foo_read,
	.write = foo_write,
};
