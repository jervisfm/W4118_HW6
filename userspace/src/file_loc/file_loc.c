/*
 * file_loc.c
 *
 * Columbia University
 * COMS W4118 Fall 2012
 * Homework 6 - Geo Tagged File System
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "file_loc.h"
#define GMAP_URL "http://maps.googleapis.com/maps/api/staticmap?zoom=13" \
"&size=800x800&maptype=roadmap&markers=color:blue|label:P|%f,%f&sensor=false"

#define GET_GPS 377

struct gps_location {
	double latitude;
	double longitude;
	float  accuracy;  /* in meters */
};

static void print_gps(struct gps_location gps_location)
{
	double lat = 0, lng = 0;
	float acc = 0;
	lat = gps_location.latitude;
	lng = gps_location.longitude;
	acc = gps_location.accuracy;

	printf("Latitude: %f\n Longitude: %f\n Accuracy: %fm\n",
			lat, lng, acc);
}

static void sighandler(int sig);

/* make system call here */
static int do_file_loc(const char *path)
{
	/* print Google Maps URL based on path, return -1 on failure */
	long int ret;
	struct gps_location loc;

	if (path == NULL)
		return -1;

	printf("GPS File Reader Tool\n");
	memset(&loc, 0, sizeof(loc));

	/* Retrieve File GPS Info From Kernel */
	ret = syscall(GET_GPS, path, &loc);
	if (ret < 0 ) {
		perror("GPS Retrieval System call failed:");
		return -1;
	} else {
		printf("Retrieved GPS Information:\n");
		print_gps(loc);
		printf("\nAge of GPS info is %ld seconds\n", ret);
		printf("Google Maps URL: ");
		printf(GMAP_URL, loc.latitude, loc.longitude);
		printf("\n");
	}
	return 0;
}

static void usage(char **argv)
{
	printf("Usage: %s <file>\n", argv[0]);
	exit(0);
}

int main(int argc, char **argv)
{
	(void)signal(SIGPIPE, sighandler);
	(void)signal(SIGHUP, sighandler);
	(void)signal(SIGQUIT, sighandler);

	if (argc != 2)
		usage(argv);

	if (do_file_loc(argv[1]) < 0) {
		printf("No GPS information: ");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void sighandler(int sig)
{
	switch (sig) {
	case SIGHUP:
	case SIGPIPE:
	case SIGQUIT:
		break;
	default:
		break;
	}
}

