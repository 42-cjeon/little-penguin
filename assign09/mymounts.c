// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/list.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/nsproxy.h>
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#include <../fs/mount.h>

#define NODE_NAME "mymounts"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");
MODULE_DESCRIPTION("show all mount points");

/*
void * (*start) (struct seq_file *m, loff_t *pos);
	void (*stop) (struct seq_file *m, void *v);
	void * (*next) (struct seq_file *m, void *v, loff_t *pos);
	int (*show) (struct seq_file *m, void *v);

*/

static void *mymount_seq_start(struct seq_file *m, loff_t *pos) {
	struct list_head *head = m->private;

	return seq_list_start_head(head, *pos + 1);
}

static void *mymount_seq_next(struct seq_file *m, void *v, loff_t *pos) {
	struct list_head *head = m->private;
	pr_info("seq_next. pos=%ld\n", *pos);

	struct list_head *cur = seq_list_next(v, head, pos);
	pr_info("seq_next. end cur=[%p]\n", cur);
	
	return cur;
}

// unlock rcus?
void mymount_seq_stop(struct seq_file *m, void *v)
{
}

static int mymount_seq_show(struct seq_file *m, void *v) {
	struct mount *rmnt = container_of(v, struct mount, mnt_list);

	pr_info("rmnt addr == %p\n", rmnt);
	pr_info("rmnt devname p == %p\n", rmnt->mnt_devname);
	pr_info("rmnt devname n == %s\n", rmnt->mnt_devname ? rmnt->mnt_devname : "unknown");

	struct vfsmount *vmnt = &rmnt->mnt;
	struct super_block *sb = vmnt->mnt_sb;
	const struct path mnt_path = {
		.mnt = vmnt,
		.dentry = vmnt->mnt_root,
	};
	int ret;

	if (sb->s_op->show_devname) {
		ret = sb->s_op->show_devname(m, vmnt->mnt_root);
		if (ret)
			return ret;
	} else {
		seq_puts(m, rmnt->mnt_devname ? rmnt->mnt_devname : "unknown");
	}
	seq_putc(m, ' ');

	char buf[1<<10];
	char *bufp = d_path(&mnt_path, buf, ARRAY_SIZE(buf));
	seq_puts(m, bufp ? bufp : "unknown");

	// ret = seq_path(m, &mnt_path, " \t\n\\");
	
	seq_putc(m, '\n');

	return 0;
}

static struct seq_operations mymount_sops = {
	.start = mymount_seq_start,
	.next = mymount_seq_next,
	.show = mymount_seq_show,
	.stop = mymount_seq_stop,
};

// int (*open) (struct inode *, struct file *);
// ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
// loff_t (*llseek) (struct file *, loff_t, int);
// int (*release) (struct inode *, struct file *);

static int mymount_file_open(struct inode *inode, struct file *filp) {
	pr_info("open mymount...\n");

	struct nsproxy *nsp = current->nsproxy;
	struct mnt_namespace *mnt_ns = nsp->mnt_ns;
	struct seq_file *seq;
	int ret;

	ret = seq_open(filp, &mymount_sops);
	if (ret)
		return ret;

	seq = filp->private_data;
	seq->private = &mnt_ns->list;

	pr_info("open mymount...end\n");
	return 0;
}

// static int mymount_file_release(struct inode *inode, struct file *filp) {
// 	pr_info("release mymount...\n");
// 	int ret = seq_release_private(inode, filp);

// 	pr_info("release mymount...end ret=[%d]\n", ret);
// 	return ret;
// }

static const struct proc_ops mymount_fops = {
	.proc_open	= mymount_file_open,
        .proc_read	= seq_read,
        .proc_lseek	= seq_lseek,
        .proc_release	= seq_release,
};

static int __init init_mod(void)
{
	static struct proc_dir_entry *entry;
        
	entry = proc_create(NODE_NAME, 0, NULL, &mymount_fops);
	if (entry == NULL) {
		remove_proc_entry(NODE_NAME, NULL);
		return -ENOMEM;
	}
        return 0;
}

static void __exit exit_mod(void)
{
	remove_proc_entry(NODE_NAME, NULL);
}

module_init(init_mod);
module_exit(exit_mod);

	// very very very danger code.
	// struct nsproxy *nsp = current->nsproxy;
	// struct mnt_namespace *mnt_ns = nsp->mnt_ns;
	
	// struct mount *head = container_of(&mnt_ns->list, struct mount, mnt_list);

	// struct mount *curr;

	// struct path p;

	// char buf[1<<10];
	// list_for_each_entry(curr, &head->mnt_list, mnt_list) {
	// 	pr_info("mnt_info\n");
	// 	pr_info("  - devname   : %s\n", curr->mnt_devname ? curr->mnt_devname : "unknown");
		
	// 	p.dentry = curr->mnt.mnt_root;
	// 	p.mnt = &curr->mnt;

	// 	char *bufp = d_path(&p, buf, ARRAY_SIZE(buf));
	// 	pr_info("  - mounted on: %s\n", bufp ? bufp : "unknown");
	// }
