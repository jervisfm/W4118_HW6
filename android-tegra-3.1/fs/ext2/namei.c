/*
 * linux/fs/ext2/namei.c
 *
 * Rewrite to pagecache. Almost all code had been changed, so blame me
 * if the things go wrong. Please, send bug reports to
 * viro@parcelfarce.linux.theplanet.co.uk
 *
 * Stuff here is basically a glue between the VFS and generic UNIXish
 * filesystem that keeps everything in pagecache. All knowledge of the
 * directory layout is in fs/ext2/dir.c - it turned out to be easily separatable
 * and it's easier to debug that way. In principle we might want to
 * generalize that a bit and turn it into a library. Or not.
 *
 * The only non-static object here is ext2_dir_inode_operations.
 *
 * TODO: get rid of kmap() use, add readahead.
 *
 * Copyright (C) 1992, 1993, 1994, 1995
 * Remy Card (card@masi.ibp.fr)
 * Laboratoire MASI - Institut Blaise Pascal
 * Universite Pierre et Marie Curie (Paris VI)
 *
 *  from
 *
 *  linux/fs/minix/namei.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Big-endian to little-endian byte-swapping/bitmaps by
 *        David S. Miller (davem@caip.rutgers.edu), 1995
 */

#include <linux/pagemap.h>
#include <linux/quotaops.h>
#include "ext2.h"
#include "xattr.h"
#include "acl.h"
#include "xip.h"

static inline int ext2_add_nondir(struct dentry *dentry, struct inode *inode)
{
	int err = ext2_add_link(dentry, inode);
	if (!err) {
		d_instantiate(dentry, inode);
		unlock_new_inode(inode);
		return 0;
	}
	inode_dec_link_count(inode);
	unlock_new_inode(inode);
	iput(inode);
	return err;
}

/*
 * Methods themselves.
 */

static struct dentry *ext2_lookup(struct inode * dir, struct dentry *dentry, struct nameidata *nd)
{
	struct inode * inode;
	ino_t ino;
	
	if (dentry->d_name.len > EXT2_NAME_LEN)
		return ERR_PTR(-ENAMETOOLONG);

	ino = ext2_inode_by_name(dir, &dentry->d_name);
	inode = NULL;
	if (ino) {
		inode = ext2_iget(dir->i_sb, ino);
		if (inode == ERR_PTR(-ESTALE)) {
			ext2_error(dir->i_sb, __func__,
					"deleted inode referenced: %lu",
					(unsigned long) ino);
			return ERR_PTR(-EIO);
		}
	}
	return d_splice_alias(inode, dentry);
}

struct dentry *ext2_get_parent(struct dentry *child)
{
	struct qstr dotdot = {.name = "..", .len = 2};
	unsigned long ino = ext2_inode_by_name(child->d_inode, &dotdot);
	if (!ino)
		return ERR_PTR(-ENOENT);
	return d_obtain_alias(ext2_iget(child->d_inode->i_sb, ino));
} 

/*
 * By the time this is called, we already have created
 * the directory cache entry for the new file, but it
 * is so far negative - it has no inode.
 *
 * If the create succeeds, we fill in the inode information
 * with d_instantiate(). 
 */
static int ext2_create (struct inode * dir, struct dentry * dentry, int mode, struct nameidata *nd)
{
	struct inode *inode;
	int ret;

	dquot_initialize(dir);

	inode = ext2_new_inode(dir, mode, &dentry->d_name);
	if (IS_ERR(inode))
		return PTR_ERR(inode);

	/* Set GPS information and warn if not successful */
	ret = ext2_set_gps(inode);
	WARN_ON(ret != 0);


	inode->i_op = &ext2_file_inode_operations;
	if (ext2_use_xip(inode->i_sb)) {
		inode->i_mapping->a_ops = &ext2_aops_xip;
		inode->i_fop = &ext2_xip_file_operations;
	} else if (test_opt(inode->i_sb, NOBH)) {
		inode->i_mapping->a_ops = &ext2_nobh_aops;
		inode->i_fop = &ext2_file_operations;
	} else {
		inode->i_mapping->a_ops = &ext2_aops;
		inode->i_fop = &ext2_file_operations;
	}
	mark_inode_dirty(inode);
	return ext2_add_nondir(dentry, inode);
}

/*TODO: Implement these fucntions and test them out */
/* Uses the latest gps information from the kernel and saves it to the
 * inode.
 * Returns 0 on success,  < 0 (i.e. -ve) on failure.   */
