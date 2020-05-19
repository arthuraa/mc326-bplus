#ifndef __DBCORE_H
#define __DBCORE_H
#define INTERNAL

#define DB_VERSION_VERSION         0x01000000
#define DB_VERSION_SIGNATURE_DATA  0x89ABCDEF
#define DB_VERSION_SIGNATURE_INDEX 0xFEDCBA98
#define DB_DEFAULT_LEAK_IDENTIFIER 0x00000000
#define DB_DEFAULT_IDX0_EXT        ".idx0"
#define DB_DEFAULT_IDX0_EXT_LEN    5

#include <string.h>
#include "debug.h"
#include "dbbase.h"
#include "dbf.h"

/* Data field: compound entry */
typedef union {
   struct {
      DBID id;
      DBDATA data;
   } entry;
   struct {
      DBOFFSET nextBlock;
      DBSIZE leakSize;
   } leak;
} DBENTRY, *LPDBENTRY;

/* Index field: variable entry */
typedef struct {
   unsigned char leafFlag;
   unsigned short blockSize;
   void** data;
   LPDBOFFSET offset;
   DBOFFSET adjacent[2];
} DBINDEX, *LPDBINDEX;

typedef struct {
   DBID id;
   DBOFFSET offset;
} DBILIST, *LPDBILIST;

/* File structure */
typedef struct {
   unsigned long signature;
   unsigned long version;
   unsigned long internalID;
} DBVERSION, *LPDBVERSION;

typedef struct {
   enum { DATA = 0 } handleType;
   DBVERSION version;
   DBDATA databaseInfo;
   DBOFFSET firstLeakBlock;
   FILE* stream;
   HDBINDEX defaultIndex;
   unsigned int indexCount;
   LPHDBINDEX* index;
} _HDB, *_LPHDB;

typedef struct {
   enum { INDEX = 1 } handleType;
   DBVERSION version;
   DBDATA indexInfo;
   DBOFFSET firstLeakBlock;
   DBOFFSET rootBlock;
   unsigned short blockSize;
   unsigned short dataSize;
   FILE* stream;
   HDB parentDB;
   DBINDEX blockBuffer[3];
   DBDATAINPROC dataInProc;
   DBDATACMPPROC dataCmpProc;
} _HDBINDEX, *_LPHDBINDEX;

DBERR INTERNAL WriteDataHeader(HDB);
DBERR INTERNAL WriteIndexHeader(HDBINDEX);

DBERR INTERNAL GetIndexBuffer(LPDBINDEX, unsigned short, unsigned short);
DBERR INTERNAL FreeIndexBuffer(LPDBINDEX, unsigned short);

DBERR INTERNAL ReadIndexBlock(HDBINDEX, DBOFFSET, LPDBINDEX);
DBERR INTERNAL WriteIndexBlock(HDBINDEX, DBOFFSET, LPDBINDEX);

DBERR RemoveIDDB ( HDB, DBID, void*);

DBOFFSET INTERNAL FindFreeDataBlock(HDB, DBSIZE);
DBOFFSET INTERNAL FindFreeIndexBlock(HDBINDEX);
DBOFFSET INTERNAL FindFreeIListBlock(HDBINDEX);

void INTERNAL ReleaseDataBlock(HDB, DBOFFSET);
void INTERNAL ReleaseIndexBlock(HDBINDEX, DBOFFSET);

int INTERNAL NodeSearch(HDBINDEX, LPDBINDEX, int, int, LPDBDATA, void*);
int INTERNAL NodeInsert(HDBINDEX, int, LPDBDATA, DBOFFSET, DBOFFSET);
DBERR INTERNAL ScanInsertIndexRoot(HDBINDEX, LPDBDATA, LPDBOFFSET, LPDBDATA, DBSIZE, void*);
DBERR INTERNAL ScanInsertIndex(HDBINDEX, DBOFFSET, LPDBDATA, LPDBOFFSET, LPDBDATA, DBSIZE, int*, void*);

#ifdef DEBUG
void INTERNAL PrintNode(LPDBINDEX, DBOFFSET, unsigned short);
#else
#define PrintNode(a,b,c) ((void)0)
#endif
#endif

