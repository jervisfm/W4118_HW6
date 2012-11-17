#ifndef _FILE_LOC_H_
#define _FILE_LOC_H_
/*
 * file_loc.h
 *
 * Columbia University
 * COMS W4118 Fall 2012
 * Homework 6 - Geo Tagged File System
 */
#include <sys/syscall.h>

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

