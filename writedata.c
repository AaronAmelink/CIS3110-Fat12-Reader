#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/**
 * A very simple utility that will write out a file filled with
 * a specific character.  Use this on a filesystem that you have
 * already mounted using the mtools utilites to make a new file
 * on your FAT based storage.
 */
int
main(int argc, char **argv)
{
	int start;
	int nbytes;
	int fd;
	int i;

	if (argc != 5) {
		fprintf(stderr, "Usage:\n");
		fprintf(stderr, "  %s file start nbytes charval\n", argv[0]);
		return (-1); 
	}

	if (sscanf(argv[2], "%x", &start) != 1) {
		fprintf(stderr,
			"Cannot parse hex starting location from '%s'\n",
			argv[2]);
		return (-1); 
	}

	if (sscanf(argv[3], "%x", &nbytes) != 1) {
		fprintf(stderr,
			"Cannot parse hex number of bytes from '%s'\n",
			argv[3]);
		return (-1); 
	}

	fd = open(argv[1], O_WRONLY|O_CREAT, 0600);
	if (fd < 0) {
		fprintf(stderr,
			"Cannot open file '%s' for output : %s\n",
			argv[1], strerror(errno));
		return (-1); 
	}

	if (lseek(fd, start, SEEK_SET) < 0) {
		fprintf(stderr,
			"Cannot seek to location %x : %s\n",
			start, strerror(errno));
		return (-1); 
	}

	for (i = 0; i < nbytes; i++) {
		if (write(fd, &argv[4][0], 1) != 1) {
			fprintf(stderr,
				"Failed writing to file : %s\n",
				strerror(errno));
			return (-1); 
		}
	}

	(void) close(fd);

	return 0;
}