int ext2_set_gps(struct inode *inode)
{
	struct timespec now, age;
	unsigned int age_in_seconds;
	__u64 lat = 0, lng = 0;
	__u32 accuracy = 0;

	/* stores the current kernel gps */
	struct kernel_gps k_gps;
	/* the gps info in the inode */
	struct gps_on_disk *inode_gps = NULL;

	struct ext2_inode_info *inode_in_ram = EXT2_I(inode);

	if (inode_in_ram == NULL)
		return -EINVAL;

	get_current_location(&k_gps);
	inode_gps = &inode_in_ram->i_gps;

	BUG_ON(inode_gps == NULL);

	/* TODO:  Test and see if the double bits
	 * are correctly preserved.  Need to verify
	 * the C-hackery, we do below actually works in practice.
	 */

	/*
	 * Following the typedef declarations,
	 * le64 == unsigned long long.
	 * Should not use le64* directly, b'se that won't do
	 * what you think it does (since le64 is a typedef).
	 *
	 * We do this C-hackery here because the kernel does not
	 * support floating points directly.
	 *
	 * Also note, that we don't convert to __le64 here,
	 * that is taken care of when this inode is acutally written
	 * to disk by __write_inode in inode.c
	 */
	write_lock(&inode_in_ram->i_gps_lock);
	lat = (*((unsigned long long *)&k_gps.loc.latitude));
	lng = (*((unsigned long long *)&k_gps.loc.longitude));
	accuracy = (*((unsigned int *)&k_gps.loc.accuracy));

	inode_gps->latitude = lat;
	inode_gps->longitude = lng;
	inode_gps->accuracy = accuracy;

	/* Set the timestamp for the gps information. We compute the age
	 * now and store this information so that we can directly write it
	 * in when we do the actual write of the inode to disk in
	 * __write_inode() function in inode.c.
	 */
	getnstimeofday(&now);
	age.tv_sec = k_gps.timestamp.tv_sec;
	/*age.tv_sec = age.tv_sec < 0 ? -age.tv_sec : age.tv_sec;*/
	age_in_seconds = (unsigned int) age.tv_sec;
	inode_gps->age = cpu_to_le32(age_in_seconds);
	/* Mark Inode dirty.
	 * Okay, this if-condition is a bit wierd but from my testing it's
	 * the only that works properly and does not cause file system
	 * corruption.
	 * TODO: Investigate why this hack is necessary.  */
	if (inode->i_state & I_DIRTY)
		mark_inode_dirty(inode);

	/* inode->i_sb->s_op->dirty_inode(inode, I_DIRTY_SYNC); */
	write_unlock(&inode_in_ram->i_gps_lock);
	return 0;
}

/* Reads back the gps information stored in the given inode. */
int ext2_get_gps(struct inode *inode, struct gps_location *loc)
{
	/* TODO: still to be implemented */
	struct inode *ext_inode = NULL;
	struct ext2_inode_info *ei = NULL;
	unsigned int age = 0;
	long temp;
	if (loc == NULL || inode == NULL)
		return -EINVAL;

	/*
	 * Load the In Memory struct of the ext_inode
	 * with the given number by using the ext2_iget function.
	 *
	 * TODO: Is syncing an issue ? I am reading from RAM directly
	 * and I think that should be OK.
	 */
	ext_inode = ext2_iget(inode->i_sb, inode->i_ino);
	ei = EXT2_I(ext_inode);

	/* From my study of the code, all of the *inode structures
	 * should be embedded with the ext2_inode_info structure.
	 * Let's just do a test sanity check to make verify this.
	 * (I'm actually not using this assumption btw) */
	WARN_ON(&ei->vfs_inode != inode);


	/* How to get Inode from DISK.
	struct super_block *sb = inode->i_sb;
	ino_t ino = inode->i_ino;
	uid_t uid = inode->i_uid;
	gid_t gid = inode->i_gid;
	struct buffer_head *bh;
	struct ext2_inode *raw_inode = ext2_get_inode(sb, ino, &bh);
	*/

	/*
	 * Use C pointer hackery again, to convert the stored bits
	 * back to a double.
	 */
	read_lock(&ei->i_gps_lock);
	loc->latitude = *((double *)(&ei->i_gps.latitude));
	loc->longitude = *((double *)(&ei->i_gps.longitude));
	loc->accuracy = *((float *)(&ei->i_gps.accuracy));
	age = *((unsigned int *)(&ei->i_gps.age));
	temp = (long) age;
	temp = get_seconds() - temp;
	read_unlock(&ei->i_gps_lock);
	return (int) temp;
}

