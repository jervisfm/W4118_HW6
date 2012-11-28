
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

	printf("Latitude: %#llx\n Longitude: %#llx\n Accuracy: %#x\n",
			lat, lng, acc);
}

static void print_gps_normal(struct gps_location gps_location)
{
	double lat = 0, lng = 0;
	float acc = 0;
	lat = gps_location.latitude;
	lng = gps_location.longitude;
	acc = gps_location.accuracy;

	printf("Latitude: %f\n Longitude: %f\n Accuracy: %f",
			lat, lng, acc);
}


static void test_write_read()
{
	long int ret;
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
		print_gps_normal(loc);
		printf("\nAge of GPS info is %ld\n", ret);
		printf("\n");
	}
}

/**
 * Check if a file exits. Returns 1 if true or 0 if false.
 */
static int file_exists(char *filename)
{
	FILE *fp = NULL;
	fp = fopen(filename, "r");
	if (fp == NULL)
		return 0;
	else
		return 1;
}

static void test_mod()
{
	long int ret;
	struct gps_location loc;
	FILE *fp = NULL;

	printf("File Modification Test GPS Program\n");
	memset(&loc, 0, sizeof(loc));

	if (!file_exists(TEST_GPS_FILE)) {
		printf("File does not exist: %s\n", TEST_GPS_FILE);
		return;
	}

	ret = syscall(GET_GPS, TEST_GPS_FILE, &loc);

	if (ret < 0) {
		perror("Warning : Unable to load initial GPS data");
		return;
	}

	printf("Starting GPS Info:\n");
	print_gps(loc);


	/* Open existing File and write to it */
	fp = fopen(TEST_GPS_FILE, "r");
	printf("Opening file ...\n");
	if (fp == NULL) {
		printf("Failed to open Test GPS File: %s\n", TEST_GPS_FILE);
		return;
	}
	printf("File Open succeeded. Proceeding to modification...\n");
	fprintf(fp, "Hello World at Time T = %ld\n", time(NULL));
	fflush(NULL);
	printf("File write succeeded\n");
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
		print_gps_normal(loc);
		printf("\nAge of GPS info is %ld\n", ret);
		printf("\n");
	}
}

static void test_read()
{
	long int ret;
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
		print_gps_normal(loc);
		printf("\nAge of GPS info is %ld\n", ret);
		printf("\n");
	}
}

static void do_nothing()
{
	return;
	test_read();
	test_write_read();
	test_mod();
}

int main(int argc, char **argv)
{
	do_nothing();
	if (argc < 2) {
		printf("Usage: test read|write|mod\n");
		return 0;
	}

	if (strcmp(argv[1], "read") == 0)
		test_read();
	else if (strcmp(argv[1], "write") == 0)
		test_write_read();
	else if (strcmp(argv[1], "mod") == 0)
		test_mod();
	return 0;
}
