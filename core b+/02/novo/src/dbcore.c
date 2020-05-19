#include <dbcore.h>

#define checkinit()  { if (!initFlag) error(DBERR_NOT_INIT); }
#define checkdata(x) { checkinit(); if (dbcast(x)->handleType != DATA) error(DBERR_ARGUMENT); }
#define checkidx(x)  { checkinit(); if (idxcast(x)->handleType != INDEX) error(DBERR_ARGUMENT); }
#define checkf(x)    { checkidx(x); if (!idxcast(x)->dataInProc || !idxcast(x)->dataCmpProc) error(DBERR_NOT_INIT); }
#define isidx0(x)    (idxcast(x)->dataInProc == DefaultDataInProc && idxcast(x)->dataCmpProc == DefaultDataCmpProc)
#define error(x)     { DBERR err = x; leave(); SetLastDBError(err); return err; }

#define dbcast(x)    ((_LPHDB)((x)))
#define idxcast(x)   ((_LPHDBINDEX)((x)))
#define parent(x)    dbcast(idxcast((x))->parentDB)
#define defIndex(x)  idxcast(dbcast(x)->defaultIndex)

#ifdef DEBUG
static unsigned int callDepth = 0;
void skipdepth(unsigned int depth) { unsigned int i; for (i = 0; i < 2*depth; ++i) fputc(' ',stderr); fprintf(stderr,"(%02i)",depth); }
#define sd skipdepth(callDepth)
#define flag(x) { sd; mark(x); }
#else
#define sd      ((void)0)
#define flag(x) ((void)0)
#endif
#if defined(DEBUG) && !defined(NENTER)
#define enter() { skipdepth(++callDepth); mark("enter\n"); }
#define leave() { skipdepth(callDepth--); mark("leave\n"); }
#else
#define enter() ((void)0)
#define leave() ((void)0)
#endif

void* (*allocfn)(unsigned long);
void* (*reallocfn)(void*, unsigned long);
void  (*freefn)(void*);
unsigned long (*randcall)(void);
int initFlag = 0;

DBERR lastErrorCode = DBERR_SUCCESS;

char* idx0(const char* filename) {
   static char name[0x100];
   strncpy(name,filename,sizeof(name)-(DB_DEFAULT_IDX0_EXT_LEN+1));
   return strcat(name,DB_DEFAULT_IDX0_EXT);
}

void DefaultDataInProc(DBID id, LPDBDATA data, HDBINDEX index, void* param) {
   data->size = sizeof(DBID);
   *((LPDBID)data->data) = id;
}

int DefaultDataCmpProc(const LPDBDATA data1, const LPDBDATA data2, HDBINDEX index, void* param) {
   DBID id1 = *((LPDBID)data1->data), id2 = *((LPDBID)data2->data);
   return id1 > id2 ? 1 : (id1 < id2 ? -1 : 0);
}

DBERR GetLastDBError(void) { return lastErrorCode; }
DBERR SetLastDBError(DBERR err) { DBERR lastErr = lastErrorCode; lastErrorCode = err; return lastErr; }

DBERR InitializeDBCore(void* (*af)(unsigned long), void* (*raf)(void*, unsigned long), void (*ff)(void*), unsigned long (*rc)(void)) {
   enter();
   if (initFlag) error(DBERR_ALREADY_INIT);
   if (!af || !raf || !ff || !rc) error(DBERR_ACCESS);
   initFlag = 1; allocfn = af; reallocfn = raf; freefn = ff; randcall = rc;
   error(DBERR_SUCCESS);
}

DBERR ReleaseDBCore(void) {
   enter();
   checkinit();
   initFlag = 0;
   error(DBERR_SUCCESS);
}

DBERR InitializeDB(const char* filename, LPHDB hdb) {
   FILE* stream;
   _LPHDB db;
   enter();
   if (!filename || !hdb) error(DBERR_ACCESS);
   checkinit();
   if ((stream = fopen(filename,"r+b")) == NULL) error(DBERR_IO);
   if ((db = (_LPHDB)allocfn(sizeof(_HDB))) == NULL) {
      err: fclose(stream);
      error(DBERR_MEMORY);
   }
   freadp(&db->version,sizeof(db->version),0,SEEK_SET,stream);
   if (db->version.signature != DB_VERSION_SIGNATURE_DATA || db->version.version != DB_VERSION_VERSION) {
      freefn(db);
      fclose(stream);
      error(DBERR_FORMAT);
   }
   freadp(&db->databaseInfo.size,sizeof(db->databaseInfo.size),0,SEEK_CUR,stream);
   if (db->databaseInfo.size) {
      if ((db->databaseInfo.data = allocfn(db->databaseInfo.size)) == NULL) {
         freefn(db);
         goto err;
      }
      freadp(db->databaseInfo.data,db->databaseInfo.size,0,SEEK_CUR,stream);
   } else db->databaseInfo.data = NULL;
   freadp(&db->firstLeakBlock,sizeof(db->firstLeakBlock),0,SEEK_CUR,stream);
   db->handleType = DATA;
   db->indexCount = 0;
   db->index = NULL;
   if (InitializeIndex(idx0(filename),(HDB)db,&db->defaultIndex) != DBERR_SUCCESS) {
      if (db->databaseInfo.data) freefn(db->databaseInfo.data);
      freefn(db);
      fclose(stream);
      error(GetLastDBError());
   }
   SetIndexFunction(db->defaultIndex,DefaultDataInProc,DefaultDataCmpProc);
   db->stream = stream;
   *hdb = (HDB)db;
   error(DBERR_SUCCESS);
}

DBERR GenerateDB(const char* filename, LPDBDATA data, unsigned short blockSize, LPHDB hdb) {
   _HDB db;
   enter();
   if (!filename || !hdb) error(DBERR_ACCESS);
   checkinit();
   if ((db.stream = fopen(filename,"w+b")) == NULL) error(DBERR_IO);
   if (data) {
      db.databaseInfo.size = data->size;
      db.databaseInfo.data = data->data;
   }
   else {
      db.databaseInfo.size = 0;
      db.databaseInfo.data = NULL;
   }
   db.handleType = DATA;
   db.version.signature = DB_VERSION_SIGNATURE_DATA;
   db.version.version = DB_VERSION_VERSION;
   db.version.internalID = randcall();
   db.firstLeakBlock = 0;
   if (WriteDataHeader((HDB)&db) != DBERR_SUCCESS) {
      err: fclose(db.stream);
      error(GetLastDBError());
   }
   if (GenerateIndex(idx0(filename),NULL,blockSize,sizeof(DBID),DefaultDataInProc,DefaultDataCmpProc,(HDB)&db,&db.defaultIndex) != DBERR_SUCCESS)
      goto err;
   fclose(db.stream);
   error(InitializeDB(filename,hdb));
}

DBERR SetDatabaseName(HDB hdb, LPDBDATA data) {
   enter();
   if (!hdb || !data) error(DBERR_ACCESS);
   checkdata(hdb);
   if (data->size > dbcast(hdb)->databaseInfo.size) error(DBERR_ARGUMENT);
   memset(dbcast(hdb)->databaseInfo.data,0,dbcast(hdb)->databaseInfo.size);
   memcpy(dbcast(hdb)->databaseInfo.data,data->data,data->size);
   error(DBERR_SUCCESS);
}

