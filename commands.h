#ifndef	__COMMANDS_HEADER__
#define	__COMMANDS_HEADER__

#include <stdio.h>
#include "fat12fs.h"

int processCommands(FILE *ifp, FILE *ofp, struct fat12fs *fs, int displayBase);

#endif /* __COMMANDS_HEADER__ */
