#ifndef __DBERR_H
#define __DBERR_H

#define DBERR_SUCCESS        0
#define DBERR_ACCESS         1
#define DBERR_MEMORY         2
#define DBERR_IO             3
#define DBERR_ARGUMENT       4
#define DBERR_FORMAT         5
#define DBERR_ALREADY_INIT   6
#define DBERR_NOT_INIT       7
#define DBERR_NOT_FOUND      8
#define DBERR_ENTRY_FOUND    9
#define DBERR_UNKNOWN        0xFFFF

typedef unsigned int DBERR;

DBERR GetLastDBError(void);
DBERR SetLastDBError(DBERR);

#endif

