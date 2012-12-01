/*
 * gpsd.c
 *
 * Columbia University
 * COMS W4118 Fall 2012
 * Homework 6
 *
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "gpsd.h"

/* System call numbers */
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


static void print_gps(struct gps_location gps_location)
{
	/* Kernel lacks support for floating points.
	 * Will print in HEX instead */
	unsigned long *lat = 0, *lng = 0, *acc = 0;
	lat = (unsigned long*) &gps_location.latitude;
	lng = (unsigned long*) &gps_location.longitude;
	acc = (unsigned long*) &gps_location.accuracy;

	printf("Latitude: %lx\n Longitude: %lx\n Accuracy: %lx",
			*lat, *lng, *acc);
}


/*
 * My own get line function. I am writing my own because the
 * GNU getline fucntion is not defined under the ARM
 * compiler we're using.
 *
 * Caller is responsible for freeing located character string.
 *
 * Returns the currently read line.
 */
static char *my_get_line(FILE *file)
{
	char c;
	int i = 0;
	int size = 2;

	if (file == NULL) {
		printf("Warn: NULL file given to my-get-line\n");
		return NULL;
	}

	char *result = calloc(size, sizeof(char));
	if (result == NULL) {
		perror("Memory allocation failed");
		return NULL;
	}

	for (c = fgetc(file); c != EOF && c != '\n'; c = fgetc(file)) {
		/* resize if necessary */
		if (i == size - 1) { /* we're about to overwrite null tmnator*/
			size *= 2;
			result = realloc(result, size);
			if (result == NULL) {
				perror("Memory Re-allocation failed");
				return NULL;
			}
		}
		result[i] = c;
		++i;
	}
	/* Null terminate string */
	result[i] = '\0';
	return result;
}

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
	int i, ret = 0;
	static int NO_FIELDS = 3;
	char *line = NULL;
	errno = 0;

	/* check input parameters */
	if (file == NULL || result == NULL) {
		printf("Warning: NULL parameters given to read_gps");
		return false;
	}

	double lat_lng_value;
	float accuracy;
	for (i = 0; i < NO_FIELDS; ++i) {

		/* Note that getline auto allocates memory */
		line = my_get_line(file);
		/* printf("line = %s | ", line); */
		if (line == NULL) {
			perror("Failed to read line from file stream");
			break;
		}
		switch (i) {
			case LATITUDE: {
				lat_lng_value = strtod(line, NULL);
				/* printf("lat=%f\n", lat_lng_value); */
				result->latitude = lat_lng_value;
				break;
			}
			case LONGITUDE: {
				lat_lng_value = strtod(line, NULL);
				/* printf("lng=%f\n", lat_lng_value); */
				result->longitude = lat_lng_value;
				break;
			}
			case ACCURACY: {
				accuracy = strtof(line, NULL);
				/* printf("%f\n", accuracy); */
				result->accuracy = accuracy;
				break;
			}
			default:
				break;
		}

		if (errno != 0) {
			/* This is a hack to get the code to compile.
			 * Check the value of accuracy directly results in
			 * a aeabi_fcmpeq undefined ref error.
			 */
			double temp = accuracy;
			if (temp + 0 == 0 || lat_lng_value == 0) {
				ret = -1;
				printf("Error: Parsing number error(%d): %s\n",
						errno, line );
				break;
			}
		}

		if (line != NULL)
			free(line);
	}

	return ret < 0 ? false : true;
}
int main2(int argc, char **argv);
int main(int argc, char **argv)
{
	printf("HELLLLO\n");
	main2(0, NULL);
	return 0;
}

int main2(int argc, char **argv)
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


	printf("Turning into a Daemon ...");

	/* When turned to a daemon, redirection of stderr/stdout to nothing
	 * (/dev/null) happens automatically */
	/*ret = daemon(0, 0);

	if (ret < 0) {
		perror("Failed to daemonize process. Exiting...");
		return EXIT_FAILURE;
	}
	*/

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
				      " for reading\n");
			sleep(GPSD_FIX_FREQ);
			continue;
		} else {
			printf("Opened LOC file\n");
		}

		/* send GPS values to kernel using system call */
		ret = read_gps(fp, &location);
		if (ret == false) {
			fprintf(log, "Error: Failed to read GPS\n");
			fclose(fp);
			printf("failed to read gps\n");
			sleep(GPSD_FIX_FREQ);
			continue;
		}

		printf("Making System call\n");
		ret = syscall(SET_GPS, &location);

		if (ret < 0)
			fprintf(log, "Failed to update kernel with new GPS\n");
		else
			printf("Successfully updated kernel with GPS info \n");

		print_gps(location);

		fclose(fp);
		fflush(NULL);
		/* sleep for one second */
		sleep(GPSD_FIX_FREQ);
	}

	fclose(log);
	return EXIT_SUCCESS;
}
