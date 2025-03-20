#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "commands.h"
#include "fat12fs.h"

#define	COMMANDLINE_LEN	80
#define MAXTOKENS 4
#define DELIMITERLIST " \t\n"

#define	BASE_16		0
#define	BASE_10		1

static int
printBuffer(FILE *ofp, char *filename, int start, int nBytes, char *buffer)
{
	int i;

	fprintf(ofp, "File '%s' %x bytes beginning at %x\n",
		filename, nBytes, start);

	for (i = 0; i < nBytes; i++) {
		if (isprint(buffer[i]) || buffer[i] == '\n') {
			fputc(buffer[i], ofp);
		} else {
			fprintf(ofp, "\\%03o", (char) buffer[i]);
		}
	}

	fprintf(ofp, "\n");
	return 0;
}

int
processCommands(FILE *ifp, FILE *ofp, struct fat12fs *fs, int displayBase)
{
	char commandbuf[COMMANDLINE_LEN];
	char *tokenList[MAXTOKENS];
	char *conv[] = { "%x", "%d" };
	char *convDesc[] = { "hexadecimal", "decimal" };
	int curBase = BASE_16;
	char *filename, *buffer;
	int start, nBytes;
	int entryIndex;
	int status;
	int tokenIndex;
	int done = 0;

	/** set up the right output number system */
	if (displayBase == 16) {
		curBase = BASE_16;
	} else if (displayBase == 10) {
		curBase = BASE_10;
	} else {
		fprintf(stderr, "I cannot handle base '%d'\n", displayBase);
	}



	/**
	 * read commands one line at a time, processing them as we go
	 */
	while ((!done) && (fgets(commandbuf, COMMANDLINE_LEN, ifp) != NULL)) {

		/**
		 * convert first token
		 */
		tokenIndex = 0;
		tokenList[tokenIndex++] = strtok(commandbuf, DELIMITERLIST);

		/** if nothing was typed, just get a new line */
		if (tokenList[0] == NULL)
			continue;

		/**
		 * put the rest of the tokens into a list
		 */
		while (tokenIndex < MAXTOKENS) {
			tokenList[tokenIndex] = strtok(NULL, DELIMITERLIST);
			/** break if no more tokens */
			if (tokenList[tokenIndex++] == NULL) {
				tokenIndex--;
				break;
			}
		}


		/**
		 * now the tokens are arranged in the token list,
		 * we can simple "run" the command in the first
		 * token and get the arguments as required
		 */
		switch (tokenList[0][0]) {
		case 'q':
			/** we are done, exit the main loop */
			done = 1;
			break;

		case 'f':
			/** dump out the FAT table */
			if (fat12fsDumpFat(ofp, fs) < 0) {
				fprintf(stderr,
					"Failed dumping FAT for filesystem\n");
				return (-1);
			}
			break;

		case 'r':
			/** dump out the FAT table */
			if (fat12fsDumpRootdir(ofp, fs) < 0) {
				fprintf(stderr,
					"Failed dumping filesystem"
					" root directory\n");
				return (-1);
			}
			break;

		case 'b':
			if (tokenIndex < 2) {
				fprintf(stderr, "Need <base>\n");
				continue;
			}
			if ((tokenList[1][0] == 'a')
					|| (tokenList[1][0] == 'A')) {
				printf("Arguments in base-10\n");
				curBase = BASE_10;
			} else {
				printf("Arguments in base-16\n");
				curBase = BASE_16;
			}
			break;

		case 'd':
			if (tokenIndex < 4) {
				fprintf(stderr, "Need <file> <start> <len>\n");
				continue;
			}
			filename = tokenList[1];
			if (sscanf(tokenList[2], conv[curBase], &start) != 1) {
				fprintf(stderr,
					"Cannot convert start position"
						" '%s' to %s\n",
					tokenList[2],
					convDesc[curBase]);
				continue;
			}

			if (sscanf(tokenList[3], conv[curBase], &nBytes) != 1) {
				fprintf(stderr,
					"Cannot convert nbytes"
						" '%s' to %s\n",
					tokenList[3],
					convDesc[curBase]);
				continue;
			}

			/** read data from file in root directory */
			buffer = (char *) malloc(nBytes);
			memset(buffer, 0, nBytes);
			status = fat12fsReadData(fs, buffer, filename,
				start, nBytes);
			if (status < 0) {
				fprintf(stderr,
					"Failed reading %d bytes from"
						" file '%s' at 0x%x\n",
					nBytes, filename, start);
				return (-1);
			}
			fprintf(ofp, "Buffer using return status\n");
			printBuffer(ofp, filename, start, status, buffer);
			fprintf(ofp, "Buffer using request size\n");
			printBuffer(ofp, filename, start, nBytes, buffer);
			free(buffer);
			break;

		case 'v':
			if (tokenIndex < 2) {
				fprintf(stderr, "Need <direntry index>\n");
				continue;
			}
			if (sscanf(tokenList[1],
					conv[curBase],
					&entryIndex) != 1) {
				fprintf(stderr,
					"Cannot convert entry index"
						" '%s' to %s\n",
					tokenList[1],
					convDesc[curBase]);
				continue;
			}

			/** verify that entry is valid */
			status = fat12fsVerifyEOF(fs, entryIndex);
			if (status < 0) {
				fprintf(stderr, "Entry is not a FILE\n");

			} else if (status == 0) {
				fprintf(stderr, "Entry is NOT VALID\n");
			} else {
				fprintf(stderr, "Entry is OK\n");
			}
			break;

		default:
			fprintf(stderr, "Unknown command '%s'\n",
				tokenList[0]);
			fprintf(stderr, "Commands are:\n");
			fprintf(stderr, "  %-26s : %s\n",
				"d <filename> <start> <len>",
				"dump <filename> from <start> for <len> bytes");
			fprintf(stderr, "  %-26s : %s\n",
				"f",
				"print out FAT table ");
			fprintf(stderr, "  %-26s : %s\n",
				"r",
				"print out root directory");
			fprintf(stderr, "  %-26s : %s\n",
				"v <file>",
				"verify <file> and ensure EOF is correct");
			fprintf(stderr, "  %-26s : %s\n",
				"b <base>",
				"switch base for input numbers to be <base>");
		}
	}

	return 1;
}

