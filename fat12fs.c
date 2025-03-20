#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "fat12fs.h"


/*
 * This structure holds the DOS 5 boot block.  This is what you will
 * find in the book block (first block) of any FAT-12 filesystem, such
 * as found on SD cards and other small storage media.
 *
 * It is a packed structure defined exactly as is found on the disk,
 * which is why all fields are character.
 */
typedef struct fat12fs_BOOTBLOCK {
	unsigned char	bb_jump_instr[3];
	unsigned char	bb_oem_name[8];
	unsigned char	bb_bytes_per_sector[2];
	unsigned char	bb_sectors_per_block;
	unsigned char	bb_reserved_sectors[2];
	unsigned char	bb_num_fats;
	unsigned char	bb_root_dir_entries[2];
	unsigned char	bb_total_sectors[2];
	unsigned char	bb_media_type;
	unsigned char	bb_sectors_per_fat[2];
	unsigned char	bb_sectors_per_track[2];
	unsigned char	bb_number_of_heads[2];
	unsigned char	bb_number_of_hidden_sectors[4];
	unsigned char	bb_total_sectors_big[4];
	unsigned char	bb_extra[476];
} fat12fs_BOOTBLOCK;


/** define locations and sizes */
#define FAT_BOOTBLOCK	0
#define FAT12_MAXSIZE	4086
#define FAT_MAXDIR	(int) ((25 * FS_BLKSIZE) / sizeof (struct fat12fs_DIRENTRY))
#define FAT_DIRPERBLK	(int) (FS_BLKSIZE / sizeof (struct fat12fs_DIRENTRY))


/*
 * Special values for FAT table entries.
 * FAT12_EOF1	- first value that indicates End of File 
 * FAT12_EOFF	- last End of File value. (why a range and not just one??)
 * FAT12_FREE	- indicates a Free (unallocated) block
 * FAT16_xxx	- same as above, for FAT16
 */
#define FAT12_EOF1	0x0ff8  
#define FAT12_EOFF	0x0fff  
#define FAT12_FREE	0


/*
 * and constants for directory fields
 * NAME0_XXX - Entries for first byte of file name
 * ATTR_XXX - Bits that can be set in attrbutes byte
 */
#define NAME0_EMPTY	0x00	/* Unused entry */
#define NAME0_DELETED	0xe5	/* entry deleted (can be reused) */
#define NAME0_E5	0x05	/* entry should be E5 */
#define ATTR_REGULAR	0x00	/* Regular file */ 
#define ATTR_READONLY	0x01	/* Read-only file */ 
#define ATTR_HIDDEN	0x02	/* Hidden file */ 
#define ATTR_SYSTEM	0x04	/* System file */ 
#define ATTR_VOLUME	0x08	/* Volume label */
#define ATTR_LONGNAME	0x0f	/* Long name entry */
#define ATTR_DIR	0x10	/* Directory */
#define ATTR_ARCHIVE	0x20	/* File is new or modified */



/**
 * Read a physical block of data from the disk into the given buffer.
 * the blknum address is a physical address within the file system
 */
int
fat12fsRawDiskRead(int fd, int blknum, char *buffer)
{
	int status;

	status = lseek(fd, (off_t)(blknum * FS_BLKSIZE), SEEK_SET);
	if (status < 0)
		return (-1);

	status = read(fd, buffer, FS_BLKSIZE);
	if (status < 0)
		return (-1);

	return 0;
}

/**
 * Load the boot block (block 0), which contains the information
 * which lets us figure everything else out (including whether or
 * not this is even a FAT-12 file system).
 */
