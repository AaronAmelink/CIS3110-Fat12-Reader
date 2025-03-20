#  Assignment 4
## CIS\*3110/Operating Systems
## W25

----------------------------------------------------------------

# File Systems: Overview

This assignment will give you some experience reading and navigating
a FAT-12 formatted filesystem.  This filesystem is still commonly
used in embedded systems with small storage requirement, and more
importantly will be more straight-forward for you to implement than
one of the tree-based filesystem structures.

I recommend you work on understanding and "dumping" the FAT data
and root directory before progressing on to the organization of the
data within the file blocks.

# Reading/Navigating FAT Filesystems

Your company, Smart-Aware is in the business of making identity
cards containing various amounts of secure information (biometrics,
etc).

Last month your company bought and took over a much smaller company,
NimbleSoft, which makes a popular smartcard/keycard containing a
little over a Mb of info.

Unfortunately, most of the programming staff of NimbleSoft left
right after the takeover.  Your boss wants you to write something
up to access the data on these cards.

NimbleSoft seems to have almost no documentation, but one of the
managers who remain is sure that the cards use 12-bit FAT, however
with all files in the "root" directory
(*i.e.*; there are no sub-directories).

Most of the code you can find is hand-written in assembler, and
will only run on a different architecture than the one Smart-Aware
uses.  There is the beginning of a re-implementation in C, but some
critical portions of the code are missing.

Your boss wants to you complete this C code to read the data files
from the NimbleSoft devices.  This code must conform to a standard
API (Application Programmers Interface) specified by Smart-Aware.

# Tasks

Some code is provided to get you started.  This is in the directory

	/home/courses/cis3110/assignments/CIS3110-W25-A4-code

Your task is to work in the file `fat12fs.c` and complete the missing
functions in the file.  These functions are:

* `fat12fsDumpFat()`
* `fat12fsDumpRootdir()`
* `fat12fsSearchRootdir()`
* `fat12fsLoadDataBlock()`
* `fat12fsVerifyEOF()`
* `fat12fsReadData()`


# Things to Keep in Mind

## FAT "version" is pointer size

Remember that each version of FAT uses a different number of bits
to store a value in the FAT table.  For the sizes of filesystem you
will be working with, this size will be 12 bits.

Using the assumption of a 12-bit FAT table, you will be able to
read blocks from any real FAT-12 disk image when you are done.
(Typically these are used on SD cards, or were used on floppy disks.)

A "disk image" is simply the contents of an entire raw disk
(*i.e.*; all the raw blocks) saved to a file on a different disk.


Refer to the FAT filesystem notes from class to help you gain an
understanding of how the data will be laid out.  Keep in mind that
the values `0' and `1' are used to mark FAT entries corresponding
to root dir and unused blocks, respectively.  For this reason, the
`real' FAT entry indexes start at 2.

## Handling the end of the file

Your reading code should work like the `read(2)` system call in
terms of handling the end of the file -- that is, if asked to read
*past* the end of the file, all of the bytes up to the end should
be returned, and the length of the read reported should be the
number of valid bytes.  *No data bytes after the end of the file
should be returned.*

As an example, if you have an open file descriptor that is only 15
bytes away from the point where the file ends and a user attempts
to perform a read of 128 bytes, then `read(2)` will place the 15
valid bytes in the buffer it is given and returns a successful
read length of 15 bytes.  Any read performed *after* this point
will return zero bytes to indicate the end of the file.



# Tools and API

All the changes you will make are contained in the file `fat12fs.c`,
which is the only file you will hand in.  The API mentioned above
refers to the function signatures already placed in this file ---
do not change the function names or argument types to any of the
existing functions, however you are free to add more functions to
the file if you find it convenient to do so.

To test your code, the TA's will supply their "test harness", and
your code must link with it.  If you change any of the function
signatures, the code will no longer link.

Some tools are provided to assist you in your task.  In particular,
running `make` in this directory will create two executables:

* `fat12reader` --- a tool linked to `fat12fs.c` which you can use
	to test **your** code.  This is a simple command-based tool which
	will allow you to run the various functions you have been asked to
	write in conjunction with a test filesystem.
* `writedata` --- a simple program which will write blocks composed
	of a single character to a file.  With this function you can create
	files on a FAT-12 disk image which you can then use to test your own
	code.



# Testing Your Code

As you are reading "real" 12-bit FAT filesystem images, you can
create your own by reading or copying any real FAT-12 disk image.  Several
such disk images are provided for you -- these are the files whose
names end with `.fd0`.

To place files into one of these disk images, you can use the `mtools(1)`
tool set, which have been installed for you onto the `linux.socs`
machines.

As an example, the commands below will perform the following three
commands based on the root directory that is stored in the image of the
`smallfiles.fd0` FAT-12 image:

* list the (root) directory contents)
* copy the file `jabber.txt` into your current directory
* copy the file `main.c` into the FAT-12 image

~~~
	mdir -i smallfiles.fd0
	mcopy -i smallfiles.fd0 ::jabber.txt .
	mcopy -i smallfiles.fd0 main.c ::main.c
~~~

As you can see, the arguments `-i smallfile.fd0` indicate the image to
read or write.  Arguments whose names start with `::` refer to a name
within the disk image and arguments without the `::` characters refer
to names of files or directories in the current directory.


## Looking at a raw image

To get a "look" at what is in a FAT filesystem (or other binary
data file), a couple of useful commands are available.


The first is `hexdump(1)` which can print out a file in a combined
hexadecimal byte and ASCII character format exactly like the
`memdbg_map.c` example from the course directory:

	hexdump -C smallfiles.fd0 | more

The command `od(1)` (for "octal dump") for dumping in a variety of
formats (as you probably don't really care about octal).  To print
out the file `smallfiles.fd0` as ASCII bytes and hexadecimal `long`s,
use this command:

	od -t cxL smallfiles.fd0 | more

To print the same file as ASCII bytes and hexadecimal `short`s, use
this:

	od -t cx smallfiles.fd0 | more


# Submission Requirements
The assignment should be handed in as a single `tar(1)` file,
through CourseLink.

Be sure that:

* *your name* is in a comment at the beginning of the program
* you have included a functional `makefile`
* your submission contains a brief discussion describing your program design
	in a file entitled `README.md` or `README.txt`

**Be absolutely sure** that your code compiles and runs on
`linux.socs.uoguelph.ca` using `make(1)`.  It is your responsibility
to ensure that your code works properly.

As before, there are several system dependencies that will differ
having to do with header files and the usage of some of the low
level semaphore primitives that will mean that if you develop your
code elsewhere and hand that in without checking it is unlikely to
compile and run properly on our grading platform.  Whatever you
hand in is what we will grade, so do make sure that you are handing
in working code.

