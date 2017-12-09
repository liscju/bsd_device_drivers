#include <sys/types.h>
#include <sys/ioctl.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ECHO_CLEAR_BUFFER     _IO('E', 1)
#define ECHO_SET_BUFFER_SIZE  _IOW('E', 2, int)

static enum {UNSET, CLEAR, SETSIZE} action = UNSET;

static void
usage() {
	fprintf(stderr, "usage: echo_config -c | -s size");
	exit(1);
}

int main(int argc, char **argv) {
	int ch, fd, i, size;
	char *p;

	while ((ch = getopt(argc, argv, "cs:")) != -1)
		switch (ch) {
			case 'c':
				if (action != UNSET)
					usage();
				action = CLEAR;
				break;
			case 's':
				if (action != UNSET)
					usage();
				action = SETSIZE;
				size = (int) strtol(optarg, &p, 10);
				if (*p)
					errx(1, "illegal size -- %s", optarg);
				break;
			default:
				usage();
		}

	if (action == CLEAR) {
		fd = open("/dev/echo", O_RDWR);
		if (fd < 0)
			err(1, "open(/dev/echo)");
		i = ioctl(fd, ECHO_CLEAR_BUFFER, NULL);
		if (i < 0)
			err(1, "ioctl(/dev/echo)");
		close(fd);
	} else if (action == SETSIZE) {
		fd = open("/dev/echo", O_RDWR)	;
		if (fd < 0)
			err(1, "open(/dev/echo)");
		i = ioctl(fd, ECHO_SET_BUFFER_SIZE, &size);
		if (i < 0)
			err(1, "ioctl(/dev/echo)");
		close(fd);
	} else
		usage();

	return (0);
}
