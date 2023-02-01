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
#include <linux/kprobes.h>

#include <../fs/mount.h>

#define NODE_NAME "mymounts"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");
MODULE_DESCRIPTION("show all mount points");

int (*p_seq_path_root)(struct seq_file *m, const struct path *path,
		  const struct path *root, const char *esc);

struct list_head *mount_list_next_n(struct list_head *head, loff_t pos) {
	struct mount *curr;

	list_for_each_entry(curr, head, mnt_list) {
		if (curr->mnt.mnt_flags & MNT_CURSOR)
			continue;
		if (pos == 0)
			return &curr->mnt_list;
		--pos;
	}

	return NULL;
}

struct list_head *mount_list_next(struct list_head *head, struct list_head *curr, loff_t *pos) {
	struct mount *rmnt;

	*pos += 1;

	list_for_each_continue(curr, head) {
		rmnt = container_of(curr, struct mount, mnt_list);
		if (rmnt->mnt.mnt_flags & MNT_CURSOR)
			continue;
		return &rmnt->mnt_list;
	}

	return NULL;
}

struct proc_mymounts {
	struct path *root;
	struct list_head *head;
};

static void *mymount_seq_start(struct seq_file *m, loff_t *pos)
{
	struct proc_mymounts *mymnt = m->private;

	return mount_list_next_n(mymnt->head, *pos);
}

static void *mymount_seq_next(struct seq_file *m, void *v, loff_t *pos)
{
	struct proc_mymounts *mymnt = m->private;
	struct list_head *curr = v;

	return mount_list_next(mymnt->head, curr, pos);
}

void mymount_seq_stop(struct seq_file *m, void *v)
{
}

static int mymount_seq_show(struct seq_file *m, void *v)
{
	struct proc_mymounts *mymnt = m->private;
	struct mount *rmnt = container_of(v, struct mount, mnt_list);
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

	ret = p_seq_path_root(m, &mnt_path, mymnt->root, " \t\n\\");
	if (ret)
		return ret;

	seq_putc(m, ' ');

	// pr_alert("MNT_CURSOR=%s\n", (vmnt->mnt_flags & MNT_CURSOR) ? "true" : "false");

	seq_putc(m, '\n');

	return 0;
}

static struct seq_operations mymount_sops = {
	.start = mymount_seq_start,
	.next = mymount_seq_next,
	.show = mymount_seq_show,
	.stop = mymount_seq_stop,
};

static int mymount_file_open(struct inode *inode, struct file *filp)
{
	struct nsproxy *nsp;
	struct mnt_namespace *mnt_ns;
	struct proc_mymounts *mymounts;

	nsp = current->nsproxy;
	mnt_ns = nsp->mnt_ns;

	mymounts = __seq_open_private(filp, &mymount_sops, sizeof(*mymounts));
	if (mymounts == NULL)
		return -ENOMEM;

	mymounts->head = &mnt_ns->list;
	mymounts->root = &current->fs->root;

	return 0;
}

// static int mymount_file_release(struct inode *inode, struct file *filp) {
// 	pr_info("release mymount...\n");
// 	int ret = seq_release_private(inode, filp);

// 	pr_info("release mymount...end ret=[%d]\n", ret);
// 	return ret;
// }

static const struct proc_ops mymount_fops = {
	.proc_open = mymount_file_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = seq_release_private,
};

static int __init init_mod(void)
{
	static struct proc_dir_entry *entry;
	struct kprobe kp = {
		.symbol_name = "seq_path_root"
	};
	int err;

	err = register_kprobe(&kp);
	if (err)
		return err;
	p_seq_path_root = (typeof(p_seq_path_root))kp.addr;
	unregister_kprobe(&kp);

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
