/*
 * gps.h
 *
 *  Created on: Nov 30, 2012
 */

#ifndef GPS_H_
#define GPS_H_

/* The name of the EXT-2 file system which has
 * GPS support. */
#define EXT_FS_GPS "ext2"

struct gps_location {
	double latitude;
	double longitude;
	float  accuracy;  /* in meters */
};

/* Embeds a timestamp with the location */
struct kernel_gps {
	/* A latitude, longitude location along with accuracy  */
	struct gps_location loc;
	/* The time when this location was updated. */
	struct timespec timestamp;
};

/* Represenation of the GPS information on disk */
struct gps_on_disk {
	// TO DELETE
//	double latitude;
//	double longitude;
//	float accuracy;
//	unsigned int age;


	__u64	latitude; /* GPS Latitude (double - 64 bits) */
	__u64	longitude; /* GPS Longitude (double - 64 bits) */
	__u32 	accuracy; /* GPS accuracy in meters (int - 32 bits) */
	__u32	age; /* int - 32 bits */
};

/* This is a public interface method. It's meant to be available
 * in the kernel as a means of accessing current gps data. */
void get_current_location(struct kernel_gps *result);


#endif /* GPS_H_ */
