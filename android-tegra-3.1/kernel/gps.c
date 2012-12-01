/*
 * gps.c
 *  Created on: Nov 30, 2012
 *  Operating System Assignment 6
 */

#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/gps.h>


SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc) {
	/* Still to be implemented */
	return -EINVAL;
}

SYSCALL_DEFINE2(get_gps_location,
				const char __user *, pathname,
				struct gps_location __user *, loc) {
	/* still to be implemented */
	return -EINVAL;
}
