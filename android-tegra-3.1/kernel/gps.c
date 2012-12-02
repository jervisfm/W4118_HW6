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


/* Structure to store the latest gps location.
 * Access to this struct outside this file should be done
 * through the get_current_location funtion.  */
static struct kernel_gps kernel_gps = {
		.loc =  {	.latitude = 0,
				.longitude = 0,
				.accuracy = 0
		},

		.timestamp = {	.tv_sec = 0,
				.tv_nsec = 0
		}
};

/* Used to protect the gps_location against concurrent modification */
static DEFINE_RWLOCK(gps_lock);

/* For consistency, some lock (read/write) should be held when this method is
 * called. */
static void print_gps(void)
{
	/* Kernel lacks support for floating points.
	 * Will print in HEX instead */
	unsigned long int *lat, *lng, *acc;
	lat = (unsigned long int*) &kernel_gps.loc.latitude;
	lng = (unsigned long int*) &kernel_gps.loc.longitude;
	acc = (unsigned long int*) &kernel_gps.loc.accuracy;

	/* currently disbaled */
	pr_debug("Latitude: %x\n Longitude: %x\n Accuracy: %x",
			*lat, *lng, *acc);
}

/* Returns 1 on success, and -ve on error.
 */
static int valid_gps(struct gps_location *loc)
{
	if (loc == NULL)
		return -EINVAL;

	if (loc->latitude == 0 && loc->longitude == 0)
		return -EINVAL;

	return 1;
}

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
	/* Still to be implemented */

	struct gps_location *k_gps = &kernel_gps.loc;

	if (valid_gps(loc))
		return -EINVAL;

	if (copy_from_user(k_gps,
			   loc, sizeof(struct gps_location)) != 0)
		return -EFAULT;

	write_lock(&gps_lock);
	memcpy(k_gps, loc, sizeof(struct gps_location));

	printk("Updated Kernel GPS\n");
	print_gps();
	printk("\n");

	write_unlock(&gps_lock);

	return 0;
}

/* Determines if the current user can access the current file.
 * Return 1 on true and 0 if false (i.e. user does not have access) */
static int can_access_file(const char *file)
{
	/* TO be implemented */
	return 0;
}

/* Determines if the given file path is a valid one.
 * Returns 1 if true, and 0 if false.  */
static int valid_filepath(const char *file)
{
	/* TO be implemented */
	return 0;
}

/*
 * Retrieves the gps saved on the given file path and saves
 * this data in @loc parameter.
 */
static void get_file_gps_location(const char *file, struct gps_location *loc)
{
	char *kfile;
	int flags;
	struct inode *d_inode;
	struct path kpath = { .mnt = NULL, .dentry = NULL} ;
	int string_size = strlen(file) + 1;

	/* Still to be implemented */
	if (file == NULL || loc == NULL)
		return;

	/* TODO: enable these checks when their functions
	 * are implemented.
	if (!valid_filepath(file))
		return;

	if (!can_access_file(file))
		return;
	*/

	kfile = kcalloc(string_size, sizeof(char), GFP_KERNEL);
	if (kfile == NULL)
		return -ENOMEM;

	if (copy_from_user(kfile, file, string_size) != 0) {
		kfree(kfile);
		return -EFAULT;
	}
	/*
	 * After looking at namei.c file in /fs, I determined
	 * path_lookup is the function we want to use.
	 * This function is inturn called from do_path_lookup,
	 * with tries different variations, which is inturn called by
	 * kern_path(). So, we should use kern_path(). It returns 0 on
	 * success and something else on failure
	 *
	 */

	/* TODO: check if we need the LOOKUP_AUTOMOUNT flag as well ?
	 * I don't think so, but just check to be sure. */
	if (kern_path(kfile, LOOKUP_DIRECTORY | LOOKUP_FOLLOW, &kpath) != 0) {
		printk("File Lookup Failed: %s\n", kfile);
		kfree(kfile);
		return -EAGAIN;
	}

	/* d_inode represents the inode of the looked up path */
	d_inode = kpath.dentry->d_inode;

	if (d_inode == NULL) {
		prinkt("File Lookup Failed. Non-existent path: %s", kfile);
		kfree(kfile);
		return -EINVAL;
	}

	/* Continue from here : need to check if INODE is EXT2,
	 * load up the ext2 specific inode, and lookup gps */
	d_inode->i_ino;


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
EXPORT_SYMBOL(get_current_location);

/*
 */
SYSCALL_DEFINE2(get_gps_location,
		const char __user *, pathname,
		struct gps_location __user *, loc)
{
	/* still to be implemented */

	struct gps_location kloc;
	char *kpathname;

	if (pathname == NULL || loc == NULL)
		return -EINVAL;

	memset(&kloc, 0, sizeof(kloc));

	kpathname = kcalloc(PATH_MAX, sizeof(char), GFP_KERNEL);

	if (kpathname == NULL)
		return -ENOMEM;

	/* Attempt to copy user parameter */
	if (copy_from_user(&kpathname, pathname, sizeof(kpathname)) != 0) {
		kfree(kpathname);
		return -EFAULT;
	}

	/* TODO: Enable these checks when we implement the functions.
	 * Don't forget to free the kpathname memory too. */
	/* if (!valid_filepath(file))
		return -EINVAL;

	if (!can_access_file(pathname))
		return -EACCES;
	*/


	read_lock(&gps_lock);

	get_file_gps_location(kpathname, &kloc);

	if (copy_to_user(loc, &kloc, sizeof(kloc)) != 0) {
		read_unlock(&gps_lock);
		return -EFAULT;
	}



	read_unlock(&gps_lock);

	/* TODO:
	 * On success, the system call should return the i_coord_age value
	 * of the inode associated with the path.*/
	return 0;
}
