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

#define true 1;
#define false 0

static int should_exit = 0;

static void sighandler(int signal) {
	printf("Exit Signal received. Will terminate soon ... \n");
	should_exit = 1;
}


enum { LATITUDE = 0, LONGITUDE = 1, ACCURACY = 2};

/* Reads gps values from given file.
 * Return 1 on success, 0 on error. */
static int read_gps(FILE *file, struct gps_location *result)
{
	/* Format of file is
	 * lat, longitude, accuracy */
	int ret, i;
	static int NO_FIELDS = 3;
	char *line = NULL;

	/* check input parameters */
	if (file == NULL || result == NULL) {
		printf("Warning: NULL parameters given to read_gps");
		return false;
	}

	double lat_lng_value;
	float accuracy;
	for (i = 0; i < NO_FIELDS; ++i) {
		/* Note tht getline auto allocates memory */
		ret = getline(&line, 0, file);
		if (ret < 0) {
			perror("Failed to read line from file stream");
			break;
		}
		switch (i) {
			case LATITUDE: {
				lat_lng_value = strtod(line, NULL);
				result->latitude = lat_lng_value;
				break;
			}
			case LONGITUDE: {
				lat_lng_value = strtod(line, NULL);
				result->longitude = lat_lng_value;
				break;
			}
			case ACCURACY: {
				accuracy = strtof(line, NULL);
				result->accuracy = accuracy;
				break;
			}
			default:
				break;
		}
		if (line != NULL)
			free(line);

		if ((lat_lng_value == 0 || accuracy == 0)
			&& errno != 0) { /* error happened */
			ret = -1;
			printf("Warning: Parsing number error\n");
			break;
		}
	}

	return ret < 0 ? false : true;
}

int main(int argc, char **argv)
{
	/* daemonize  first */
	struct sigaction sigact;
	struct gps_location location;
	int ret;
	FILE *fp = NULL;
	FILE *log = NULL;

	memset(&sigact, 0, sizeof(sigact));
	memset(&location, 0, sizeof(location));

	sigact.sa_handler = &sighandler;

	if (sigaction(SIGINT, &sigact, NULL) ||
		sigaction(SIGQUIT, &sigact, NULL) ||
		sigaction(SIGTERM, &sigact, NULL))
		perror("Failed to install sig handler for daemon! ");

	printf("Turning into a Daemon ....");

	/* When turned to a daemon, redirection of stderr/stdout to nothing
	 * (/dev/null) happens automatically */
	ret = daemon(0, 0);

	if (ret < 0) {
		perror("Failed to daemonize process. Exiting...");
		return EXIT_FAILURE;
	}

	/* Open the daemon log file for writing updates. */
	log = fopen(LOG_FILE, "w+");
	if (log == NULL) {
		perror("Failed to open LOG file for daemon. Exiting...");
		return EXIT_FAILURE;
	} else
		fprintf(log, "\n***** New Daemon Run *****\n");



	while (!should_exit) {
		/* read GPS values stored in GPS_LOCATION_FILE */
		fp = fopen(GPS_LOCATION_FILE, "r");
		if (fp == NULL) {
			fprintf(log, "Warning: Failed to open LOC file"
				      " for reading");
			sleep(GPSD_FIX_FREQ);
			continue;
		}

		/* send GPS values to kernel using system call */
		ret = read_gps(fp, &location);
		if (ret == false) {
			fprintf(log, "Error: Failed to read GPS");
			fclose(fp);
			sleep(GPSD_FIX_FREQ);
			continue;
		}

		ret = syscall(SET_GPS, &location);

		if (ret < 0)
			fprintf(log, "Failed to update kernel with new GPS");


		fclose(fp);

		/* sleep for one second */
		sleep(GPSD_FIX_FREQ);
	}

	fclose(log);
	return EXIT_SUCCESS;
}
