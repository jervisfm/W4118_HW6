#ifndef _GPSD_H_
#define _GPSD_H_
/*
 * gpsd.h
 *
 * Columbia University
 * COMS W4118 Fall 2012
 * Homework 6
 *
 */

#include <sys/syscall.h>

struct gps_location {
	double latitude;
	double longitude;
	float  accuracy;  /* in meters */
};

/* Use this file to access the most recent gps location
 * Provides in separate lines: latitude, longitude, accuracy */
#define GPS_LOCATION_FILE "/data/media/gps_location.txt"

#ifndef DEBUG
#define DEBUG 1
#endif

#if DEBUG
	#define errp(fmt, ...) fprintf(stderr, "[E:%s:%d] (%d:%s) " fmt "\n", \
					__FUNCTION__, __LINE__, errno, \
				       strerror(errno), ## __VA_ARGS__)
	#define dbgp(fmt, ...) printf("[D:%s:%d] " fmt "\n", __FUNCTION__, \
					__LINE__, ## __VA_ARGS__)
	#define infop(fmt, ...) printf("[I:%s:%d] " fmt "\n", __FUNCTION__, \
					__LINE__, ## __VA_ARGS__)
#else
	#define errp(fmt, ...) fprintf(stderr, "[E] (%d:%s) " fmt "\n", \
					errno, strerror(errno), ## __VA_ARGS__)
	#define dbgp(fmt, ...)
	#define infop(fmt, ...) printf("[I] " fmt "\n", ## __VA_ARGS__)
#endif

#endif
