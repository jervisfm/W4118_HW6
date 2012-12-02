
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SET_GPS 376
#define GET_GPS 377

/* Mounted Test GPS File */
#define TEST_GPS_FILE "/data/misc/hmwk6/gps_test.txt"

struct gps_location {
	double latitude;
	double longitude;
	float  accuracy;  /* in meters */
};


static void test()
{
	int ret;
	struct gps_location loc;
	FILE *fp = NULL;

	printf("Test GPS Program\n");
	memset(&loc, 0, sizeof(loc));

	/* Create a New File and Close it */

	fp = fopen(TEST_GPS_FILE, "w");
	printf("Opening file ...\n");
	if (fp == NULL) {
		perror("Failed to open Test GPS File: %s", TEST_GPS_FILE);
		return;
	}
	printf("File Open succeeded");
	fprintf(fp, "Hello World at Time T = %l\n", time(NULL).tv_sec);
	fflush(NULL);
	fclose(fp);

	/* Retrieve File GPS Info From Kernel */
	printf("About to Make System Call to Kernel to retrieve GPS info\n");
	ret = syscall(GET_GPS, TEST_GPS_FILE, &loc);

	if (ret < 0 ) {
		perror("System call failed");
		return;
	} else {
		printf("System call worked");
	}
}

int main(int argc, char **argv)
{
	test();
	return 0;
}