DBERR CloseDB(LPHDB hdb) {
   enter();
   if (!hdb) error(DBERR_ACCESS);
   if (!*hdb) error(DBERR_ACCESS);
   checkdata(*hdb);
   if (WriteDataHeader(dbcast(*hdb)) != DBERR_SUCCESS) error(GetLastDBError());
   fclose(dbcast(*hdb)->stream);
   while (dbcast(*hdb)->indexCount) if (CloseIndex((LPHDBINDEX)dbcast(*hdb)->index[0]) != DBERR_SUCCESS) break;
   if (dbcast(*hdb)->databaseInfo.data) freefn(dbcast(*hdb)->databaseInfo.data);
   freefn(dbcast(*hdb));
   *hdb = NULL;
   error(DBERR_SUCCESS);
}

DBERR InitializeIndex(const char* filename, HDB hdb, LPHDBINDEX hdbIndex) {
   void* ptr;
   FILE* stream;
   _LPHDBINDEX dbIndex;
   enter();
   if (!filename || !hdb || !hdbIndex) error(DBERR_ACCESS);
   checkdata(hdb);
   if ((stream = fopen(filename,"r+b")) == NULL) error(DBERR_IO);
   if ((dbIndex = (_LPHDBINDEX)allocfn(sizeof(_HDBINDEX))) == NULL) {
      err: fclose(stream);
      error(DBERR_MEMORY);
   }
   if ((ptr = reallocfn(dbcast(hdb)->index,(dbcast(hdb)->indexCount+1)*sizeof(LPHDBINDEX))) == NULL) {
      err2: freefn(dbIndex);
      goto err;
   }
   else dbcast(hdb)->index = ptr;
   freadp(&dbIndex->version,sizeof(dbIndex->version),0,SEEK_SET,stream);
   if (dbIndex->version.signature != DB_VERSION_SIGNATURE_INDEX ||
       dbIndex->version.version != DB_VERSION_VERSION ||
       dbIndex->version.internalID != dbcast(hdb)->version.internalID) {
      freefn(dbIndex);
      fclose(stream);
      error(DBERR_FORMAT);
   }
   freadp(&dbIndex->indexInfo.size,sizeof(dbIndex->indexInfo.size),0,SEEK_CUR,stream);
   if (dbIndex->indexInfo.size) {
      if ((dbIndex->indexInfo.data = allocfn(dbIndex->indexInfo.size)) == NULL) goto err2;
      freadp(dbIndex->indexInfo.data,dbIndex->indexInfo.size,0,SEEK_CUR,stream);
   } else dbIndex->indexInfo.data = NULL;
   freadp(&dbIndex->firstLeakBlock,sizeof(dbIndex->firstLeakBlock),0,SEEK_CUR,stream);
   freadp(&dbIndex->rootBlock,sizeof(dbIndex->rootBlock),0,SEEK_CUR,stream);
   freadp(&dbIndex->blockSize,sizeof(dbIndex->blockSize),0,SEEK_CUR,stream);
   freadp(&dbIndex->dataSize,sizeof(dbIndex->dataSize),0,SEEK_CUR,stream);
   if (GetIndexBuffer(&dbIndex->blockBuffer[0],dbIndex->blockSize,dbIndex->dataSize) != DBERR_SUCCESS) {
      err3: if (dbIndex->indexInfo.data) freefn(dbIndex->indexInfo.data);
      freefn(dbIndex);
      fclose(stream);
      error(GetLastDBError());
   }
   if (GetIndexBuffer(&dbIndex->blockBuffer[1],dbIndex->blockSize,dbIndex->dataSize) != DBERR_SUCCESS) {
      err4: FreeIndexBuffer(&dbIndex->blockBuffer[0],dbIndex->blockSize);
      goto err3;
   }
   if (GetIndexBuffer(&dbIndex->blockBuffer[2],dbIndex->blockSize,dbIndex->dataSize) != DBERR_SUCCESS) {
      FreeIndexBuffer(&dbIndex->blockBuffer[1],dbIndex->blockSize);
      goto err4;
   }
   dbIndex->handleType = INDEX;
   dbIndex->stream = stream;
   dbIndex->parentDB = hdb;
   dbIndex->dataInProc = NULL;
   dbIndex->dataCmpProc = NULL;
   dbcast(hdb)->index[dbcast(hdb)->indexCount++] = hdbIndex;
   *hdbIndex = (HDBINDEX)dbIndex;
   error(DBERR_SUCCESS);
}

DBERR GenerateIndex(const char* filename, LPDBDATA data, unsigned short blockSize, unsigned short dataSize, DBDATAINPROC dataInProc, DBDATACMPPROC dataCmpProc, HDB hdb, LPHDBINDEX hdbIndex) {
   _HDBINDEX dbIndex;
   enter();
   if (!filename || !hdb || !hdbIndex || !dataInProc || !dataCmpProc) error(DBERR_ACCESS);
   if (blockSize < 2 || !dataSize) error(DBERR_ARGUMENT);
   checkdata(hdb);
   if ((dbIndex.stream = fopen(filename,"w+b")) == NULL) error(DBERR_IO);
   if (data) {
      dbIndex.indexInfo.size = data->size;
      dbIndex.indexInfo.data = data->data;
   }
   else {
      dbIndex.indexInfo.size = 0;
      dbIndex.indexInfo.data = NULL;
   }
   dbIndex.version.signature = DB_VERSION_SIGNATURE_INDEX;
   dbIndex.version.version = DB_VERSION_VERSION;
   dbIndex.version.internalID = dbcast(hdb)->version.internalID;
   dbIndex.firstLeakBlock = 0;
   dbIndex.rootBlock = 0;
   dbIndex.blockSize = blockSize;
   dbIndex.dataSize = dataSize;
   if (dataInProc == DefaultDataInProc && dataCmpProc == DefaultDataCmpProc) { /* Primary index */
      if (WriteIndexHeader((HDBINDEX)&dbIndex) != DBERR_SUCCESS) {
         fclose(dbIndex.stream);
         error(GetLastDBError());
      }
      fclose(dbIndex.stream);
      error(DBERR_SUCCESS);
   }
   else {
      /* Things go MAD here */
   }
   error(DBERR_UNKNOWN);
}

DBERR SetIndexFunction(HDBINDEX hdbIndex, DBDATAINPROC inProc, DBDATACMPPROC cmpProc) {
   enter();
   if (!hdbIndex || !inProc || !cmpProc) error(DBERR_ACCESS);
   checkidx(hdbIndex);
   idxcast(hdbIndex)->dataInProc = inProc;
   idxcast(hdbIndex)->dataCmpProc = cmpProc;
   error(DBERR_SUCCESS);
}

DBERR SetIndexName(HDBINDEX hdbIndex, LPDBDATA data) {
   enter();
   if (!hdbIndex || !data) error(DBERR_ACCESS);
   checkidx(hdbIndex);
   if (data->size > idxcast(hdbIndex)->indexInfo.size) error(DBERR_ARGUMENT);
   memset(idxcast(hdbIndex)->indexInfo.data,0,idxcast(hdbIndex)->indexInfo.size);
   memcpy(idxcast(hdbIndex)->indexInfo.data,data->data,data->size);
   error(DBERR_SUCCESS);
}

