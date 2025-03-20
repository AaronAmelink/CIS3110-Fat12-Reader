#ifndef	__DOS12_FILESYSTEM_HEADER__
#define	__DOS12_FILESYSTEM_HEADER__


/** define the number of bytes per block */
#define	FS_BLKSIZE	512




/*
 * A DOS/FAT directory entry is a fixed size (32 byte) structure, as
 * follows:
 * (The fields lowercase, ctime100thsec, ctime, cdate, adate, file_block0high
 *  were added for VFAT for Win95. For DOS, they were reserved and
 *  are not used by this program.)
 */
typedef struct fat12fs_DIRENTRY {
	unsigned char	de_name[8];	/* File name */
	unsigned char	de_nameext[3];	/* File name extension .AAA */
	unsigned char	de_attributes;	/* File attributes */
	unsigned char	de_lowercase;	/* Flags for lower case name or ext */
	unsigned char	de_ctime100thsec;	/* 100th sec for create time */
	unsigned char	de_ctime[2];	/* creation time */
	unsigned char	de_cdate[2];	/* and creation date */
	unsigned char	de_adate[2];	/* access date */
	unsigned char	de_file_block0high[2]; /* High 16bits of file_block0 */
	unsigned short	de_modtime;	/* Modify time */
	unsigned short	de_moddate;	/* and data */
	unsigned short	de_fileblock0;	/* first block# of file */
	unsigned int	de_filelen;	/* Length of file, in bytes */
} fat12fs_DIRENTRY;


typedef struct fat12fs  {
	/* file desc to access the device */
	int fs_fd;

	/** data copied or calculated from boot block info */
	unsigned short fs_fatblock;	/* location of first FAT block */
	unsigned short fs_rootdirblock;	/* location of first DIR block */
	unsigned short fs_datablock0;	/* location of "data block 0" */
	unsigned short fs_fatsize;	/* number of entries in FAT table */
	unsigned short fs_fatsectors;	/* number of sectors of FAT info */
	unsigned short fs_rootdirsize;	/* number of entries in rootdir */
	unsigned char fs_numfats;	/* number of copies of FAT table */
	unsigned int fs_fssize;	/* number of data blocks in fs */

	/** working space */
	unsigned char *fs_fatdata;	/* in-memory array of FAT values */ 
	fat12fs_DIRENTRY *fs_rootdirentry; /* in-memory rootdir image */
} fat12fs;


struct fat12fs *fat12fsMount(const char *filename);
int fat12fsUmount(struct fat12fs *fs);
int fat12fsDumpFat(FILE *ofp, struct fat12fs *fs);
int fat12fsDumpRootdir(FILE *ofp, struct fat12fs *fs);

int fat12fsReadData(struct fat12fs *fs,
		char *buffer,
		const char *filename, int startpos, int nbytes);
int fat12fsFindFile(struct fat12fs *fs, const char *filename);
int fat12fsVerifyEOF(struct fat12fs *fs, int dirEntry);


#endif /* __DOS12_FILESYSTEM_HEADER__ */
