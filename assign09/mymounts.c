// SPDX-License-Identifier: GPL-2.0-or-later

#include <linux/list.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/fs_struct.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Changmin Jeon <cjeon@student.42seoul.kr>");
MODULE_DESCRIPTION("simple device which reverses given string.");

int __init init_mod(void)
{
	struct fs_struct *fs = current->fs;
	struct path *root = &fs->root;
	struct vfsmount *mnt = root->mnt;
	struct super_block *sb = mnt->mnt_sb;
	struct super_block *cur;

	list_for_each_entry(cur, &sb->s_list, s_list) {
		const char *name = cur->s_root->d_name.name;
		pr_info("entry: [%s]\n", name);
	}
	return 0;
}

void __exit exit_mod(void)
{
}

module_init(init_mod);
module_exit(exit_mod);

// static DEFINE_MUTEX(namespace_lock);

// struct proc_mymounts {
// 	struct mnt_namespace *ns;
// 	struct path root;
// 	struct mount cursor;
// };

// static struct mount *mnt_list_next(struct mnt_namespace *ns,
// 				   struct list_head *p)
// {
// 	struct mount *mnt, *ret = NULL;

// 	spin_lock(&ns->ns_lock);
// 	spin_lock(&ns->ns_lock);
// 	list_for_each_continue(p, &ns->list) {
// 		mnt = list_entry(p, typeof(*mnt), mnt_list);
// 		if (!mnt_is_cursor(mnt)) {
// 			ret = mnt;
// 			break;
// 		}
// 	}
// 	unlock_ns_list(ns);

// 	return ret;
// }

// void *mymount_seq_start(struct seq_file *seq, loff_t *off) {
// 	struct proc_mymounts *p = seq->private;
// 	struct list_head *prev;
	
// 	mutex_lock(&namespace_lock);

// 	if (*off == 0) {
// 		prev = &p->ns->list;
// 	} else {
// 		prev = &p->cursor.mnt_list;

// 		if (list_empty(prev))
// 			return NULL;
// 	}

// 	return mnt_list_next();
// }
// // mymount_seq_next
// // mymount_seq_stop
// // mymount_seq_show

// const struct seq_operations mymount_sops = {
// 	.start =
// 	.next
// 	.stop
// 	.show
// };

// static int mymount_open(struct inode *inode, struct file *filp)
// {
// 	struct task_struct *task = get_proc_task(inode);
// 	struct nsproxy *nsp;
// 	struct mnt_namespace *ns = NULL;
// 	struct path root;
// 	struct proc_mymounts *p;
// 	struct seq_file *m;
// 	int ret;

// 	if (!task)
// 		return -EINVAL;
	
// 	task_lock(task);
// 	nsp = task->nsproxy;
// 	if (!nsp || !nsp->mnt_ns) {
// 		ret = -EINVAL;
// 		goto out_put_task;
// 	}
// 	ns = nsp->mnt_ns;
// 	get_mnt_ns(ns);

// 	if (!task->fs) {
// 		ret = -ENOENT;
// 		goto err_put_ns;
// 	}
// 	get_fs_root(task->fs, &root);

// 	p = __seq_open_private(filp, &mymount_sops, sizeof(struct proc_mymounts));
// 	if (!p) {
// 		ret = -ENOMEM;
// 		goto err_put_path;
// 	}

// 	p->ns = ns;
// 	p->root = root;
// 	INIT_LIST_HEAD(&p->cursor.mnt_list);
// 	p->cursor.mnt.mnt_flags = MNT_CURSOR;
// 	ret = 0;

// 	goto out_put_task;

// err_put_path:
// 	path_put(&root);

// err_put_ns:
// 	put_mnt_ns(ns);
	
// out_put_task:
// 	task_unlock(task);
// 	put_task_struct(task);

// 	return ret;
// }

// static int mymount_release(struct inode *inode, struct file *filp) {

// 	struct seq_file *m = filp->private_data;
// 	struct proc_mymounts *p = m->private;

// 	path_put(&p->root);
// 	mnt_cursor_del(p->ns, &p->cursor);
// 	put_mnt_ns(p->ns);

// 	// release filp->private_data->private
// 	return seq_release_private(inode, filp);
// }

// const struct file_operations mymount_fops = {
// 	.owner		= THIS_MODULE,
// 	.open		= mymount_open,
// 	.read_iter	= seq_read_iter,
// 	.llseek		= seq_lseek,
// 	.release	= mymount_release,
// };
