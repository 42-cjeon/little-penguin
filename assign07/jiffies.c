// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/jiffies.h>

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

static void reverse_mem(char *p, int len)
{
	int left = 0;
	int right = len - 1;
	char tmp;

	while (left < right) {
		tmp = p[left];
		p[left] = p[right];
		p[right] = tmp;

		left++;
		right--;
	}
}

static void jiffies_to_buf(struct jiffies_buffer *buf)
{
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
	kfree(filp->private_data);
	filp->private_data = NULL;

	return 0;
}

const struct file_operations fops_jiffies = {
	.owner = THIS_MODULE,
	.open = jiffies_open,
	.release = jiffies_release,
	.read = jiffies_read,
};
