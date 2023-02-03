// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/debugfs.h>

#include "fortytwo.h"

#define ENTRY_NAME "fortytwo"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");

struct dentry *entry;

static int __init fortytwo_init(void)
{
	struct dentry *temp;

	entry = debugfs_create_dir(ENTRY_NAME, NULL);
	if (IS_ERR(entry))
		return PTR_ERR(entry);

	temp = debugfs_create_file("id", 0666, entry, NULL, &fops_id);
	if (IS_ERR(temp))
		goto err_remove_enties;

	temp = debugfs_create_file("jiffies", 0444, entry, NULL, &fops_jiffies);
	if (IS_ERR(temp))
		goto err_remove_enties;

	temp = debugfs_create_file("foo", 0644, entry, NULL, &fops_foo);
	if (IS_ERR(temp))
		goto err_remove_enties;

	return 0;

err_remove_enties:
	debugfs_remove_recursive(entry);
	return PTR_ERR(temp);
}

static void __exit fortytwo_exit(void)
{
	debugfs_remove_recursive(entry);
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);