DBERR CloseIndex(LPHDBINDEX hdbIndex) {
   unsigned int count;
   enter();
   if (!hdbIndex) error(DBERR_ACCESS);
   if (!*hdbIndex) error(DBERR_ACCESS);
   checkidx(*hdbIndex);
   if (WriteIndexHeader(idxcast(*hdbIndex)) != DBERR_SUCCESS) error(GetLastDBError());
   fclose(idxcast(*hdbIndex)->stream);
   if (parent(*hdbIndex)->indexCount > 1) {
      for (count = 0; count < parent(*hdbIndex)->indexCount; ++count) if (parent(*hdbIndex)->index[count] == hdbIndex) break;
      parent(*hdbIndex)->index[count] = parent(*hdbIndex)->index[--parent(*hdbIndex)->indexCount];
   }
   else {
      freefn(parent(*hdbIndex)->index);
      parent(*hdbIndex)->index = NULL;
      parent(*hdbIndex)->indexCount = 0;
   }
   if (idxcast(*hdbIndex)->indexInfo.data) freefn(idxcast(*hdbIndex)->indexInfo.data);
   for (count = 0; count < 3; ++count) FreeIndexBuffer(&idxcast(*hdbIndex)->blockBuffer[count],idxcast(*hdbIndex)->blockSize);
   freefn(idxcast(*hdbIndex));
   *hdbIndex = NULL;
   error(DBERR_SUCCESS);
}

DBERR INTERNAL WriteDataHeader(HDB hdb) {
   enter();
   fwritep(&dbcast(hdb)->version,sizeof(dbcast(hdb)->version),0,SEEK_SET,dbcast(hdb)->stream);
   fwrited(dbcast(hdb)->databaseInfo.data,dbcast(hdb)->databaseInfo.size,0,SEEK_CUR,dbcast(hdb)->stream);
   fwritep(&dbcast(hdb)->firstLeakBlock,sizeof(dbcast(hdb)->firstLeakBlock),0,SEEK_CUR,dbcast(hdb)->stream);
   error(DBERR_SUCCESS);
}

