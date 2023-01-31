// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>

#define ENTRY_NAME	"fortytwo"
#define LOGIN		"cjeon"
#define LOGIN_END	ARRAY_SIZE(LOGIN)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");

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

	if (copy_to_user(ptr, &LOGIN[*off], read_size))
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

static struct file_operations fops_id = {
	.owner = THIS_MODULE,
	.read  = ft_read,
	.write = ft_write,
};

// =============================================================================

struct jiffies_buffer {
	char data[28];
	int end;
};

static ssize_t jiffies_read(struct file *filp, char __user *ptr, size_t len,
		       loff_t *off)
{
	const struct jiffies_buffer *buffer;
	ssize_t read_size = len;

	buffer = filp->private_data;
	if (*off == buffer->end) {
		*off = 0;
		return 0;
	}

	if (*off + len > buffer->end)
		read_size = buffer->end - *off;

	if (copy_to_user(ptr, buffer->data + *off, read_size))
		return -EFAULT;

	*off += read_size;
	return read_size;
}

static void reverse_mem(char *p, int len) {
	int left = 0;
	int right = len - 1;
	char tmp;

	while (left < right) {
		tmp      = p[left];
		p[left]  = p[right];
		p[right] = tmp;

		left++;
		right--;
	}
}

static void jiffies_to_buf(struct jiffies_buffer *buf) {
	unsigned long cur_jiffies = jiffies;

	buf->end = 0;
	
	do {
		buf->data[buf->end] = '0' + (cur_jiffies % 10);
		cur_jiffies /= 10;
		buf->end += 1;
	} while (cur_jiffies);

	reverse_mem(buf->data, buf->end);
}

static int jiffies_open(struct inode *inode, struct file *filp)
{
	struct jiffies_buffer *buffer;

	buffer = kmalloc(sizeof(*buffer), GFP_KERNEL);
	if (buffer == NULL)
		return -ENOMEM;

	jiffies_to_buf(buffer);
	filp->private_data = buffer;
	
	return 0;
}

static int jiffies_release(struct inode *inode, struct file *filp)
{
	if (filp->private_data) {
		kfree(filp->private_data);
		filp->private_data = NULL;
	}

	return 0;
}

static struct file_operations fops_jiffies = {
	.owner    = THIS_MODULE,
	.open     = jiffies_open,
	.release  = jiffies_release,
	.read     = jiffies_read,
};

// =============================================================================

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

	if (filp->f_mode & FMODE_WRITE) {		
		if (filp->f_flags & O_TRUNC) {
			foo_buffer_len = 0;
			foo_buffer_id++;	
		}

		foo_buffer_cur = 0;
	}

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

	if (foo_buffer_cur + len > FOO_BUFFER_MAX_SIZE) {
		write_size = FOO_BUFFER_MAX_SIZE - foo_buffer_cur; 
	}

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

static struct file_operations fops_foo = {
	.owner = THIS_MODULE,
	.open = foo_open,
	.read = foo_read,
	.write = foo_write,
};

// =============================================================================

struct dentry *entry;
struct dentry *files[3];

static int __init ft_debug_init(void)
{
	entry = debugfs_create_dir(ENTRY_NAME, NULL);
	if (IS_ERR(entry))
		return PTR_ERR(entry);

	files[0] = debugfs_create_file("id", 0666, entry, NULL, &fops_id);
	if (IS_ERR(files[0])) {
		debugfs_remove(entry);
		return PTR_ERR(files[0]);
	}

	files[1] = debugfs_create_file("jiffies", 0444, entry, NULL, &fops_jiffies);
	if (IS_ERR(files[1])) {
		debugfs_remove(entry);
		debugfs_remove(files[0]);
		return PTR_ERR(files[1]);
	}

	files[2] = debugfs_create_file("foo", 0644, entry, NULL, &fops_foo);
	if (IS_ERR(files[2])) {
		debugfs_remove(entry);
		debugfs_remove(files[0]);
		debugfs_remove(files[1]);
		return PTR_ERR(files[2]);
	}

	return 0;
}

static void __exit ft_debug_exit(void)
{
	for (int i = 0; i < ARRAY_SIZE(files); i++) {
		if (files[i])
			debugfs_remove(files[i]);
	}
	debugfs_remove(entry);
}

module_init(ft_debug_init);
module_exit(ft_debug_exit);
