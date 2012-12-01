
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SET_GPS 376
#define GET_GPS 377

struct gps_location {
	double latitude;
	double longitude;
	float  accuracy;  /* in meters */
};


static void test()
{
	int ret;
	printf("Hello world program");


	ret = syscall(SET_GPS, NULL);

	if (ret < 0 ) {
		perror("System call failed");
	} else {
		printf("System call worked");
	}
}

int main(int argc, char **argv)
{
	test();
	return 0;
}