int
fat12fsLoadBootBlock(struct fat12fs *fs)
{
	struct fat12fs_BOOTBLOCK bootblock;
	unsigned short val;


	/** read in the block from the disk */
	if (fat12fsRawDiskRead(fs->fs_fd,
			FAT_BOOTBLOCK,
			(char *)&bootblock) < 0) {
		fprintf(stderr, "Failed reading boot block\n");
		return (-1);
	}

	/**
	 * make sure the data block size is FS_BLKSIZE, and that
	 * the filesystem is set up for only 1 sector ber block
	 *
	 * if these constraints don't hold, all of our math later
	 * is much more complicated, and the code here is wrong
	 */
	bcopy((char *)&bootblock.bb_bytes_per_sector, (char *)&val, 2);
	if (val != FS_BLKSIZE || bootblock.bb_sectors_per_block != 1) {
		fprintf(stderr,
			"Expected %d bytes/filesystem block, found %d\n",
			val * bootblock.bb_sectors_per_block,
			FS_BLKSIZE);
		return (-1);
	}


	/**
	 * Load up the "filesystem" structure with the appropriate
	 * data from the boot block, so that we know how large
	 * the various parts of the system are, etc.
	 */
	bcopy((char *)&bootblock.bb_reserved_sectors,
			(char *)&fs->fs_fatblock, 2);
	bcopy((char *)&bootblock.bb_sectors_per_fat,
			(char *)&fs->fs_fatsectors, 2);

	fs->fs_fatsize = (fs->fs_fatsectors * FS_BLKSIZE * 2) / 3;
	fs->fs_numfats = bootblock.bb_num_fats;
	fs->fs_rootdirblock = fs->fs_fatblock
		+ (fs->fs_numfats * fs->fs_fatsectors);

	bcopy((char *)&bootblock.bb_root_dir_entries,
		(char *)&fs->fs_rootdirsize, 2);
	if (fs->fs_rootdirsize > FAT_MAXDIR) {
		fprintf(stderr,
			"Root directory is %hd sectors, max %d\n",
				fs->fs_rootdirsize, FAT_MAXDIR);
		return (-1);
	}


	/**
	 * Make sure we know where "datablock zero" is.
	 *
	 * Note that the data blocks start numbering at 2,
	 * for some reason peculiar to DOS, so when we
	 * use this value to locate an actual data block,
	 * we need to take this into account.
	 */
	fs->fs_datablock0 = fs->fs_rootdirblock +
		(fs->fs_rootdirsize * sizeof (struct fat12fs_DIRENTRY))
			/ FS_BLKSIZE;
	bcopy((char *)&bootblock.bb_total_sectors, (char *)&val, 2);
	fs->fs_fssize = val;


	/*
	 * DOS 5 or later uses the long at the end to indicate the size
	 * of large file systems, so adjust fs_fssize to be the number
	 * of data blocks by subtracting out fs_datablock0.
	 */
	if (fs->fs_fssize == 0)
		bcopy((char *)&bootblock.bb_total_sectors_big,
				(char *)&fs->fs_fssize, 4);
	fs->fs_fssize = fs->fs_fssize - fs->fs_datablock0;


	/*
	 * If there are > FAT12_MAXSIZE data blocks, the file system is
	 * FAT16 or FAT32.
	 */
	if (fs->fs_fssize == 0 || fs->fs_fssize > FAT12_MAXSIZE) {
		fprintf(stderr,
			"Not a FAT-12 filesystem. FAT size is %d\n",
				fs->fs_fssize);
		return (-1);
	}


	/*
	 * Now, add 2 to fs_fssize, since the data blocks are numbered
	 * starting at 2.
	 */
	fs->fs_fssize += 2;


	/*
	 * And reduce the FAT table size to the number of data blocks;
	 * this is done because we may have a partially full FAT block
	 * at the end because the filesystem size may not be an even
	 * multiple of FAT-block indexable entries
	 */
	if (fs->fs_fssize < fs->fs_fatsize)
		fs->fs_fatsize = fs->fs_fssize;

	return (0);
}


/**
 * Delete a structure of type fat12fs, including all sub-structures
 */
void
fat12fsDeleteFSData(struct fat12fs *fs)
{
	if (fs != NULL) {
		if (fs->fs_fatdata != NULL) {
			free (fs->fs_fatdata);
		}
		if (fs->fs_rootdirentry != NULL) {
			free (fs->fs_rootdirentry);
		}
		free(fs);
	}
}


