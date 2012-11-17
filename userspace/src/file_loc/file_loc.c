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

static void sighandler(int sig);

/* make system call here */
static int do_file_loc(const char *path)
{
	/* print Google Maps URL based on path, return -1 on failure */

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
        switch (sig)
        {
        case SIGHUP:
        case SIGPIPE:
        case SIGQUIT:
                break;
        default:
                break;
        }
}

