/*
 * gps.c
 *  Created on: Nov 30, 2012
 *  Operating System Assignment 6
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/gps.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

/* Import the maximum length of an absolute path including the null char
 * [PATH_MAX] */
#include <linux/limits.h>


/* Structure to store the latest gps location  */
static struct gps_location gps_location = {	.latitude = 0,
						.longitude = 0,
						.accuracy = 0};

/* Used to protect the gps_location against concurrent modification */
static DEFINE_RWLOCK(gps_lock);

/* For consistency, some lock (read/write) should be held when this method is
 * called. */
static void print_gps(void)
{
	printk("Latitude: %f\n Longitude: %f\n Accuracy: %f",
			gps_location.latitude,
			gps_location.longitude, gps_location.accuracy);
}

SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
	/* Still to be implemented */

	struct gps_location *kernel_gps = &gps_location;

	if (loc == NULL)
		return -EINVAL;

	if (copy_from_user(kernel_gps,
			   loc, sizeof(struct gps_location)) != 0)
		return -EFAULT;

	write_lock(&gps_lock);
	memcpy(kernel_gps, loc, sizeof(struct gps_location));

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
	/* Still to be implemented */
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