/**
 * "Mount" a file system.  As this code does not do any caching
 * itself, "mount" is quite simple:
 *   - load boot block and ensure the filesystem is actually correct
 *   - load up the FAT information so we can find file blocks
 *   - load up the "root" directory, so we can look up files.
 *
 * If this were going to be an efficient read/write file system,
 * a set of managed buffers for cached data would need to be set
 * up as well
 */
struct fat12fs *
fat12fsMount(const char *filename)
{
	struct fat12fs *fs;
	int nDirBlocks;
	int fd;
	int i;


	/** if we can't open this file, just bail */
	if ((fd = open(filename, O_RDONLY, 06000)) < 0) {
		return NULL;
	}


	/**
	 * if we have opened it, allocate the storage to figure out
	 * what is inside
	 */
	fs = (struct fat12fs *) malloc(sizeof(struct fat12fs));
	fs->fs_rootdirentry = NULL;
	fs->fs_fatdata = NULL;
	fs->fs_fd = fd;


	if (fat12fsLoadBootBlock(fs) < 0) {
		goto FAIL;
	}


	/**
	 * read FAT into memory
	 */
	fs->fs_fatdata = (unsigned char *)
			malloc(FS_BLKSIZE * fs->fs_fatsectors);

	for (i = 0; i < fs->fs_fatsectors; i++) {
		if (fat12fsRawDiskRead(fs->fs_fd, fs->fs_fatblock + i,
				(char *) &fs->fs_fatdata[i * FS_BLKSIZE]) < 0) {
			goto FAIL;
		}
	}


	/**
	 * read rootdir into memory
	 */
	fs->fs_rootdirentry = (struct fat12fs_DIRENTRY *)
			malloc(fs->fs_rootdirsize
				* sizeof(struct fat12fs_DIRENTRY));

	nDirBlocks = (fs->fs_rootdirsize / FAT_DIRPERBLK);
	for (i = 0; i < nDirBlocks; i++) {
		if (fat12fsRawDiskRead(fs->fs_fd, fs->fs_rootdirblock + i,
				(char *) &fs->fs_rootdirentry[
						i * FAT_DIRPERBLK
					]) < 0) {
			goto FAIL;
		}
	}

	printf("Mounted :: loaded bootblock, fat and rootdir\n");
	return fs;


FAIL:
	fat12fsDeleteFSData(fs);
	return NULL;
}


/**
 * As mentioned in "mount" above, there is no managed cache in this
 * code.  As there is no cache, there is very little to clean up,
 * and nothing to "flush" to the disk.
 */
int
fat12fsUmount(struct fat12fs *fs)
{
	fat12fsDeleteFSData(fs);
	printf("Unmounted :: cleaned up\n");
	return 0;
}


/**
 * read the appropriate 12 bits from the packed block of FAT
 * entries and convert it into a 16-bit number.
 *
 * The twiddling here is done because in every
 *    24 bits = 3 * 8 bytes, there are
 *    24 bits = 2 * 12 FAT entries,
 * arranged as follows:
 *
 *	|------| |------|  |------|
 *	00000000 11111111  22222222
 * CHAR	01234567 01234567  01234567
 *	---------------------------
 *  FAT	01234567 89AB0123  456789AB
 *	00000000 00001111  11111111
 *	|------- ---||---  -------|
 */
unsigned short
fat12fsGetFatEntry(struct fat12fs *fs, int index)
{
	unsigned short val;
	int i;

	i = (index * 3) / 2;
	bcopy((char *)&fs->fs_fatdata[i], (char *)&val, 2);
	if (index & 0x1)
		val >>= 4;
	val &= 0xfff;
	return (val);
}


/**
 * Print the FAT table out to the supplied FILE pointer
 *
 * This should be printed in hexadecimal, with 16 entries
 * per row.
 */