static int ext2_mknod (struct inode * dir, struct dentry *dentry, int mode, dev_t rdev)
{
	struct inode * inode;
	int err;

	if (!new_valid_dev(rdev))
		return -EINVAL;

	dquot_initialize(dir);

	inode = ext2_new_inode (dir, mode, &dentry->d_name);
	err = PTR_ERR(inode);
	if (!IS_ERR(inode)) {
		init_special_inode(inode, inode->i_mode, rdev);
#ifdef CONFIG_EXT2_FS_XATTR
		inode->i_op = &ext2_special_inode_operations;
#endif
		mark_inode_dirty(inode);
		err = ext2_add_nondir(dentry, inode);
	}
	return err;
}

static int ext2_symlink (struct inode * dir, struct dentry * dentry,
	const char * symname)
{
	struct super_block * sb = dir->i_sb;
	int err = -ENAMETOOLONG;
	unsigned l = strlen(symname)+1;
	struct inode * inode;

	if (l > sb->s_blocksize)
		goto out;

	dquot_initialize(dir);

	inode = ext2_new_inode (dir, S_IFLNK | S_IRWXUGO, &dentry->d_name);
	err = PTR_ERR(inode);
	if (IS_ERR(inode))
		goto out;

	if (l > sizeof (EXT2_I(inode)->i_data)) {
		/* slow symlink */
		inode->i_op = &ext2_symlink_inode_operations;
		if (test_opt(inode->i_sb, NOBH))
			inode->i_mapping->a_ops = &ext2_nobh_aops;
		else
			inode->i_mapping->a_ops = &ext2_aops;
		err = page_symlink(inode, symname, l);
		if (err)
			goto out_fail;
	} else {
		/* fast symlink */
		inode->i_op = &ext2_fast_symlink_inode_operations;
		memcpy((char*)(EXT2_I(inode)->i_data),symname,l);
		inode->i_size = l-1;
	}
	mark_inode_dirty(inode);

	err = ext2_add_nondir(dentry, inode);
out:
	return err;

out_fail:
	inode_dec_link_count(inode);
	unlock_new_inode(inode);
	iput (inode);
	goto out;
}

static int ext2_link (struct dentry * old_dentry, struct inode * dir,
	struct dentry *dentry)
{
	struct inode *inode = old_dentry->d_inode;
	int err;

	if (inode->i_nlink >= EXT2_LINK_MAX)
		return -EMLINK;

	dquot_initialize(dir);

	inode->i_ctime = CURRENT_TIME_SEC;
	/* update gps info */
	ext2_set_gps(inode);
	inode_inc_link_count(inode);
	ihold(inode);

	err = ext2_add_link(dentry, inode);
	if (!err) {
		d_instantiate(dentry, inode);
		return 0;
	}
	inode_dec_link_count(inode);
	iput(inode);
	return err;
}

static int ext2_mkdir(struct inode * dir, struct dentry * dentry, int mode)
{
	struct inode * inode;
	int err = -EMLINK;

	if (dir->i_nlink >= EXT2_LINK_MAX)
		goto out;

	dquot_initialize(dir);

	inode_inc_link_count(dir);

	inode = ext2_new_inode(dir, S_IFDIR | mode, &dentry->d_name);
	err = PTR_ERR(inode);
	if (IS_ERR(inode))
		goto out_dir;

	inode->i_op = &ext2_dir_inode_operations;
	inode->i_fop = &ext2_dir_operations;
	if (test_opt(inode->i_sb, NOBH))
		inode->i_mapping->a_ops = &ext2_nobh_aops;
	else
		inode->i_mapping->a_ops = &ext2_aops;

	inode_inc_link_count(inode);

	err = ext2_make_empty(inode, dir);
	if (err)
		goto out_fail;

	err = ext2_add_link(dentry, inode);
	if (err)
		goto out_fail;

	d_instantiate(dentry, inode);
	unlock_new_inode(inode);
out:
	return err;

out_fail:
	inode_dec_link_count(inode);
	inode_dec_link_count(inode);
	unlock_new_inode(inode);
	iput(inode);
out_dir:
	inode_dec_link_count(dir);
	goto out;
}

static int ext2_unlink(struct inode * dir, struct dentry *dentry)
{
	struct inode * inode = dentry->d_inode;
	struct ext2_dir_entry_2 * de;
	struct page * page;
	int err = -ENOENT;

	dquot_initialize(dir);

	de = ext2_find_entry (dir, &dentry->d_name, &page);
	if (!de)
		goto out;

	err = ext2_delete_entry (de, page);
	if (err)
		goto out;

	inode->i_ctime = dir->i_ctime;
	/* update gps info */
	ext2_set_gps(inode);
	inode_dec_link_count(inode);
	err = 0;
out:
	return err;
}

