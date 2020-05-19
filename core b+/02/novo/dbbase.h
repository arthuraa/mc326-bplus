#ifndef __DBBASE_H
#define __DBBASE_H

#include "dberr.h"

typedef unsigned long DBID,     *LPDBID;
typedef unsigned long DBOFFSET, *LPDBOFFSET;
typedef unsigned long DBSIZE,   *LPDBSIZE;

typedef struct {
   DBSIZE size;
   void* data;
} DBDATA, *LPDBDATA;

typedef void* HDB,      **LPHDB;
typedef void* HDBINDEX, **LPHDBINDEX;

typedef void (*DBDATAINPROC)(DBID, LPDBDATA, HDBINDEX, void*);
typedef int (*DBDATACMPPROC)(const LPDBDATA, const LPDBDATA, HDBINDEX, void*);
typedef int (*DBTRAVERSEPROC)(HDB, DBID, const LPDBDATA, void*);

DBERR InitializeDBCore(void* (*)(unsigned long), void* (*)(void*, unsigned long), void (*)(void*), unsigned long (*)(void));
DBERR ReleaseDBCore(void);

DBERR InitializeDB(const char*, LPHDB);
DBERR GenerateDB(const char*, LPDBDATA, unsigned short, LPHDB);
DBERR SetDatabaseName(HDB, LPDBDATA);
DBERR CloseDB(LPHDB);

DBERR InitializeIndex(const char*, HDB, LPHDBINDEX);
DBERR GenerateIndex(const char*, LPDBDATA, unsigned short, unsigned short, DBDATAINPROC, DBDATACMPPROC, HDB, LPHDBINDEX);
DBERR SetIndexName(HDBINDEX, LPDBDATA);
DBERR SetIndexFunction(HDBINDEX, DBDATAINPROC, DBDATACMPPROC);
DBERR CloseIndex(LPHDBINDEX);

DBERR InsertDataDB(HDB, DBID, LPDBDATA, void*);
DBERR ScanIDDB(HDB, DBID, LPDBDATA, void*);
DBERR TraverseDB(HDB, DBTRAVERSEPROC, void*);
DBERR RemoveIDDB ( HDB hdb, DBID index, void* param);

#endif