int
fat12fsDumpFat(FILE *ofp, struct fat12fs *fs)
{
	fprintf(stderr, "fat12fsDumpFat() -- NOT IMPLEMENTED YET\n");
	/* Not implemented yet, so just return an error */
	/* ??? */
	return -1;
}

/**
 * Print out the root directory, printing out only entries
 * which are not empty, and labelling each type of entry:
 *   VOL(ume)
 *   DEL(eted)
 *   FILE
 */
int
fat12fsDumpRootdir(FILE *ofp, struct fat12fs *fs)
{
	fprintf(stderr, "fat12fsDumpRootdir() -- NOT IMPLEMENTED YET\n");
	/* Not implemented yet, so just return an error */
	/* ??? */
	return -1;
}

/**
 * Search through the root directory, looking for the given
 * file name.
 *
 * Note that as is typical with DOS, search should be case
 * insensitive.  Be sure to handle the fact that the "extension"
 * is stored separately from the "name" portion.
 *
 * Only "files" should be found -- Volumes or "deleted" items
 * should be skipped
 */
int
fat12fsSearchRootdir(
	struct fat12fs *fs,
	const char *filename)
{
	fprintf(stderr, "fat12fsSearchRootdir() -- NOT IMPLEMENTED YET\n");
	/* Not implemented yet, so just return an error */
	/* ??? */
	return -1;
}

/**
 * Use the fat12fsRawDiskRead() function to load a logical
 * data block into the provided buffer, remembering that
 * the first data block is numbered "2"
 */
int
fat12fsLoadDataBlock(
	struct fat12fs *fs,
	char *buffer,
	int index)
{
	fprintf(stderr, "fat12fsLoadDataBlock() -- NOT IMPLEMENTED YET\n");
	/* Not implemented yet, so just return an error */
	/* ??? */
	return -1;
}

/**
 * If the indicated dirEntry is a FILE, verify its integrity
 * by processing through the file-block list and ensuring
 * that the last FAT entry is marked EOF.  Ensure that the
 * number of bytes reported by the file matches this length
 * in file blocks.
 *
 * If everything is OK, return 1.
 * If the entry is a FILE, but it does not check out, return 0;
 * If the entry does not correspond to a FILE, or some other
 * problem is encountered, return -1
 */
int
fat12fsVerifyEOF(
	struct fat12fs *fs,
	int dirEntryIndex)
{
	int bytesRemainInFile, bytesThisBlock;
	short curblock;

	
	curblock = fs->fs_rootdirentry[dirEntryIndex].de_fileblock0;
	bytesRemainInFile = fs->fs_rootdirentry[dirEntryIndex].de_filelen;

	while (bytesRemainInFile > 0) {

		if (bytesRemainInFile < FS_BLKSIZE)
			bytesThisBlock = bytesRemainInFile;
		else
			bytesThisBlock = FS_BLKSIZE;

		bytesRemainInFile -= FS_BLKSIZE;

		if (bytesRemainInFile > 0) {
			if (curblock >= FAT12_EOF1 && curblock <= FAT12_EOFF)
			    return 0;
			curblock = fat12fsGetFatEntry(fs, curblock);
		}
	}

	curblock = fat12fsGetFatEntry(fs, curblock);
	if ((curblock & FAT12_EOF1) == FAT12_EOF1) {
		return 1;
	}

	return -1;
}

/**
 * Read the specified data from the file, writing the output
 * to the given FILE pointer and returning the number of
 * bytes successfully read or (-1) on failure
 *
 * Notes:
 *    - use the fat12fsLoadDataBlock() function to load the blocks
 *	of data from the file
 */
int
fat12fsReadData(
	struct fat12fs *fs,
	char *buffer,
	const char *filename,
	int startpos,
	int nBytesToCopy)
{
	fprintf(stderr, "fat12fsReadData() -- NOT IMPLEMENTED YET\n");
	/* Not implemented yet, so just return an error */
	/* ??? */
	return -1;
}