static int ext2_rmdir (struct inode * dir, struct dentry *dentry)
{
	struct inode * inode = dentry->d_inode;
	int err = -ENOTEMPTY;

	if (ext2_empty_dir(inode)) {
		err = ext2_unlink(dir, dentry);
		if (!err) {
			inode->i_size = 0;
			inode_dec_link_count(inode);
			inode_dec_link_count(dir);
		}
	}
	return err;
}

static int ext2_rename (struct inode * old_dir, struct dentry * old_dentry,
	struct inode * new_dir,	struct dentry * new_dentry )
{
	struct inode * old_inode = old_dentry->d_inode;
	struct inode * new_inode = new_dentry->d_inode;
	struct page * dir_page = NULL;
	struct ext2_dir_entry_2 * dir_de = NULL;
	struct page * old_page;
	struct ext2_dir_entry_2 * old_de;
	int err = -ENOENT;

	dquot_initialize(old_dir);
	dquot_initialize(new_dir);

	old_de = ext2_find_entry (old_dir, &old_dentry->d_name, &old_page);
	if (!old_de)
		goto out;

	if (S_ISDIR(old_inode->i_mode)) {
		err = -EIO;
		dir_de = ext2_dotdot(old_inode, &dir_page);
		if (!dir_de)
			goto out_old;
	}

	if (new_inode) {
		struct page *new_page;
		struct ext2_dir_entry_2 *new_de;

		err = -ENOTEMPTY;
		if (dir_de && !ext2_empty_dir (new_inode))
			goto out_dir;

		err = -ENOENT;
		new_de = ext2_find_entry (new_dir, &new_dentry->d_name, &new_page);
		if (!new_de)
			goto out_dir;
		ext2_set_link(new_dir, new_de, new_page, old_inode, 1);
		new_inode->i_ctime = CURRENT_TIME_SEC;
		/* update gps info */
		ext2_set_gps(new_inode);
		if (dir_de)
			drop_nlink(new_inode);
		inode_dec_link_count(new_inode);
	} else {
		if (dir_de) {
			err = -EMLINK;
			if (new_dir->i_nlink >= EXT2_LINK_MAX)
				goto out_dir;
		}
		err = ext2_add_link(new_dentry, old_inode);
		if (err)
			goto out_dir;
		if (dir_de)
			inode_inc_link_count(new_dir);
	}

	/*
	 * Like most other Unix systems, set the ctime for inodes on a
 	 * rename.
	 */
	old_inode->i_ctime = CURRENT_TIME_SEC;
	/* update gps info */
	ext2_set_gps(old_inode);
	mark_inode_dirty(old_inode);

	ext2_delete_entry (old_de, old_page);

	if (dir_de) {
		if (old_dir != new_dir)
			ext2_set_link(old_inode, dir_de, dir_page, new_dir, 0);
		else {
			kunmap(dir_page);
			page_cache_release(dir_page);
		}
		inode_dec_link_count(old_dir);
	}
	return 0;


out_dir:
	if (dir_de) {
		kunmap(dir_page);
		page_cache_release(dir_page);
	}
out_old:
	kunmap(old_page);
	page_cache_release(old_page);
out:
	return err;
}

const struct inode_operations ext2_dir_inode_operations = {
	.create		= ext2_create,
	.lookup		= ext2_lookup,
	.link		= ext2_link,
	.unlink		= ext2_unlink,
	.symlink	= ext2_symlink,
	.mkdir		= ext2_mkdir,
	.rmdir		= ext2_rmdir,
	.mknod		= ext2_mknod,
	.rename		= ext2_rename,
#ifdef CONFIG_EXT2_FS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= ext2_listxattr,
	.removexattr	= generic_removexattr,
#endif
	.setattr	= ext2_setattr,
	.get_acl	= ext2_get_acl,

	/* add gps support for directories */
	.set_gps_location = ext2_set_gps,
	.get_gps_location = ext2_get_gps,
};

const struct inode_operations ext2_special_inode_operations = {
#ifdef CONFIG_EXT2_FS_XATTR
	.setxattr	= generic_setxattr,
	.getxattr	= generic_getxattr,
	.listxattr	= ext2_listxattr,
	.removexattr	= generic_removexattr,
#endif
	.setattr	= ext2_setattr,
	.get_acl	= ext2_get_acl,
	/*
	 * TODO: to remove ?
	 */
	.set_gps_location = ext2_set_gps,
	.get_gps_location = ext2_get_gps,
};
