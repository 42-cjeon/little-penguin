// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/module.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/nsproxy.h>
#include <linux/fs_struct.h>
#include <linux/dcache.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/kprobes.h>

/* kernel internal header */
#include <../fs/mount.h>

#define NODE_NAME "mymounts"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");
MODULE_DESCRIPTION("show every mount points for given process");

static const struct seq_operations *p_mounts_op;
static int (*p_seq_path_root)(struct seq_file *m, const struct path *path,
			      const struct path *root, const char *esc);
static unsigned long (*p_kallsyms_lookup_name)(const char *name);
static int (*p_mounts_release)(struct inode *inode, struct file *file);

#define LOOKUP_SYMBOL_OR_RET(ptr, name)						\
	do {									\
		*(ptr) = (typeof(*(ptr)))p_kallsyms_lookup_name(name);		\
		if (*(ptr) == 0) {						\
			pr_alert("mymounts: cannot lookup symbol [%s]\n",	\
				 name);						\
			return -ENOENT;						\
		}								\
										\
	} while (0)

static int mymount_seq_show(struct seq_file *m, struct vfsmount *vmnt)
{
	struct proc_mounts *p = m->private;
	struct mount *rmnt = real_mount(vmnt);
	struct path mnt_path = { .dentry = vmnt->mnt_root, .mnt = vmnt };
	struct super_block *sb = mnt_path.dentry->d_sb;

	int err;

	if (sb->s_op->show_devname) {
		err = sb->s_op->show_devname(m, vmnt->mnt_root);
		if (err)
			return err;
	} else {
		seq_puts(m, rmnt->mnt_devname ? rmnt->mnt_devname : "unknown");
	}

	seq_putc(m, ' ');

	err = p_seq_path_root(m, &mnt_path, &p->root, " \t\n\\");
	if (err)
		return err;

	seq_putc(m, '\n');

	return 0;
}

static int get_proc_mounts(struct proc_mounts *pmnt)
{
	struct task_struct *task;
	struct nsproxy *nsp;
	struct mnt_namespace *mnt_ns;
	struct path root;
	int ret;

	task = get_task_struct(current);
	task_lock(task);

	if (!task->fs) {
		ret = -ENOENT;
		goto release_task_struct;
	}
	get_fs_root(task->fs, &root);

	nsp = task->nsproxy;
	if (!nsp || !nsp->mnt_ns) {
		ret = -EINVAL;
		goto release_path;
	}
	mnt_ns = nsp->mnt_ns;
	get_mnt_ns(mnt_ns);

	pmnt->ns = mnt_ns;
	pmnt->root = root;
	pmnt->show = mymount_seq_show;
	INIT_LIST_HEAD(&pmnt->cursor.mnt_list);
	pmnt->cursor.mnt.mnt_flags = MNT_CURSOR;

	ret = 0;
	goto release_task_struct;

release_path:
	path_put(&root);

release_task_struct:
	task_unlock(task);
	put_task_struct(task);

	return ret;
}

static int mymount_proc_open(struct inode *inode, struct file *filp)
{
	struct proc_mounts *pmnt;
	int ret;

	pmnt = __seq_open_private(filp, p_mounts_op, sizeof(*pmnt));
	if (pmnt == NULL)
		return -ENOMEM;

	ret = get_proc_mounts(pmnt);
	if (ret)
		seq_release_private(inode, filp);

	return ret;
}

static struct proc_ops mymount_pops = {
	.proc_open = mymount_proc_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	/* .proc_release will set during init_mymount. */
};

int lookup_kallsyms_look_name(void)
{
	struct kprobe kp = { .symbol_name = "kallsyms_lookup_name" };
	int err;

	err = register_kprobe(&kp);
	if (err) {
		pr_alert(
			"mymounts: cannot lookup kallsyms_lookup_name. err was [%d].\n",
			err);
		return err;
	}
	p_kallsyms_lookup_name = (typeof(p_kallsyms_lookup_name))kp.addr;
	unregister_kprobe(&kp);

	return 0;
}

static int __init init_mymount(void)
{
	static struct proc_dir_entry *entry;
	int err;

	err = lookup_kallsyms_look_name();
	if (err)
		return err;

	LOOKUP_SYMBOL_OR_RET(&p_mounts_op, "mounts_op");
	LOOKUP_SYMBOL_OR_RET(&p_seq_path_root, "seq_path_root");
	LOOKUP_SYMBOL_OR_RET(&p_mounts_release, "mounts_release");

	mymount_pops.proc_release = p_mounts_release;

	entry = proc_create(NODE_NAME, 0, NULL, &mymount_pops);
	if (entry == NULL) {
		remove_proc_entry(NODE_NAME, NULL);
		return -ENOMEM;
	}

	return 0;
}

static void __exit exit_mymount(void)
{
	remove_proc_entry(NODE_NAME, NULL);
}

module_init(init_mymount);
module_exit(exit_mymount);
