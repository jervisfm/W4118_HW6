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

/* SYstem call numbers */
#define SET_GPS 376
#define GET_GPS 377

/* after daemon-izing, STDOUT/STDERR will show up in this file */
/* use this log file to report updates on gpsd after daemonizing */
#define LOG_FILE "/data/misc/gpsd.log"

/* reading GPS coordinates (seconds) */
#define GPSD_FIX_FREQ  1

static int should_exit = 0;

static void sighandler(int signal) {
	printf("Exit Signal received. Will terminate soon ... \n");
	should_exit = 1;
}

enum { LATITUDE = 0, LONGITUDE = 1, ACCURACY = 2}

/* Reads gps values from given file.
 * Return 1 on success, 0 on error. */
static int read_gps(FILE *file, struct gps_location *result)
{
	/* Format of file is
	 * lat, longinute, accuracy */
	int ret, i;
	static int NO_FIELDS = 3;
	char *line = NULL;
	ret = getline(&line, 0, file); /* getline auto allocates memory */
	if (ret < 0)
		perror("Failed to read line from file stream");

	for (i = 0; i < NO_FIELDS; ++i) {
		switch (i) {
			case value:

				break;
			default:
				break;
		}
	}
	strtod(line, NULL)

	return ret < 0 ? 0 : 1;
}

int main(int argc, char **argv)
{

	/* daemonize */
	struct sigaction sigact;
	int ret;
	FILE *fp = NULL;

	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = &sighandler;

	if (sigaction(SIGINT, &sigact, NULL) ||
		sigaction(SIGQUIT, &sigact, NULL) ||
		sigaction(SIGTERM, &sigact, NULL))
		perror("Failed to install sig handler for daemon! ");


	while (!should_exit) {
		/* read GPS values stored in GPS_LOCATION_FILE */
		fp = fopen(GPS_LOCATION_FILE, "r");
		if (fp == NULL)
			perror("Warning: Failed to open file for reading");


		/* send GPS values to kernel using system call */


		ret = syscall(SET_GPS, NULL);

		if (ret < 0)
			perror("Failed to update kernel with new GPS");


		/* sleep for one second */
		sleep(GPSD_FIX_FREQ);
	}

	if (fp != NULL)
		fclose(fp);


	return EXIT_SUCCESS;
}

