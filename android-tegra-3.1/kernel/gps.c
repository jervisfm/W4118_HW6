/*
 * gps.c
 *  Created on: Nov 30, 2012
 *  Operating System Assignment 6
 */

/*
 * Useful notes / files / functions
 *
 * ext2_get_inode function - returns a raw ext_inode given INODE number
 *
 * How does the general inode strcture fit with the specific ext_2 structure?
 * An 2nd ext2 structure called "struct ext2_inode_info" is used to keep an in
 * memory representation of the relevant information from the on-disk ext2
 * structure. This ext2_inode_info structure has the Inode number and it can
 * be used to actually get the RAW ext2_inode structure. For an example,
 * see __ext2_write_inode() in inode.c, #1424
 *
 * EXPORT_SYMBOL() is useful macro to make certain non-static function
 * visilbe inside the kernel.
 *
 * I was wondering why / how our code was working when creating files,
 * read through the code and found that ext2_create is called from
 * vfs_create(). The sys_open() system call is used to both open and
 * create new empty file. It just called the do_sys_open(). In this function
 * the most interesting call is to do_last(), which lookups the last segment
 * of a file path. It's in do_last() where have a call to vfs_create().
 *
 * Plan of actions
 * We have two choices:
 *
 * 1) Add GPS information in ext2_write_inode() function. This is defined
 * in super block and it is called when the data is *actually* written to
 * disk.
 *
 * 2) Add the GPS information only when the file is modified or created.
 * Will need to modify the specific functions defined in the file operationg
 * pointers.
 *
 * Option 1 should be a good for testing. Switch to Option 2 after
 * Implementing more functions.
 *
 * How to find out what type of inode you have: inode.c #1366
 */

/*
 * Main TODO:
 * -> Okay, Next course of action is to actually test out the code
 * that we've written so far, i.e. step 3.
 *
 * -> write out a separate test program that just reads coordinates of file
 * w/o creating a new file all together.(this may be a better test).
 *
 * -> Important: Look at ext2_iget() function in inode.c and see how the
 * i_op function pointers are assigned. At the moment, we have support
 * file operations, and also dir operation. do we want symlinks too??
 *
 * -> Implementing modification:
 * Useful functions include update_file_time(), mark_inode_dirty()
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/gps.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
/* For LOOKUP_FOLLOW, LOOKUP DIRECTORY flags */
#include <linux/namei.h>
/* Import the maximum length of an absolute path including the null char
 * [PATH_MAX] */
#include <linux/limits.h>

/* Define flag that stands for read file permission check. Borrowed
 * from standard UNIX headers (unistd.h)
 * See example at:
 * http://sourceware.org/git/?p=glibc.git;a=blob;f=posix/unistd.h#l281*/
#ifndef R_OK
#define R_OK 4
#endif /* R_OK */


/* Structure to store the latest gps location.
 * Access to this struct outside this file should be done
 * through the get_current_location funtion.  */
static struct kernel_gps kernel_gps = {
		.loc =  {	.latitude = 100,
				.longitude = 1000,
				.accuracy = 100
		},

		.timestamp = {	.tv_sec = 100,
				.tv_nsec = 0
		}
};

/* Used to protect the gps_location against concurrent modification */
static DEFINE_RWLOCK(gps_lock);

static unsigned long long int double_to_long(const double d)
{
	unsigned long long int result;
	result = *((unsigned long long int *)(&d));
	return result;
}

static unsigned int float_to_int(const float f)
{
	unsigned int result;
	result = *((unsigned int *)(&f));
	return result;
}


/* Tries to determine if the given path is a directory or not.
 * Definitely Always Returns 1 on true and 0 if false with high probability. */
static int is_directory(const char *path)
{
	int ret;
	if (path == NULL)
		return 0;
	ret = sys_open(path, O_DIRECTORY, O_RDONLY);

	if (ret < 0) /* Failed to open DIR, */
		return 0;
	else { /* Definitely is a directory */
		sys_close(ret);
		return 1;
	}
}

/* Returns 0 on success, and -ve on error.
 */
static int valid_gps(struct gps_location *loc)
{
	/* TODO: Important.
	 * Can't do direct floating comparison as that causes a compiler error.
	 * From Piazza, it was mentioned that we should return an error
	 * only if the GPS coordinates are all zeros, so we just check for
	 * that
	 * https://piazza.com/class#fall2012/comsw4118/1065
	 */
	const double zero_d = 0;
	const float zero_f = 0;
	unsigned long long int err_lat = double_to_long(zero_d);
	unsigned long long int err_lng = double_to_long(zero_d);
	unsigned int err_acc = float_to_int(zero_f);

	unsigned long long int loc_lat;
	unsigned long long int loc_lng;
	unsigned int loc_acc;

	if (loc == NULL)
		return -EINVAL;

	loc_lat = double_to_long(loc->latitude);
	loc_lng = double_to_long(loc->longitude);
	loc_acc = float_to_int(loc->accuracy);

	if (loc_lat == err_lat && loc_lng == err_lng && loc_acc == err_acc)
		return -EINVAL;
	else
		return 0;
}

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
	/* Still to be implemented */

	struct gps_location *k_gps = &kernel_gps.loc;

	/* Only root can update the gps information */
	if (current_uid() != 0 && current_euid() != 0)
		return -EACCES;

	if (valid_gps(loc) != 0)
		return -EINVAL;

	if (copy_from_user(k_gps,
			   loc, sizeof(struct gps_location)) != 0)
		return -EFAULT;

	write_lock(&gps_lock);
	kernel_gps.timestamp = CURRENT_TIME;
	memcpy(k_gps, loc, sizeof(struct gps_location));

	write_unlock(&gps_lock);

	return 0;
}

