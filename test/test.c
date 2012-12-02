
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define SET_GPS 376
#define GET_GPS 377
#define PATH_MAX 4096

/* Mounted Test GPS File */
#define TEST_GPS_FILE "/data/misc/hmwk6/gps_test.txt"

struct gps_location {
	double latitude;
	double longitude;
	float  accuracy;  /* in meters */
};


static void print_gps(struct gps_location gps_location)
{
	unsigned long long int lat = 0, lng = 0;
	unsigned int acc = 0;
	lat = *((unsigned long long int*) &gps_location.latitude);
	lng = *((unsigned long long int*) &gps_location.longitude);
	acc = *((unsigned int*) &gps_location.accuracy);

	/* still doesn't appear to work */
	printf("Latitude: %#llx\n Longitude: %#llx\n Accuracy: %#x",
			lat, lng, acc);
}

static void test_write_read()
{
	int ret;
	struct gps_location loc;
	FILE *fp = NULL;

	printf("Write-Read Test GPS Program\n");
	memset(&loc, 0, sizeof(loc));

	printf("Starting GPS Info:\n");
	print_gps(loc);

	/* Create a New File and Close it */
	fp = fopen(TEST_GPS_FILE, "w");
	printf("Opening file ...\n");
	if (fp == NULL) {
		printf("Failed to open Test GPS File: %s\n", TEST_GPS_FILE);
		return;
	}
	printf("File Open succeeded\n");
	fprintf(fp, "Hello World at Time T = %ld\n", time(NULL));
	fflush(NULL);
	fclose(fp);

	/* Retrieve File GPS Info From Kernel */
	printf("About to Make System Call to Kernel to retrieve GPS info\n");
	ret = syscall(GET_GPS, TEST_GPS_FILE, &loc);
	if (ret < 0 ) {
		perror("System call failed:");
		return;
	} else {
		printf("System call worked\n");
		printf("Retrieved GPS Information:\n");
		print_gps(loc);
		printf("\n");
	}
}

static void test_read()
{
	int ret;
	struct gps_location loc;

	printf("Reading Test GPS Program\n");
	memset(&loc, 0, sizeof(loc));

	printf("Starting GPS Info:\n");
	print_gps(loc);


	/* Retrieve File GPS Info From Kernel */
	printf("About to Make System Call to Kernel to retrieve GPS info\n");
	ret = syscall(GET_GPS, TEST_GPS_FILE, &loc);
	if (ret < 0 ) {
		perror("System call failed:");
		return;
	} else {
		printf("System call worked\n");
		printf("Retrieved GPS Information:\n");
		print_gps(loc);
		printf("Age of GPS info is %s\n", ret);
		printf("\n");
	}
}

static void do_nothing()
{
	return;
	test_read();
	test_write_read();
}

int main(int argc, char **argv)
{
	do_nothing();
	test_write_read();
	//test_read();
	return 0;


}
