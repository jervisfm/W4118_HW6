/*
 * gps.c
 *  Created on: Nov 30, 2012
 *  Operating System Assignment 6
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/gps.h>

/* Structure to store the latest gps location  */
static struct gps_location gps_location = {	.latitude = 0,
						.longitude = 0,
						.accuracy = 0};

/* Used to protect the gps_location against concurrent modification */
static DEFINE_RWLOCK(gps_lock);

/* For consistency, some lock (read/write) should be held when this method is
 * called. */
static void print_gps()
{
	pr_debug("Latitude: %f\n Longitude: %f\n Accuracy: %f",
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

	write_lock(&set_gps_lock);
	memcpy(kernel_gps, loc, sizeof(struct gps_location));
	write_unlock(&set_gps_lock);

}

SYSCALL_DEFINE2(get_gps_location,
		const char __user *, pathname,
		struct gps_location __user *, loc)
{
	/* still to be implemented */


	read_lock(&set_gps_lock);

	read_unlock(&set_gps_lock);

	return -EINVAL;
}
