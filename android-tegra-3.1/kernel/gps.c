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
 *
 * -> write out a separate test program that just reads coordinates of file
 * w/o creating a new file all together.(this may be a better test).
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

/* For consistency, some lock (read/write) should be held when this method is
 * called. */
static void print_gps(void)
{
	/* Kernel lacks support for floating points.
	 * Will print in HEX instead */
	unsigned long long int lat, lng;
	unsigned int acc;
	lat = *((unsigned long long int*) &kernel_gps.loc.latitude);
	lng = *((unsigned long long int*) &kernel_gps.loc.longitude);
	acc = *((unsigned int*) &kernel_gps.loc.accuracy);

	printk("Latitude: %#llx\n Longitude: %#llx\n Accuracy: %#x",
			lat, lng, acc);
	double dd = 100.47;
	double ee = 100;
	unsigned long long int temp = *((unsigned long long int *)(&dd));
	__le64 dd_le = cpu_to_le64(temp);
	__u64 dd_u64 = le64_to_cpu(dd_le);

	printk("\nreg double | %#llx \n", *((unsigned long long int * )(&dd)));
	// cast dd to unsigned int

	printk("rd le64 : %#llx\n", *((unsigned long long int * )(&dd_le)));
	printk("rd u64  : %#llx\n", *((unsigned long long int * )(&dd_u64)));
	printk("rd ctrl : %#llx\n", *((unsigned long long int * )(&temp)));


	//printk("int double==== | %#llx \n", *((unsigned long long int * )(&ee)));



	/* other Debugging / test code. DELETE LATER.
	__le64 lat1, lng1;
	__le32 acc1;
	lat1 = cpu_to_le64(100);
	lng1 = cpu_to_le64(1000);
	acc1 = cpu_to_le32(100);
	printk("Latitude: %#llx\n Longitude: %#llx\n Accuracy: %#x",
			cpu_to_le64(100), cpu_to_le64(1000), cpu_to_le32(100));
	printk("\n");
	printk("Lat : %#llx | Lng: %#llx | Accuracy: %x\n===== ==== \n",
		le64_to_cpu(lat1), le64_to_cpu(lng1), le32_to_cpu(acc1));
	*/
}

/* Returns 0 on success, and -ve on error.
 */
static int valid_gps(struct gps_location *loc)
{
	if (loc == NULL)
		return -EINVAL;

	/* TODO: Important.
	 * This is disabled because it causes a compiler error.
	 * From Piazza, it was mentioned that we should return an error
	 * if the GPS coordinates are not all zeros.
	if (loc->latitude == 0 && loc->longitude == 0)
		return -EINVAL;

	*/
	return 0;
}

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
	/* Still to be implemented */

	struct gps_location *k_gps = &kernel_gps.loc;

	if (valid_gps(loc) != 0)
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

/*
 * Retrieves the gps saved on the given file path and saves
 * this data in @loc parameter.
 * @kfile is already a kernel allocated string.
 * It returns the age of the data as an int.
 */
static int get_file_gps_location(const char *kfile, struct gps_location *loc)
{
	struct inode *d_inode;
	struct path kpath = { .mnt = NULL, .dentry = NULL} ;

	/* Still to be implemented */
	if (kfile == NULL || loc == NULL)
		return -EINVAL;

	/* TODO: enable these checks when their functions
	 * are implemented.
	if (!valid_filepath(file))
		return;

	if (!can_access_file(file))
		return;
	*/


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
	// if (kern_path(kfile, LOOKUP_DIRECTORY | LOOKUP_FOLLOW, &kpath) != 0) {
	if (kern_path(kfile, LOOKUP_FOLLOW | LOOKUP_AUTOMOUNT, &kpath) != 0) {
		printk("File Lookup Failed: %s\n", kfile);
		return -EAGAIN;
	}

	/* d_inode represents the inode of the looked up path */
	d_inode = kpath.dentry->d_inode;

	if (d_inode == NULL) {
		printk("File Lookup Failed. Non-existent path: %s\n", kfile);
		return -EINVAL;
	}


	/* Verify that the file path given is on a FS with GPS support */
	if (!gps_supported(d_inode)) {
		printk("GPS Lookup Not Supported: File (%s) "
				"not on EXT FS with GPS support\n", kfile);
		return -ENODEV;
	}

	/* Assume gps call to read file worked */
	printk("Looking up GPS information for file : %s\n",
			kpath.dentry->d_iname);

	/* Make the System GPS Read Call.*/
	return vfs_get_gps(d_inode, loc);

	//struct file_system_type *fst; d_inode->i_ino;


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
	int ret;
	int path_size = PATH_MAX + 1;
	if (pathname == NULL || loc == NULL)
		return -EINVAL;

	memset(&kloc, 0, sizeof(kloc));

	kpathname = kcalloc(path_size, sizeof(char), GFP_KERNEL);

	if (kpathname == NULL)
		return -ENOMEM;

	/* Attempt to copy user parameter */
	if (copy_from_user(kpathname, pathname, path_size) != 0) {
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

	ret = get_file_gps_location(kpathname, &kloc);

	if (ret < 0) {
		printk("Oops, failed to read GPS information for %s. Error %d\n",
				kpathname, ret);
		kfree(kpathname);
		return -EAGAIN;
	}

	if (copy_to_user(loc, &kloc, sizeof(struct gps_location)) != 0) {
		read_unlock(&gps_lock);
		kfree(kpathname);
		return -EFAULT;
	}

	read_unlock(&gps_lock);
	kfree(kpathname);
	/* TODO:
	 * On success, the system call should return the i_coord_age value
	 * of the inode associated with the path.*/
	return ret;
}
