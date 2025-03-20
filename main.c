#include <stdio.h>

#include "fat12fs.h"
#include "commands.h"

int
main(int argc, char **argv)
{
	fat12fs *fs;
	int didSomething = 0;
	int base = 16;
	int i;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (argv[i][0] == 'x') {
				base = 16;
			} else if (argv[i][0] == 'd') {
				base = 10;
			} else {
				fprintf(stderr, "Unknown option '%s'\n",
					argv[i]);
				return (-1);
			}
		} else {
			fs = fat12fsMount(argv[i]);
			if (fs == NULL) {
				fprintf(stderr,
					"Cannot mount filesystem in"
					" '%s'\n", argv[i]);
				return (1);
			}

			printf("Filesystem data:\n");
			printf("   size (bytes): 0x%06x (%d) %dkB\n",
					fs->fs_fssize * FS_BLKSIZE,
					fs->fs_fssize * FS_BLKSIZE,
					(fs->fs_fssize * FS_BLKSIZE) / 1024);
			printf("  size (blocks):   0x%04x (%d)\n",
					fs->fs_fssize, fs->fs_fssize);
			printf("    FAT sectors:   0x%04x (%d)\n",
					fs->fs_fatsectors, fs->fs_fatsectors);
			printf("     Rootdir at:   0x%04x (%d)\n",
					fs->fs_rootdirblock,
					fs->fs_rootdirblock);
			printf(" Datablock 0 at:   0x%04x (%d)\n",
					fs->fs_datablock0,
					fs->fs_datablock0);
			printf("\n");

			processCommands(stdin, stdout, fs, base);

			fat12fsUmount(fs);

			didSomething = 1;
		}
	}

	if (didSomething == 0) {
		fprintf(stderr, "No filesystem given\n");
		return (1);
	}

	return 0;
}