DBERR INTERNAL WriteIndexHeader(HDBINDEX hdbIndex) {
   enter();
   fwritep(&idxcast(hdbIndex)->version,sizeof(idxcast(hdbIndex)->version),0,SEEK_SET,idxcast(hdbIndex)->stream);
   fwrited(idxcast(hdbIndex)->indexInfo.data,idxcast(hdbIndex)->indexInfo.size,0,SEEK_CUR,idxcast(hdbIndex)->stream);
   fwritep(&idxcast(hdbIndex)->firstLeakBlock,sizeof(idxcast(hdbIndex)->firstLeakBlock),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   fwritep(&idxcast(hdbIndex)->rootBlock,sizeof(idxcast(hdbIndex)->rootBlock),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   fwritep(&idxcast(hdbIndex)->blockSize,sizeof(idxcast(hdbIndex)->blockSize),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   fwritep(&idxcast(hdbIndex)->dataSize,sizeof(idxcast(hdbIndex)->dataSize),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   error(DBERR_SUCCESS);
}

DBERR INTERNAL GetIndexBuffer(LPDBINDEX buffer, unsigned short blockSize, unsigned short dataSize) {
   unsigned short count;
   enter();
   if ((buffer->data = allocfn((blockSize+1)*sizeof(DBDATA))) == NULL) error(DBERR_MEMORY);
   if ((buffer->offset = allocfn((blockSize+2)*sizeof(DBOFFSET))) == NULL) {
      err: freefn(buffer->data);
      error(DBERR_MEMORY);
   }
   for (count = 0; count < blockSize+1; ++count) if ((buffer->data[count] = allocfn(dataSize)) == NULL) {
      for (blockSize = count, count = 0; count < blockSize; ++count) freefn(buffer->data[count]);
      freefn(buffer->offset);
      goto err;
   }
   error(DBERR_SUCCESS);
}

DBERR INTERNAL FreeIndexBuffer(LPDBINDEX buffer, unsigned short blockSize) {
   unsigned short count;
   enter();
   for (count = 0; count < blockSize+1; ++count) freefn(buffer->data[count]);
   freefn(buffer->data);
   freefn(buffer->offset);
   error(DBERR_SUCCESS);
}

DBERR INTERNAL ReadIndexBlock(HDBINDEX hdbIndex, DBOFFSET offset, LPDBINDEX buffer) {
   unsigned short count;
   enter();
   freadp(&buffer->leafFlag,sizeof(buffer->leafFlag),offset,SEEK_SET,idxcast(hdbIndex)->stream);
   freadp(&buffer->blockSize,sizeof(buffer->blockSize),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   flag("Reading data buffers "); value(idxcast(hdbIndex)->blockSize,"(%i blocks)\n");
   for (count = 0; count < idxcast(hdbIndex)->blockSize; ++count)
      freadp(buffer->data[count],idxcast(hdbIndex)->dataSize,0,SEEK_CUR,idxcast(hdbIndex)->stream);
   flag("Reading offset buffers\n");
   freadp(buffer->offset,(idxcast(hdbIndex)->blockSize+1)*sizeof(DBOFFSET),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   freadp(&buffer->adjacent[1],sizeof(buffer->adjacent[1]),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   if (buffer->leafFlag) buffer->adjacent[0] = buffer->offset[idxcast(hdbIndex)->blockSize];
   error(DBERR_SUCCESS);
}

DBERR INTERNAL WriteIndexBlock(HDBINDEX hdbIndex, DBOFFSET offset, LPDBINDEX buffer) {
   unsigned short count;
   enter();
   fwritep(&buffer->leafFlag,sizeof(buffer->leafFlag),offset,SEEK_SET,idxcast(hdbIndex)->stream);
   fwritep(&buffer->blockSize,sizeof(buffer->blockSize),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   flag("Writing data buffers "); value(idxcast(hdbIndex)->blockSize,"(%i blocks)\n");
   for (count = 0; count < idxcast(hdbIndex)->blockSize; ++count)
      fwritep(buffer->data[count],idxcast(hdbIndex)->dataSize,0,SEEK_CUR,idxcast(hdbIndex)->stream);
   flag("Writing offset buffers\n");
   fwritep(buffer->offset,(idxcast(hdbIndex)->blockSize+1)*sizeof(DBOFFSET),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   fwritep(&buffer->adjacent[1],sizeof(buffer->adjacent[1]),0,SEEK_CUR,idxcast(hdbIndex)->stream);
   error(DBERR_SUCCESS);
}

DBOFFSET INTERNAL FindFreeDataBlock(HDB hdb, DBSIZE size) {
   DBOFFSET offset = dbcast(hdb)->firstLeakBlock, prev;
   DBENTRY leak;
   for (prev = 0, offset = dbcast(hdb)->firstLeakBlock; offset; prev = offset, offset = leak.leak.nextBlock) {
      freadp(&leak,sizeof(leak),offset,SEEK_SET,dbcast(hdb)->stream);
      if (leak.leak.leakSize >= size+sizeof(DBOFFSET)+sizeof(DBSIZE)) break;
   }
   if (offset) {
      leak.leak.leakSize -= size;
      if (!prev) dbcast(hdb)->firstLeakBlock = offset+size;
      else {
         fwritep(&leak,sizeof(leak),offset+size,SEEK_SET,dbcast(hdb)->stream);
         leak.leak.nextBlock = offset+size;
         fwritep(&leak.leak.nextBlock,sizeof(leak.leak.nextBlock),prev,SEEK_SET,dbcast(hdb)->stream);
      }
   }
   else { fseek(dbcast(hdb)->stream,0,SEEK_END); offset = ftell(dbcast(hdb)->stream); }
   return offset;
}

DBOFFSET INTERNAL FindFreeIndexBlock(HDBINDEX hdbIndex) {
   DBOFFSET offset = idxcast(hdbIndex)->firstLeakBlock;
   if (offset) freadp(&idxcast(hdbIndex)->firstLeakBlock,sizeof(idxcast(hdbIndex)->firstLeakBlock),offset,SEEK_SET,idxcast(hdbIndex)->stream);
   else { fseek(idxcast(hdbIndex)->stream,0,SEEK_END); offset = ftell(idxcast(hdbIndex)->stream); }
   return offset;
}

DBOFFSET INTERNAL FindFreeIListBlock(HDBINDEX hdbIndex) {
   fseek(idxcast(hdbIndex)->stream,0,SEEK_END);
   return ftell(idxcast(hdbIndex)->stream);
}

void INTERNAL ReleaseDataBlock(HDB hdb, DBOFFSET offset) {
   DBSIZE size;
   freadp(&size,sizeof(size),offset+sizeof(DBID),SEEK_SET,dbcast(hdb)->stream);
   size += sizeof(DBID);
   fwritep(&dbcast(hdb)->firstLeakBlock,sizeof(dbcast(hdb)->firstLeakBlock),offset,SEEK_SET,dbcast(hdb)->stream);
   fwritep(&size,sizeof(size),0,SEEK_CUR,dbcast(hdb)->stream);
   dbcast(hdb)->firstLeakBlock = offset;
}

void INTERNAL ReleaseIndexBlock(HDBINDEX hdbIndex, DBOFFSET offset) {
   fwritep(&idxcast(hdbIndex)->firstLeakBlock,sizeof(idxcast(hdbIndex)->firstLeakBlock),offset,SEEK_SET,idxcast(hdbIndex)->stream);
   idxcast(hdbIndex)->firstLeakBlock = offset;
}

int INTERNAL NodeSearch(HDBINDEX hdbIndex, LPDBINDEX index, int start, int end, LPDBDATA data, void* param) {
   DBDATA localData;
   int mid = (start+end)/2, cmp;
   localData.size = idxcast(hdbIndex)->dataSize; localData.data = index->data[mid];
   cmp = idxcast(hdbIndex)->dataCmpProc(&localData,data,hdbIndex,param);
   if (mid == start) {
      if (cmp > 0) return -(start+1);
      else if (cmp < 0) return -(end+1);
      else return mid;
   }
   if (cmp > 0) return NodeSearch(hdbIndex,index,start,mid,data,param);
   else if (cmp < 0) return NodeSearch(hdbIndex,index,mid,end,data,param);
   else return mid;
}

#define buffer(x) idxcast(hdbIndex)->blockBuffer[x]
int INTERNAL NodeInsert(HDBINDEX hdbIndex, int pos, LPDBDATA data, DBOFFSET insert, DBOFFSET current) {
   unsigned short count;
   enter();
   flag("Inserting at "); var(pos,"%i, previous node:\n"); PrintNode(&buffer(0),current,idxcast(hdbIndex)->dataSize);
   flag("Data to insert: "); var((unsigned long)data->data,"%08lX, "); dump(data->data,data->size); nl;
   flag("Offset to insert: "); var(insert,"%08lX, "); var(current,"%08lX\n");
   for (count = buffer(0).blockSize; count > pos; --count)
      memcpy(buffer(0).data[count],buffer(0).data[count-1],idxcast(hdbIndex)->dataSize); /* Make better */
   memset(buffer(0).data[pos],0,idxcast(hdbIndex)->dataSize);
   memcpy(buffer(0).data[pos],data->data,data->size);
   memmove(&buffer(0).offset[pos+1+!buffer(0).leafFlag],&buffer(0).offset[pos+!buffer(0).leafFlag],(buffer(0).blockSize-pos)*sizeof(DBOFFSET));
   buffer(0).offset[pos+!buffer(0).leafFlag] = insert;
   ++buffer(0).blockSize;
   flag("New node:\n"); PrintNode(&buffer(0),current,idxcast(hdbIndex)->dataSize);
   if (buffer(0).blockSize > idxcast(hdbIndex)->blockSize) {
      pos = buffer(0).blockSize/2;
      for (buffer(1).blockSize = 0; buffer(1).blockSize < buffer(0).blockSize-(pos+!buffer(0).leafFlag); ++buffer(1).blockSize) {
         memcpy(buffer(1).data[buffer(1).blockSize],buffer(0).data[buffer(1).blockSize+pos+!buffer(0).leafFlag],idxcast(hdbIndex)->dataSize);
         buffer(1).offset[buffer(1).blockSize] = buffer(0).offset[buffer(1).blockSize+pos+!buffer(0).leafFlag];
      }
      if (!buffer(0).leafFlag) buffer(1).offset[buffer(1).blockSize] = buffer(0).offset[buffer(0).blockSize];
      buffer(0).blockSize -= pos;
      if ((buffer(1).leafFlag = buffer(0).leafFlag) != 0) {
         buffer(1).adjacent[1] = buffer(0).adjacent[1];
         buffer(1).adjacent[0] = current;
      }
      buffer(0).adjacent[1] = FindFreeIndexBlock(hdbIndex);
      memcpy(data->data,buffer(0).data[pos],idxcast(hdbIndex)->dataSize);
      flag("Left node generated:\n"); PrintNode(&buffer(0),current,idxcast(hdbIndex)->dataSize);
      flag("Right node generated:\n"); PrintNode(&buffer(1),buffer(0).adjacent[1],idxcast(hdbIndex)->dataSize);
      leave();
      return 1; /* Block to insert: buffer(1); Data to insert: data; Address: buffer(0).adjacent[1] */
   }
   leave();
   return 0;
}

#ifdef GETOUT
DBERR INTERNAL ScanInsertIndex(HDBINDEX hdbIndex, DBOFFSET offset, LPDBDATA data, DBOFFSET insert, void* param) {
   static int insertFlag = 0;
   int pos;
   enter();
   if (!offset) {
      if (!idxcast(hdbIndex)->rootBlock) {
         if (!insert) error(DBERR_NOT_FOUND);
         buffer(0).leafFlag = 1; buffer(0).blockSize = 0; buffer(0).adjacent[0] = buffer(0).adjacent[1] = 0;
         NodeInsert(hdbIndex,0,data,insert,0);
         WriteIndexBlock(hdbIndex,(idxcast(hdbIndex)->rootBlock = FindFreeIndexBlock(hdbIndex)),&buffer(0));
         error(DBERR_SUCCESS);
      }
      if (ScanInsertIndex(hdbIndex,idxcast(hdbIndex)->rootBlock,data,insert,param) != DBERR_SUCCESS) error(GetLastDBError());
      if (insertFlag) {
         insert = buffer(0).adjacent[1];
         WriteIndexBlock(hdbIndex,insert,&buffer(1));
         buffer(0).leafFlag = 0; buffer(0).blockSize = 0;
         NodeInsert(hdbIndex,0,data,insert,0);
         buffer(0).offset[0] = idxcast(hdbIndex)->rootBlock;
         idxcast(hdbIndex)->rootBlock = FindFreeIndexBlock(hdbIndex);
         WriteIndexBlock(hdbIndex,idxcast(hdbIndex)->rootBlock,&buffer(0));
      }
      error(DBERR_SUCCESS);
   }
   ReadIndexBlock(hdbIndex,offset,&buffer(0));
   pos = NodeSearch(hdbIndex,&buffer(0),0,buffer(0).blockSize,data,param);
   if (buffer(0).leafFlag) {
      flag("Leaf reached\n");
      if (pos >= 0) {
         if (defIndex(hdbIndex)) {
            if (!insert) { /* Load data here */ }
            error(DBERR_ENTRY_FOUND);
         }
      }
      else {
         if (!insert) error(DBERR_NOT_FOUND);
         pos = -(pos+1);
         insertFlag = NodeInsert(hdbIndex,pos,data,insert,offset);
         WriteIndexBlock(hdbIndex,offset,&buffer(0));
      }
   }
   else {
      if (pos < 0) pos = -(pos+1);
      else ++pos;
      if (ScanInsertIndex(hdbIndex,buffer(0).offset[pos],data,insert,param) != DBERR_SUCCESS) error(GetLastDBError());
      if (insertFlag) {
         insert = buffer(0).adjacent[1];
         WriteIndexBlock(hdbIndex,insert,&buffer(1));
         ReadIndexBlock(hdbIndex,offset,&buffer(0));
         insertFlag = NodeInsert(hdbIndex,pos,data,insert,offset);
         WriteIndexBlock(hdbIndex,offset,&buffer(0));
      }
   }
   error(DBERR_SUCCESS);
}
#undef buffer

DBERR InsertDataDB(HDB hdb, DBID id, LPDBDATA data, void* param) {
   DBDATA localData;
   DBOFFSET offset;
   enter();
   if (!hdb || !data) error(DBERR_ACCESS);
   if (!id) error(DBERR_ARGUMENT);
   checkdata(hdb);
   offset = FindFreeDataBlock(hdb,data->size);
   localData.data = &id;
   localData.size = sizeof(DBID);
   error(ScanInsertIndex(defIndex(hdb),0,&localData,offset,param));
}
#endif

DBERR INTERNAL ScanInsertIndexRoot(HDBINDEX hdbIndex, LPDBDATA data, LPDBOFFSET insert, LPDBDATA dataBuffer, DBSIZE size, void* param) {
   int insertFlag = 0;
   enter();
   if (!idxcast(hdbIndex)->rootBlock) {
      if (!insert) error(DBERR_NOT_FOUND);
      if (isidx0(hdbIndex)) *insert = FindFreeDataBlock(parent(hdbIndex),size);
      else {
         DBID id = (DBID)insert;
         fwritep(&id,sizeof(id),(*insert = FindFreeIListBlock(hdbIndex)),SEEK_SET,idxcast(hdbIndex)->stream); id = 0;
         fwritep(&id,sizeof(id),0,SEEK_CUR,idxcast(hdbIndex)->stream);
      }
      buffer(0).leafFlag = 1; buffer(0).blockSize = 0; buffer(0).adjacent[0] = buffer(0).adjacent[1] = 0;
      NodeInsert(hdbIndex,0,data,*insert,0);
      goto write;
   }
   if (ScanInsertIndex(hdbIndex,idxcast(hdbIndex)->rootBlock,data,insert,dataBuffer,size,&insertFlag,param) != DBERR_SUCCESS) error(GetLastDBError());
   if (insertFlag) {
      WriteIndexBlock(hdbIndex,buffer(0).adjacent[1],&buffer(1));
      buffer(0).leafFlag = 0; buffer(0).blockSize = 0;
      NodeInsert(hdbIndex,0,data,buffer(0).adjacent[1],0);
      buffer(0).offset[0] = idxcast(hdbIndex)->rootBlock;
      flag("New root block:\n"); PrintNode(&buffer(0),0,idxcast(hdbIndex)->dataSize);
      write: WriteIndexBlock(hdbIndex,(idxcast(hdbIndex)->rootBlock = FindFreeIndexBlock(hdbIndex)),&buffer(0));
   }
   error(DBERR_SUCCESS);
}

DBERR INTERNAL ScanInsertIndex(HDBINDEX hdbIndex, DBOFFSET offset, LPDBDATA data, LPDBOFFSET insert, LPDBDATA dataBuffer, DBSIZE size, int* insertFlag, void* param) {
   int pos;
   enter();
   ReadIndexBlock(hdbIndex,offset,&buffer(0));
   pos = NodeSearch(hdbIndex,&buffer(0),0,buffer(0).blockSize,data,param);
   if (buffer(0).leafFlag) {
      flag("Leaf reached\n");
      if (pos >= 0) {
         if (defIndex(hdbIndex)) {
            if (!insert && dataBuffer) {
               /* Load */
               error(DBERR_SUCCESS);
            }
            error(DBERR_ENTRY_FOUND);
         }
         else {
            /* Look for element in ilist */
            if (insert) {
               /* Fail if found */
            }
            else {
               /* Fail if not found */
            }
         }
      }
      else {
         if (!insert) error(DBERR_NOT_FOUND);
         if (isidx0(hdbIndex)) *insert = FindFreeDataBlock(parent(hdbIndex),size);
         else {
            *insert = FindFreeIListBlock(hdbIndex);
            /* Add to ilist here */
         }
         *insertFlag = NodeInsert(hdbIndex,-(pos+1),data,*insert,offset);
         goto write;
      }
   }
   else {
      if (pos < 0) pos = -(pos+1);
      else ++pos;
      if (ScanInsertIndex(hdbIndex,buffer(0).offset[pos],data,insert,dataBuffer,size,insertFlag,param) != DBERR_SUCCESS) error(GetLastDBError());
      if (*insertFlag) {
         DBOFFSET newBlock = buffer(0).adjacent[1];
         WriteIndexBlock(hdbIndex,newBlock,&buffer(1));
         ReadIndexBlock(hdbIndex,offset,&buffer(0));
         *insertFlag = NodeInsert(hdbIndex,pos,data,newBlock,offset);
         write: WriteIndexBlock(hdbIndex,offset,&buffer(0));
      }
   }
   error(DBERR_SUCCESS);
}
#undef buffer

DBERR InsertDataDB(HDB hdb, DBID id, LPDBDATA data, void* param) {
   DBDATA localData;
   DBOFFSET offset;
   DBID localID;
   enter();
   if (!hdb || !data) error(DBERR_ACCESS);
   if (!id) error(DBERR_ARGUMENT);
   checkdata(hdb);
   localData.size = sizeof(id);
   localData.data = &id;
   localID = id;
   if (ScanInsertIndexRoot(defIndex(hdb),&localData,&offset,NULL,data->size,param) != DBERR_SUCCESS) error(GetLastDBError());
   fwritep(&localID,sizeof(localID),offset,SEEK_SET,dbcast(hdb)->stream);
   fwrited(data->data,data->size,0,SEEK_CUR,dbcast(hdb)->stream);
   error(DBERR_SUCCESS);
}

DBERR ScanIDDB(HDB hdb, DBID id, LPDBDATA data, void* param) {
   DBDATA localData;
   enter();
   if (!hdb) error(DBERR_ACCESS);
   if (!id) error(DBERR_ARGUMENT);
   checkdata(hdb);
   localData.size = sizeof(id);
   localData.data = &id;
   error(ScanInsertIndexRoot(defIndex(hdb),&localData,NULL,data,0,param));
}

DBERR TraverseDB(HDB hdb, DBTRAVERSEPROC proc, void* param) {
   DBOFFSET offset;
   unsigned short count;
   int proceed;
   enter();
   if (!hdb || !proc) error(DBERR_ACCESS);
   checkdata(hdb);
   if (!defIndex(hdb)->rootBlock) error(DBERR_NOT_FOUND);
   for (ReadIndexBlock(defIndex(hdb),(offset = defIndex(hdb)->rootBlock),&defIndex(hdb)->blockBuffer[0]);
       !defIndex(hdb)->blockBuffer[0].leafFlag;
       ReadIndexBlock(defIndex(hdb),(offset = defIndex(hdb)->blockBuffer[0].offset[0]),&defIndex(hdb)->blockBuffer[0]));
   for (; offset; ReadIndexBlock(defIndex(hdb),(offset = defIndex(hdb)->blockBuffer[0].adjacent[1]),&defIndex(hdb)->blockBuffer[0]))
      for (count = 0; count < defIndex(hdb)->blockBuffer[0].blockSize; ++count) {
         DBENTRY entry;
         freadp(&entry.entry.id,sizeof(entry.entry.id),defIndex(hdb)->blockBuffer[0].offset[count],SEEK_SET,dbcast(hdb)->stream);
         freadp(&entry.entry.data.size,sizeof(entry.entry.data.size),0,SEEK_CUR,dbcast(hdb)->stream);
         if ((entry.entry.data.data = allocfn(entry.entry.data.size)) == NULL) error(DBERR_MEMORY);
         freadp(entry.entry.data.data,entry.entry.data.size,0,SEEK_CUR,dbcast(hdb)->stream);
         proceed = proc(hdb,entry.entry.id,&entry.entry.data,param);
         freefn(entry.entry.data.data);
         if (proceed < 0) break;
         else if (!proceed) goto end;
      }
   end: error(DBERR_SUCCESS);
}

#ifdef DEBUG
void INTERNAL PrintNode(LPDBINDEX node, DBOFFSET offset, unsigned short dataSize) {
   unsigned short count;
   if (!node) { sd; fprintf(stderr,"(null node)\n"); return; }
   sd; fprintf(stderr,"Node %08lX: size = %i%s\n",offset,node->blockSize,node->leafFlag ? ", leaf" : "");
   if (!node->blockSize) return;
   for (count = 0; count < node->blockSize; ++count) { sd; fprintf(stderr,"Blk %02i: ",count); dump(node->data[count],dataSize); nl; }
   for (count = 0; count < node->blockSize+!node->leafFlag; ++count) { sd; fprintf(stderr,"Off %02i: %08lX\n",count,node->offset[count]); }
   if (node->leafFlag) { sd; fprintf(stderr,"Left: %08lX, Right: %08lX\n",node->adjacent[0],node->adjacent[1]); }
}
#endif

#define RMVIDX_SUCESSO 0
#define RMVIDX_FALHA 1
#define RMVIDX_EMRPESTA_GALHO 2
#define RMVIDX_EMPRESTA_SS 3

typedef short int RMVIDX;


RMVIDX RemoveIndiceAux ( HDBINDEX baseId, LPDBDATA index,  DBOFFSET off, void **menor, void *param) {
  int i; /* contador */
  int posicao;
  DBOFFSET offBusca;  

  ReadIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
  posicao = NodeSearch( baseId, &idxcast(baseId)->blockBuffer[0], 0, idxcast(baseId)->blockBuffer[0].blockSize - 1, index, param );

  if ( idxcast(baseId)->blockBuffer[0].leafNode ) {
    
    if( posicao < 0 ) return RMVIDX_FALHA;

    /* o elemento esta na folha; pode ser removido */

    idxcast(baseId)->blockBuffer[0].blockSize--;
    for( i=posicao; i<idxcast(baseId)->blockBuffer[0].blockSize; i++) {
      memcpy( idxcast(baseId)->blockBuffer[0].data[i], idxcast(baseId)->blockBuffer[0].data[i+1], idxcast(baseId)->dataSize );
      idxcast(baseId)->blockBuffer[0].offset[i] = idxcast(baseId)->blockBuffer[0].offset[i+1];
    }

    WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0] );

    if ( posicao == 0 ) memcpy ( *menor, idxcast(baseId)->blockBuffer[0].data[0], idxcast(baseId)->dataSize );      

    if ( idxcast(baseId)->blockBuffer[0].blockSize < idxcast(baseId)->blockSize/2 ) return RMVIDX_EMPRESTA_SS;
    
  }

    else { /* nao esta na folha */

    offBusca = idxcast(baseId)->blockBuffer[0].offset[posicao];

    /* remove nos filhos */
    switch( RemoveIndiceAux ( baseId, index, offBusca, menor, param ) ) {

            case RMVIDX_SUCESSO:

	      if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
		ReadIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
		memcpy( idxcast(baseId)->blockBuffer[0].data[posicao - 1], *menor, idxcast(baseId)->dataSize);
		WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
		free(*menor);
		*menor = NULL;
	      }	/* troca delimitadores */
	      break;


	    case RMVIDX_EMPRESTA_SS:

	      ReadIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0] );

	      if ( posicao > 0 ) {
           ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao-1], &idxcast(baseId)->blockBuffer[1] );
           ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[2] );
		   if ( idxcast(baseId)->blockBuffer[1].blockSize > idxcast(baseId)->blocksize/2 ) {/* empresta do irmao esquerdo */
		     for ( i=idxcast(baseId)->blockBuffer[2].blockSize++; i>0; i-- ) {
		       memcpy( idxcast(baseId)->blockBuffer[2].data[i], idxcast(baseId)->blockBuffer[2].data[i-1], idxcast(baseId)->dataSize );
		       idxcast(baseId)->blockBuffer[2].offset[i] = idxcast(baseId)->blockBuffer[2].offset[i-1];
		       /* desloca o bloco que ficou abaixo do piso para emprestar */
		     }		    
		     memcpy( idxcast(baseId)->blockBuffer[2].data[0], idxcast(baseId)->blockBuffer[1].data[--idxcast(baseId)->blockBuffer[1].blockSize],
			 idxcast(baseId)->dataSize );
		     memcpy( idxcast(baseId)->blockBuffer[0].data[posicao-1], idxcast(baseId)->blockBuffer[2].data[0], idxcast(baseId)->dataSize );
		     idxcast(baseId)->blockBuffer[2].offset[0] = idxcast(baseId)->blockBuffer[1].offset[idxcast(baseId)->blockBuffer[1].blockSize];
		     WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
		     WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao-1], &idxcast(baseId)->blockBuffer[1]);
		     WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[2]);
		     if( *menor != NULL ) {
		       free(*menor);
		       *menor = NULL;
		     }
		     return RMVIDX_SUCESSO;
		   }
	     }

	     if ( posicao < idxcast(baseId)->blockBuffer[0]->blockSize ) {
	       ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[1] );
               ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao+1], &idxcast(baseId)->blockBuffer[2] );
	       if ( idxcast(baseId)->blockBuffer[2].blockSize > idxcast(baseId)->blocksize/2 ) {/* empresta do irmao direito */
		 if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
		   memcpy( idxcast(baseId)->blockBuffer[0].data[posicao - 1], *menor, idxcast(baseId)->dataSize );
		   free(*menor);
		   *menor = NULL;
		 } /* troca o delimitador se necessario */
		 memcpy(&idxcast(baseId)->blockBuffer[1].data[idxcast(baseId)->blockBuffer[1].blockSize],idxcast(baseId)->blockBuffer[2].data[0],idxcast(baseId)->dataSize);
		 memcpy(&idxcast(baseId)->blockBuffer[0].data[posicao], &idxcast(baseId)->blockBuffer[2].data[1], idxcast(baseId)->dataSize);
		 idxcast(baseId)->blockBuffer[1].offset[idxcast(baseId)->blockBuffer[1].blockSize++] = idxcast(baseId)->blockBuffer[2].offset[0];
		 idxcast(baseId)->blockBuffer[2].blockSize--;
		 for ( i=0;i<idxcast(baseId)->blockBuffer[2].blockSize;i++) {
		   memcpy( idxcast(baseId)->blockBuffer[2].data[i], idxcast(baseId)->blockBuffer[2].data[i+1], idxcast(baseId)->dataSize);
		   idxcast(baseId)->blockBuffer[2].offset[i] = idxcast(baseId)->blockBuffer[2].offset[i+1];
		 } /* desloca o irmao que emprestou */
		 WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
		 WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[1]);
		 WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao+1], &idxcast(baseId)->blockBuffer[2]);
		 return RMVIDX_SUCESSO;
	       }
	     }

	     if ( posicao > 0 ) { /* junta com o irmao esquerdo */
	       	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao-1], &idxcast(baseId)->blockBuffer[1] );
                ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[2] );
		for( i=0; i < idxcast(baseId)->blockBuffer[2]->blockSize; i++ ) { /* junta os dois blocos */
		  memcpy(idxcast(baseId)->blockBuffer[1]->data[i + idxcast(baseId)->blockBuffer[1]->blockSize],idxcast(baseId)->blockBuffer[2]->data[i],idxcast(baseId)->dataSize);
		  idxcast(baseId)->blockBuffer[1]->offset[i + idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
		}
		idxcast(baseId)->blockBuffer[1]->blockSize += idxcast(baseId)->blockBuffer[2]->blockBuffer;
		idxcast(baseId)->blockBuffer[0]->blockSize--;

		for( i=posicao-1; i < idxcast(baseId)->blockBuffer[0]->blockSize; i++ ) { /* ajusta o pai */
		  idxcast(baseId)->blockBuffer[0]->data[i] = idxcast(baseId)->blockBuffer[0]->data[i+1];
		  idxcast(baseId)->blockBuffer[0]->offset[i+1] = idxcast(baseId)->blockBuffer[0]->offset[i+2];
		}

		if ( ( idxcast(baseId)->blockBuffer[1]->adjacent[1] = idxcast(baseId)->blockBuffer[2]->adjacent[1] ) != NULL ) {
		  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao - 1], &idxcast(baseId)->blockBuffer[1] );
		  ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[2]->adjacent[1], &idxcast(baseId)->blockBuffer[1] );
		  idxcast(baseId)->blockBuffer[1]->adjacent[0] = idxcast(baseId)->blockBuffer[2]->adjacent[0];
		  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[2]->adjacent[1], &idxcast(baseId)->blockBuffer[1] );
		} /* arruma as referencias do SS se preciso */

		else WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao - 1], &idxcast(baseId)->blockBuffer[1] );

		FreeIndexBlock( baseId, offBusca );

		if ( *menor != NULL ) {
		  free(*menor);
		  *menor = NULL;
		}

		if( idxcast(baseId)->blockBuffer[0]->blockSize < idxcast(baseId)->blockSize/2 )  return RMVIDX_EMPRESTA_GALHO;
	     }

	     else { /* junta com o irmao direito */
	       ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[1] );
               ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao+1], &idxcast(baseId)->blockBuffer[2] );

	       if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
		   memcpy( idxcast(baseId)->blockBuffer[0]->data[posicao - 1], *menor, idxcast(baseId)->dataSize );
		   free(*menor);
		   *menor = NULL;
	       }

	       for( i=0; i < idxcast(baseId)->blockBuffer[2]->blockSize; i++ ) { /* junta os dois blocos */
		 memcpy(idxcast(baseId)->blockBuffer[1].data[i+idxcast(baseId)->blockBuffer[1].blockSize],idxcast(baseId)->blockBuffer[2].data[i],idxcast(baseId)->dataSize);
		 idxcast(baseId)->blockBuffer[1].offset[i + idxcast(baseId)->blockBuffer[1].blockSize] = idxcast(baseId)->blockBuffer[2].offset[i];
	       }
	       idxcast(baseId)->blockBuffer[1].blockSize += idxcast(baseId)->blockBuffer[2].blockBuffer;
	       idxcast(baseId)->blockBuffer[0].blockSize--;

	       offBusca = idxcast(baseId)->blockBuffer[0].offset[posicao + 1]; /* offset do bloco a ser inserido na lista de blocos disponiveis */
	       
	       for( i=posicao+1; i < idxcast(baseId)->blockBuffer[0].blockSize; i++ ) { /* ajusta o pai */
		 memcpy(idxcast(baseId)->blockBuffer[0].data[i-1],idxcast(baseId)->blockBuffer[0].data[i],idxcast(baseId)->dataSize);
		 idxcast(baseId)->blockBuffer[0].offset[i] = idxcast(baseId)->blockBuffer[0].offset[i+1];
	       }

	       if ( ( idxcast(baseId)->blockBuffer[1].adjacent[1] = idxcast(baseId)->blockBuffer[2].adjacent[1] ) != NULL ) {
		 WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[1] );
		 ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[2].adjacent[1], &idxcast(baseId)->blockBuffer[1] );
		 idxcast(baseId)->blockBuffer[1].adjacent[0] = idxcast(baseId)->blockBuffer[2].adjacent[0];
		 WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[2].adjacent[1], &idxcast(baseId)->blockBuffer[1]);	       
	       } /* arruma as referencias no SS se necessario */

	       else WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[1]) ;
	       
	       FreeIndexBlock( baseId, offBusca );

	       if ( idxcast(baseId)->blockBuffer[0].blockSize < idxcast(baseId)->blockSize/2 ) return RMVIDX_EMPRESTA_GALHO;
	     }	     
	     break;

    case RMVIDX_EMPRESTA_GALHO:
      ReadIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0] );

     
      
      if ( posicao > 0 ) {
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao-1], &idxcast(baseId).blockBuffer[1] );
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId).blockBuffer[2] );
	if ( idxcast(baseId)->blockBuffer[1].blockSize > idxcast(baseId)->blockSize/2 ) { /* pode emprestar do irmao */
	  for ( i=idxcast(baseId)->blockBuffer[2].blockSize++; i>0; i--) {
	    memcpy(idxcast(baseId)->blockBuffer[2].data[i], idxcast(baseId)->blockBuffer[2].data[i-1], idxcast(baseId)->dataSize);
	    idxcast(baseId)->blockBuffer[2].offset[i+1] = idxcast(baseId)->blockBuffer[2].offset[i];
	  } /*desloca para inserir */
	  idxcast(baseId)->blockBuffer[2].offset[1] = idxcast(baseId)->blockBuffer[2].offset[0];
	  idxcast(baseId)->blockBuffer[2].offset[0] = idxcast(baseId)->blockBuffer[1].offset[idxcast(baseId)->blockBuffer[1].blockSize];
	  memcpy(idxcast(baseId)->blockBuffer[2].data[0], idxcast(baseId)->blockBuffer[0].data[posicao-1], idxcast(baseId)->dataSize);
	  memcpy(idxcast(baseId)->blockBuffer[0].data[posicao-1],idxcast(baseId)->blockBuffer[1].data[idxcast(baseId)->blockBuffer[1].blockSize--],idxcast(baseId)->dataSize);
	  WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
	  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao-1], &idxcast(baseId)->blockBuffer[1]);
	  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[2]);
	  if ( *menor != NULL ) {
	    free(*menor);
	    *menor = NULL;
	  }
	}
      }
      
      if ( posicao < idxcast(baseId)->blockBuffer[0]->blockSize ) {
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId).blockBuffer[1] );
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao+1], &idxcast(baseId).blockBuffer[2] );
	
	if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
	  memcpy(idxcast(baseId)->blockBuffer[0].data[posicao - 1], *menor, idxcast(baseId)->dataSize);
	  free(*menor);
	  *menor = NULL;
	}

	if ( idxcast(baseId)->blockBuffer[2].blockSize > idxcast(baseId)->blockSize/2 ) {
	  idxcast(baseId)->blockBuffer[1].offset[idxcast(baseId)->blockBuffer[1].blockSize+1] = idxcast(baseId)->blockBuffer[2].offset[0];
	  memcpy(idxcast(baseId)->blockBuffer[1].data[idxcast(baseId)->blockBuffer[1].blockSize-1], idxcast(baseId)->blockBuffer[0].data[posicao], idxcast(baseId)->dataSize);
	  memcpy(idxcast(baseId)->blockBuffer[0].data[posicao], idxcast(baseId)->blockBuffer[2].data[0], idxcast(baseId)->dataSize);
	  for( i=0;i<idxcast(baseId)->blockBuffer[2].blockSize-1;i++) {
	    memcpy(idxcast(baseId)->blockBuffer[2].data[i] = idxcast(baseId)->blockBuffer[2].data[i+1]);
	    idxcast(baseId)->blockBuffer[2].offset[i] = idxcast(baseId)->blockBuffer[2].data[i+1];
	  }
	  idxcast(baseId)->blockBuffer[2].data[idxcast(baseId)->blockBuffer[2].blockSize-1] = idxcast(baseId)->blockBuffer[2].offset[idxcast(baseId)->blockBuffer[2].blockSize];
	  idxcast(baseId)->blockBuffer[2].blockSize--;
	  idxcast(baseId)->blockBuffer[1].blockSize++;
	  WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
	  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[1]);
	  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao+1], &idxcast(baseId)->blockBuffer[2]);
        }
     }

      if ( posicao > 0 ) {
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao-1], &idxcast(baseId).blockBuffer[1] );
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId).blockBuffer[2] );
	memcpy(idxcast(baseId)->blockBuffer[1]->data[idxcast(baseId)->blockBuffer[1]->blockSize],idxcast(baseId)->blockBuffer[0]->data[posicao-1],idxcast(baseId)->dataSize);
	for( i=0; i < idxcast(baseId)->blockBuffer[2]->blockSize; i++ ) { /* junta os dois blocos */
	  memcpy(idxcast(baseId)->blockBuffer[1]->data[i+idxcast(baseId)->blockBuffer[1]->blockSize+1],idxcast(baseId)->blockBuffer[2]->data[i],idxcast(baseId)->dataSize);
	  idxcast(baseId)->blockBuffer[1]->offset[i+idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
       	}
	idxcast(baseId)->blockBuffer[1]->offset[i+idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
	idxcast(baseId)->blockBuffer[1]->blockSize += idxcast(baseId)->blockBuffer[2]->blockSize + 1;
	for( i=posicao-1; i < idxcast(baseId)->blockBuffer[0]->blockSize-1; i++) {
	  memcpy(idxcast(baseId)->blockBuffer[0]->data[i],idxcast(baseId)->blockBuffer[0]->data[i+1],idxcast(baseId)->dataSize);
	  idxcast(baseId)->blockBuffer[0]->offset[i+1] = idxcast(baseId)->blockBuffer[0]->offset[i+2];
	}
	idxcast(baseId)->blockBuffer[0].blockSize--;
	WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
	WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao-1], &idxcast(baseId)->blockBuffer[1]);
	FreeIndexBlock( baseId, offBusca );
	if ( *menor != NULL ) {
	    free(*menor);
	    *menor = NULL;
        }
	if (idxcast(baseId)->blockBuffer[0].blockSize < idxcast(baseId)->blockSize/2 ) return RMVIDX_EMPRESTA_GALHO;
	}

      else {
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId).blockBuffer[1] );
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao+1], &idxcast(baseId).blockBuffer[2] );

	if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
	  memcpy(idxcast(baseId)->blockBuffer[0].data[posicao - 1], *menor, idxcast(baseId)->dataSize);
	  free(*menor);
	  *menor = NULL;
	}
	offBusca = idxcast(baseId)->blockBuffer[0]->offset[posicao+1];
	memcpy(idxcast(baseId)->blockBuffer[1]->data[idxcast(baseId)->blockBuffer[1]->blockSize],idxcast(baseId)->blockBuffer[0]->data[posicao],idxcast(baseId)->dataSize);
	for( i=0; i < idxcast(baseId)->blockBuffer[2]->blockSize; i++ ) { /* junta os dois blocos */
	  memcpy(idxcast(baseId)->blockBuffer[1]->data[i+idxcast(baseId)->blockBuffer[1]->blockSize+1],idxcast(baseId)->blockBuffer[2]->data[i],idxcast(baseId)->dataSize);
	  idxcast(baseId)->blockBuffer[1]->offset[i+idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
       	}
	idxcast(baseId)->blockBuffer[1]->offset[i+idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
	idxcast(baseId)->blockBuffer[1]->blockSize += idxcast(baseId)->blockBuffer[2]->blockSize + 1;
	for( i=posicao; i < idxcast(baseId)->blockBuffer[0]->blockSize-1; i++) {
	  memcpy(idxcast(baseId)->blockBuffer[0]->data[i],idxcast(baseId)->blockBuffer[0]->data[i+1],idxcast(baseId)->dataSize);
	  idxcast(baseId)->blockBuffer[0]->offset[i+1] = idxcast(baseId)->blockBuffer[0]->offset[i+2];
	}
	idxcast(baseId)->blockBuffer[0].blockSize--;
	WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
	WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[1]);
	FreeIndexBlock( baseId, offBusca );
	if (idxcast(baseId)->blockBuffer[0].blockSize < idxcast(baseId)->blockSize/2 ) return RMVIDX_EMPRESTA_GALHO;
	}
      }
    }

  return RMVIDX_SUCESSO;

  }

  DBERR RemoveIndice ( HDBINDEX baseId, LPDBDATA index, void* param) {
    DBOFFSET temp;
    void *menor = malloc(idxcast(baseId)->dataSize);
    switch ( RemoveIndice ( baseId, index, idxcast(baseId)->rootBlock, &menor, param )) {
  case RMVIDX_SUCESSO: 
    if ( menor != NULL ) {
      free(menor);
      menor = NULL;
    }
    return DBERR_SUCESS;
    break;
  case RMVIDX_FALHA:
    free(menor);
    menor = NULL;    
    return DBERR_ACCESS;
    break;
  case RMVIDX_EMPRESTA_GALHO:
    if ( menor != NULL ) {
      free(menor);
      menor = NULL;
    }
    ReadIndexBlock(baseId, idxcast(baseId)->rootBlock, &idxcast(baseId)->blockBuffer[0]); /* raiz ficou sem elementos, troca por seu filho */
    if( idxcast(baseId)->blockBuffer[0].blockSize == 0 ) {
      temp = idxcast(baseId)->blockBuffer[0].offset[0];
      FreeIndexBlock( baseId, idxcast(baseId)->rootBlock );
      /* poe endereco do no na lista de leaks */
      idxcast(baseId)->rootBlock = temp;
    }
    return DBERR_SUCCESS;
    break;
  case RMVIDX_EMPRESTA_SS:
    if ( menor != NULL ) {
      free(menor);
      menor = NULL;
    }
    ReadIndexBlock(baseId, idxcast(baseId)->rootBlock, &idxcast(baseId)->blockBuffer[0]);
    if( idxcast(baseId)->blockBuffer[0]->blockSize == 0 ){
      FreeIndexBlock( baseId, offBusca );
      idxcast(baseId)->rootBlock = NULL;
    }
    return DBERR_SUCCESS;
    break;	
  }
    return DBERR_SUCCESS; 
}
