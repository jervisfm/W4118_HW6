/*
 * gpsd.c
 *
 * Columbia University
 * COMS W4118 Fall 2012
 * Homework 6
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "gpsd.h"

/* after daemon-izing, STDOUT/STDERR will show up in this file */
/* use this log file to report updates on gpsd after daemonizing */
#define LOG_FILE "/data/misc/gpsd.log"

/* reading GPS coordinates (seconds) */
#define GPSD_FIX_FREQ  1

int main(int argc, char **argv)
{
	/* daemonize */

	while (1) {
		/* read GPS values stored in GPS_LOCATION_FILE */

		/* send GPS values to kernel using system call */

		/* sleep for one second */
		sleep(GPSD_FIX_FREQ);
	}

	return EXIT_SUCCESS;
}

