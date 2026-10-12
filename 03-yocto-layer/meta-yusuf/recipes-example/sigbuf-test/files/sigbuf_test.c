// SPDX-License-Identifier: GPL-2.0
/* Exercises /dev/sigbuf: ioctl, round trip, ring wrap, and the full-buffer case. */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "sigbuf_ioctl.h"

#define DEV "/dev/sigbuf"

static int failures;

static void check(const char *what, int ok)
{
	printf("%-46s %s\n", what, ok ? "PASS" : "FAIL");
	if (!ok)
		failures++;
}

int main(void)
{
	unsigned int cap = 0, stored = 0;
	char out[64], in[64];
	char *big, *back;
	int fd, ok;

	fd = open(DEV, O_RDWR);
	if (fd < 0) {
		perror("open " DEV);
		return 1;
	}

	check("ioctl GET_CAPACITY", ioctl(fd, SIGBUF_GET_CAPACITY, &cap) == 0 && cap > 0);
	printf("   capacity reported: %u bytes\n", cap);

	check("ioctl RESET", ioctl(fd, SIGBUF_RESET) == 0);
	check("buffer empty after reset",
	      ioctl(fd, SIGBUF_GET_STORED, &stored) == 0 && stored == 0);

	strcpy(out, "embedded linux");
	check("write returns the full length",
	      write(fd, out, strlen(out)) == (ssize_t)strlen(out));
	check("GET_STORED matches bytes written",
	      ioctl(fd, SIGBUF_GET_STORED, &stored) == 0 && stored == strlen(out));

	memset(in, 0, sizeof(in));
	check("read returns the same bytes",
	      read(fd, in, sizeof(in)) == (ssize_t)strlen(out) &&
	      memcmp(in, out, strlen(out)) == 0);
	check("buffer empty after draining",
	      ioctl(fd, SIGBUF_GET_STORED, &stored) == 0 && stored == 0);
	check("read on empty buffer returns 0", read(fd, in, sizeof(in)) == 0);

	/* push the write pointer close to the end, then force it to wrap */
	big = malloc(cap);
	back = malloc(cap);
	if (!big || !back) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}
	memset(big, 'A', cap);
	ok = write(fd, big, cap - 8) == (ssize_t)(cap - 8);
	ok = ok && read(fd, back, cap - 8) == (ssize_t)(cap - 8);
	memset(big, 'B', 32);
	ok = ok && write(fd, big, 32) == 32;
	ok = ok && read(fd, back, 32) == 32;
	ok = ok && memcmp(back, big, 32) == 0;
	check("data survives a wrap around the ring", ok);

	/* fill it completely, then try one byte too many */
	ioctl(fd, SIGBUF_RESET);
	memset(big, 'C', cap);
	ok = write(fd, big, cap) == (ssize_t)cap;
	ok = ok && write(fd, "x", 1) == -1 && errno == ENOSPC;
	check("full buffer rejects writes with ENOSPC", ok);

	free(big);
	free(back);
	ioctl(fd, SIGBUF_RESET);
	close(fd);

	printf("\n%s\n", failures ? "SOME TESTS FAILED" : "ALL TESTS PASSED");
	return failures ? 1 : 0;
}
