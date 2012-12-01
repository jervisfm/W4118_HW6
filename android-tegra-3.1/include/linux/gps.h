/*
 * gps.h
 *
 *  Created on: Nov 30, 2012
 *      Author: Jervis
 */

#ifndef GPS_H_
#define GPS_H_

struct gps_location {
	double latitude;
	double longitude;
	float  accuracy;  /* in meters */
};


#endif /* GPS_H_ */