/* Determines if the current user can access the current file.
 * Return 1 on true (i.e. file exists and user can access it)
 * and 0 if false (i.e. user does not either have access
 * or file is non-existent) */
static int can_access_file(const char *file)
{
	/* TO be implemented */
	int ret;

	if (file == NULL)
		return 0;

	if (is_directory(file)) {
		/* returns -1 on error */
		ret = sys_open(file, O_DIRECTORY, O_RDONLY);
		if (ret < 0)
			/* leave it that way */;
		else {
			sys_close(ret);
			ret = 0; /* set success value. */
		}

	} else {
		/* Access system call returns 0 on success. R_OK flags checks
		 * both that the file exists and that the current process has
		 * permission to read the file */
		ret = sys_access(file, R_OK);
	}

	if (ret == 0)
		return 1;
	else
		return 0;
}

/**
 * Determine if the given @inode is on a FS that supports
 * GPS.
 * If So, Returns 1, else returns 0 when false.
 */
static int gps_supported(struct inode *inode)
{
	/* At the moment, only the EXT2_FS has gps enabled. */
	if (strcmp(inode->i_sb->s_type->name, EXT2_FS_GPS) == 0)
		return 1;
	else
		return 0;
}

/**
 * Retrieves the gps saved on the given file path and saves
 * this data in @loc parameter.
 * @kfile must be already a kernel allocated string.
 * It returns the age of the data as an +ve number or -ve on error.
 * Note:
 * This function finds the gps information regardless of whether current
 * process can read file. That should be checked for by the calling funciton.
 * Also, the function *assumes* that a valid kernel path is to be given,
 * otherwise the result is undefined.
 */
static int get_file_gps_location(const char *kfile, struct gps_location *loc)
{
	int ret;
	struct inode *d_inode;
	struct path kpath = { .mnt = NULL, .dentry = NULL} ;

	/* Still to be implemented */
	if (kfile == NULL || loc == NULL)
		return -EINVAL;

	/*
	 * After looking at namei.c file in /fs, I determined
	 * path_lookup is the function we want to use.
	 * This function is inturn called from do_path_lookup,
	 * with tries different variations, which is inturn called by
	 * kern_path(). So, we should use kern_path(). It returns 0 on
	 * success and something else on failure.
	 * Note: that the complimentary macro user_path() won't work in
	 * this case because the character string we're checking is
	 * in kernel-address space
	 */
	if (kern_path(kfile, LOOKUP_FOLLOW | LOOKUP_AUTOMOUNT, &kpath) != 0)
		return -EAGAIN;

	/* d_inode represents the inode of the looked up path */
	d_inode = kpath.dentry->d_inode;

	if (d_inode == NULL)
		return -EINVAL;

	/* Verify that the file path given is on a FS with GPS support */
	if (!gps_supported(d_inode))
		return -ENODEV;

	/* Make the System GPS Read Call.*/
	ret =  vfs_get_gps(d_inode, loc);
	/* release the path found */
	path_put(&kpath);
	return ret;

}

/* Public Kernel Function.
 * Stores the current gps information in result.
 */
void get_current_location(struct kernel_gps *result)
{
	if (result == NULL)
		return;

	read_lock(&gps_lock);
	*result = kernel_gps;
	read_unlock(&gps_lock);
}

/*
 */
SYSCALL_DEFINE2(get_gps_location,
		const char __user *, pathname,
		struct gps_location __user *, loc)
{
	/* still to be implemented */
	struct gps_location kloc;
	char *kpathname;
	int ret;
	/* PATH_MAX is the maximum valid length of a filepath in UNIX */
	int path_size = PATH_MAX + 1;
	if (pathname == NULL || loc == NULL)
		return -EINVAL;

	kpathname = kcalloc(path_size, sizeof(char), GFP_KERNEL);
	if (kpathname == NULL)
		return -ENOMEM;

	ret = strncpy_from_user(kpathname, pathname, path_size);

	if (ret < 0) { /* Error occured */
		kfree(kpathname);
		return -EFAULT;
	} else if (ret == path_size) { /* Path is too long */
		kfree(kpathname);
		return -ENAMETOOLONG;
	}

	memset(&kloc, 0, sizeof(kloc));

	/* TODO: Enable these checks when we implement the functions.
	 * Don't forget to free the kpathname memory too. */
	if (!can_access_file(pathname)) {
		kfree(kpathname);
		return -EACCES;
	}

	ret = get_file_gps_location(kpathname, &kloc);

	if (ret < 0) {
		kfree(kpathname);
		return -EAGAIN;
	}

	if (copy_to_user(loc, &kloc, sizeof(struct gps_location)) != 0) {
		read_unlock(&gps_lock);
		kfree(kpathname);
		return -EFAULT;
	}


	kfree(kpathname);
	/* TODO:
	 * On success, the system call should return the i_coord_age value
	 * of the inode associated with the path.*/
	return ret;
}
