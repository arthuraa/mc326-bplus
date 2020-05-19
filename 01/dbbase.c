/* DBBASE.C: Implementacao das funcoes do banco de dados.   */
/* Compilado usando mingw (g++ (GCC) 3.4.2 (mingw-special)) */
/* Aleksey V T Covacevice RA030845 */
/* Arthur Amorim          RA031339 */

#include <stdlib.h>
#include <stdio.h>
#include "dbbase.h"

#define dbcast(x) ((_LPDB)(x))
#define valid_type(x) (x == DB_TYPE_FIX || x == DB_TYPE_VAR)

typedef struct {
   unsigned long signature;
   unsigned long version;
   unsigned char type;
} DBHEADER, *LPDBHEADER;

typedef struct {
   FILE* dbData;
   FILE* dbIndex;
   DBHEADER dbHeaderData;
   DBHEADER dbHeaderIndex;
} _DB, *_LPDB;

unsigned long DBSignature(HDB hdb) {
   if (!hdb) return 0xFFFFFFFF;
   return dbcast(hdb)->dbHeaderData.signature;
}

unsigned long DBVersion(HDB hdb) {
   if (!hdb) return 0xFFFFFFFF;
   return dbcast(hdb)->dbHeaderData.version;
}

unsigned char DBType(HDB hdb) {
   if (!hdb) return 0xFF;
   return dbcast(hdb)->dbHeaderData.type;
}

/* Implementation specific start */
DBSIZE EntryFixSize(HDB hdb) {
   if (!hdb) return 0;
   if (DBType(hdb) == DB_TYPE_FIX) return sizeof(DBWORD)+sizeof(DBFIX)*DBENTRY_FIXPARTLENGTH+sizeof(DBFIX)*DBENTRY_FIXPARTLENGTH;
   else return sizeof(DBWORD)+2;
}

DBSIZE EntryVarSize(LPDBENTRY entry, HDB hdb) {
   if (!entry || !hdb) return 0;
   if (DBType(hdb) == DB_TYPE_FIX) return 0;
   else return strlen(entry->data.var.name)+strlen(entry->data.var.address);
} /* Implementation specific end */

size_t fwritep(const void* ptr, size_t size, long off, int whence, FILE* stream) { /* fwrite para a posicao dada. */
   if (fseek(stream,off,whence)) return 0;
   return fwrite(ptr,size,1,stream);
}

size_t freadp(void* ptr, size_t size, long off, int whence, FILE* stream) { /* fread para a posicao dada. */
   if (fseek(stream,off,whence)) return 0;
   return fread(ptr,size,1,stream);
}

void finsertp(const void* ptr, size_t size, long off, FILE* stream) { /* insere dados no stream, deslocando (se */
   long count, len;                                                   /* possivel) o buffer de fluxo corrente.  */
   int buffer;
   if (!ptr || !size || !stream) return;
   if (off > (len = filelength(fileno(stream)))) chsize(fileno(stream),(len = off));
   chsize(fileno(stream),(len += size));
   for (count = len-size-1; count >= off; --count) {
      fseek(stream,count,SEEK_SET);
      buffer = fgetc(stream);
      fseek(stream,count+size,SEEK_SET);
      fputc(buffer,stream);
   }
   fwritep(ptr,size,off,SEEK_SET,stream);
}

void fremovep(size_t size, long off, FILE* stream) { /* remove dados do stream deslocando o buffer de fluxo. */
   long count, len;
   int buffer;
   if (!stream || !size) return;
   if (off > (len = filelength(fileno(stream)))) return;
   if (off+size > len) size = len-off;
   for (count = off+size;;++count) {
      fseek(stream,count,SEEK_SET);
      if ((buffer = fgetc(stream)) == EOF) break;
      fseek(stream,count-size,SEEK_SET);
      fputc(buffer,stream);
   }
   chsize(fileno(stream),len-size);
}

DBERR CreateEmptyDB(const char* filename, const char* indexFile, unsigned char type, HDB* ptr) {
   _LPDB tmp;
   if (!filename || (type == DB_TYPE_VAR && !indexFile) || !ptr) return DBERR_ACCESS;
   if (!valid_type(type)) return DBERR_ARGUMENT;
   if ((tmp = (_LPDB)malloc(sizeof(_DB))) == NULL) return DBERR_MEMORY;
   if ((dbcast((*ptr = (HDB*)tmp))->dbData = fopen(filename,"w+b")) == NULL) {
      err: free(*ptr);
      return DBERR_IO;
   }
   if (type == DB_TYPE_VAR) if ((dbcast(*ptr)->dbIndex = fopen(indexFile,"w+b")) == NULL) {
      fclose(dbcast(*ptr)->dbData);
      goto err;
   }
   dbcast(*ptr)->dbHeaderData.signature = DB_MAGIC_SIGNATURE;
   dbcast(*ptr)->dbHeaderData.version = DB_VERSION;
   dbcast(*ptr)->dbHeaderData.type = type;
   fwrite(&dbcast(*ptr)->dbHeaderData,sizeof(DBHEADER),1,dbcast(*ptr)->dbData);
   if (type == DB_TYPE_VAR) {
      dbcast(*ptr)->dbHeaderIndex.signature = DB_MAGIC_SIGNATURE;
      dbcast(*ptr)->dbHeaderIndex.version = DB_VERSION;
      dbcast(*ptr)->dbHeaderIndex.type = DB_TYPE_IDX;
      fwrite(&dbcast(*ptr)->dbHeaderIndex,sizeof(DBHEADER),1,dbcast(*ptr)->dbIndex);
   }
   return DBERR_SUCCESS;
}

DBERR InitializeDB(const char* filename, const char* indexFile, HDB* ptr) {
   _LPDB tmp;
   if (!filename || !ptr) return DBERR_ACCESS;
   if ((tmp = (_LPDB)malloc(sizeof(_DB))) == NULL) return DBERR_MEMORY;
   if ((dbcast((*ptr = (HDB*)tmp))->dbData = fopen(filename,"r+b")) == NULL) {
      err: free(*ptr);
      return DBERR_IO;
   }
   if (!fread(&dbcast(*ptr)->dbHeaderData,sizeof(DBHEADER),1,dbcast(*ptr)->dbData)) {
      err2: fclose(dbcast(*ptr)->dbData);
      goto err;
   }
   if (dbcast(*ptr)->dbHeaderData.signature != DB_MAGIC_SIGNATURE ||
       dbcast(*ptr)->dbHeaderData.version != DB_VERSION ||
       !valid_type(dbcast(*ptr)->dbHeaderData.type)) {
      err3: fclose(dbcast(*ptr)->dbData);
      free(*ptr);
      return DBERR_FORMAT;
   }
   if (dbcast(*ptr)->dbHeaderData.type == DB_TYPE_VAR) {
      if ((dbcast(*ptr)->dbIndex = fopen(indexFile,"r+b")) == NULL) goto err2;
      if (!fread(&dbcast(*ptr)->dbHeaderIndex,sizeof(DBHEADER),1,dbcast(*ptr)->dbIndex)) {
         fclose(dbcast(*ptr)->dbIndex);
         goto err2;
      }
      if (dbcast(*ptr)->dbHeaderIndex.signature != DB_MAGIC_SIGNATURE ||
          dbcast(*ptr)->dbHeaderIndex.version != DB_VERSION ||
          dbcast(*ptr)->dbHeaderIndex.type != DB_TYPE_IDX) {
         fclose(dbcast(*ptr)->dbIndex);
         goto err3;
      }
   }
   return DBERR_SUCCESS;
}

DBERR CloseDB(HDB hdb) {
   fclose(dbcast(hdb)->dbData);
   if (DBType(hdb) == DB_TYPE_VAR) fclose(dbcast(hdb)->dbIndex);
   free(dbcast(hdb));
   return DBERR_SUCCESS;
}

DBERR ReadEntryOffsetDB(DBOFFSET offset, LPDBENTRY entry, LPDBENTRYHEADER header, HDB hdb) {
   DBENTRYHEADER localHeader;
   if (!hdb) return DBERR_ACCESS;
   if (DBType(hdb) == DB_TYPE_VAR) {
      freadp(&localHeader,sizeof(localHeader),offset,SEEK_SET,dbcast(hdb)->dbData);
      if (header) memcpy(header,&localHeader,sizeof(localHeader));
   }
   if (entry) { /* Implementation specific start */
      if (DBType(hdb) == DB_TYPE_VAR) {
         DBWORD age;
         char* buffer;
         int len;
         if ((buffer = (char*)malloc(localHeader.varPartSize+2)) == NULL) return DBERR_MEMORY;
         freadp(&age,sizeof(age),offset+sizeof(DBENTRYHEADER),SEEK_SET,dbcast(hdb)->dbData);
         fread(buffer,localHeader.varPartSize+2,1,dbcast(hdb)->dbData);
         if ((entry->data.var.name = (DBVAR)malloc((len = strlen(buffer))+1)) == NULL) {
            err: free(buffer);
            return DBERR_MEMORY;
         }
         strcpy(entry->data.var.name,buffer);
         if ((entry->data.var.address = (DBVAR)malloc(strlen((buffer += len+1))+1)) == NULL) {
            free(entry->data.var.name);
            goto err;
         }
         strcpy(entry->data.var.address,buffer);
         entry->age = age;
         free(buffer);
      }
      else {
         freadp(&entry->index.id,sizeof(entry->index.id),offset,SEEK_SET,dbcast(hdb)->dbData);
         fread(&entry->age,sizeof(entry->age),1,dbcast(hdb)->dbData);
         fread(&entry->data.fix,sizeof(entry->data.fix),1,dbcast(hdb)->dbData);
      }
      entry->index.offset = offset;
   } /* Implementation specific end */
   return DBERR_SUCCESS;
}

DBERR _WriteEntryOffsetDB(DBOFFSET offset, LPDBENTRY entry, HDB hdb) { /* Escreve o registro para o offset dado. */
   DBENTRYHEADER header;
   if (DBType(hdb) == DB_TYPE_VAR) {
      header.varPartSize = EntryVarSize(entry,hdb);
      fwritep(&header,sizeof(header),offset,SEEK_SET,dbcast(hdb)->dbData);
   }
   /* Implementation specific start */ {
      if (DBType(hdb) == DB_TYPE_VAR) {
         fwritep(&entry->age,sizeof(entry->age),offset+sizeof(DBENTRYHEADER),SEEK_SET,dbcast(hdb)->dbData);
         fwrite(entry->data.var.name,strlen(entry->data.var.name)+1,1,dbcast(hdb)->dbData);
         fwrite(entry->data.var.address,strlen(entry->data.var.address)+1,1,dbcast(hdb)->dbData);
      }
      else {
         fwritep(&entry->index.id,sizeof(entry->index.id),offset,SEEK_SET,dbcast(hdb)->dbData);
         fwrite(&entry->age,sizeof(entry->age),1,dbcast(hdb)->dbData);
         fwrite(&entry->data.fix,sizeof(entry->data.fix),1,dbcast(hdb)->dbData);
      }
   } /* Implementation specific end */
   return DBERR_SUCCESS;
}

DBERR _ScanEntryIDDB(DBID id, LPDBENTRY entry, HDB hdb, int searchLeak, DBSIZE leakSize, LPDBINDEX _index, LPDBENTRYHEADER _header, LPDBOFFSET idxoff) {
   DBINDEX index;
   DBENTRYHEADER header;
   int flag, count;
   if (DBType(hdb) == DB_TYPE_VAR) {
      fseek(dbcast(hdb)->dbIndex,sizeof(DBHEADER),SEEK_SET);
      for (flag = 0; fread(&index,sizeof(index),1,dbcast(hdb)->dbIndex);) {
         freadp(&header,sizeof(header),index.offset,SEEK_SET,dbcast(hdb)->dbData);
         if (index.id == id) {
            flag = 1;
            break;
         }
         else if (index.id == DBENTRY_RESERVEDID_LEAK) {
            if (!searchLeak) break;
            if (header.leakSize == leakSize || header.leakSize > leakSize+sizeof(DBENTRYHEADER)) {
               flag = 2;
               break;
            }
         }
      }
   }
   else for (flag = count = 0; freadp(&index.id,sizeof(index.id),sizeof(DBHEADER)+count*(sizeof(DBID)+EntryFixSize(hdb)),SEEK_SET,dbcast(hdb)->dbData); ++count)
      if (index.id == id) {
         flag = 1;
         index.offset = ftell(dbcast(hdb)->dbData)-sizeof(DBID);
         break;
      }
      else if (index.id == DBENTRY_RESERVEDID_LEAK) {
         flag = 2;
         index.offset = ftell(dbcast(hdb)->dbData)-sizeof(DBID);
      }
   if (!flag) return DBERR_NOT_FOUND;
   if (DBType(hdb) == DB_TYPE_VAR) {
      if (_header) memcpy(_header,&header,sizeof(header));
      if (idxoff) *idxoff = ftell(dbcast(hdb)->dbIndex)-sizeof(index);
   }
   if (_index) memcpy(_index,&index,sizeof(index));   
   if (entry) memcpy(&entry->index,&index,sizeof(index));
   if (flag == 1) ReadEntryOffsetDB(index.offset,entry,NULL,hdb);
   return flag == 1 ? DBERR_SUCCESS : DBERR_LEAK_FOUND;
}

DBERR ScanEntryIDDB(DBID id, LPDBENTRY entry, HDB hdb) {
   if (!hdb) return DBERR_ACCESS;
   if (id == DBENTRY_RESERVEDID_LEAK) return DBERR_ARGUMENT;
   return _ScanEntryIDDB(id,entry,hdb,0,0,NULL,NULL,NULL);
}

DBERR TraverseDB(int (*func)(DBID, DBOFFSET, HDB, void*), int showLeaks, HDB hdb, void* param) {
   DBINDEX index;
   int count;
   if (!hdb || !func) return DBERR_ACCESS;
   if (DBType(hdb) == DB_TYPE_VAR) {
      fseek(dbcast(hdb)->dbIndex,sizeof(DBHEADER),SEEK_SET);
      while (fread(&index,sizeof(index),1,dbcast(hdb)->dbIndex)) {
         if (index.id == DBENTRY_RESERVEDID_LEAK && !showLeaks) break;
         if (!func(index.id,index.offset,hdb,param)) break;
      }
   }
   else for (count = 0; freadp(&index.id,sizeof(index.id),sizeof(DBHEADER)+count*(sizeof(DBID)+EntryFixSize(hdb)),SEEK_SET,dbcast(hdb)->dbData); ++count) {
      if (index.id == DBENTRY_RESERVEDID_LEAK && !showLeaks) continue;
      if (!func(index.id,ftell(dbcast(hdb)->dbData)-sizeof(DBID),hdb,param)) break;
   }
   return DBERR_SUCCESS;
}

DBERR InsertEntryDB(LPDBENTRY entry, HDB hdb) {
   DBENTRYHEADER header;
   DBOFFSET idxoff;
   DBINDEX index;
   DBSIZE fix, var;
   if (!entry || !hdb) return DBERR_ACCESS;
   if (entry->index.id == DBENTRY_RESERVEDID_LEAK) return DBERR_ARGUMENT;
   switch (_ScanEntryIDDB(entry->index.id,NULL,hdb,1,(fix = EntryFixSize(hdb))+(var = EntryVarSize(entry,hdb)),&index,&header,&idxoff)) {
      case DBERR_NOT_FOUND:
      if (DBType(hdb) == DB_TYPE_VAR) {
         fseek(dbcast(hdb)->dbData,0,SEEK_END);
         entry->index.offset = ftell(dbcast(hdb)->dbData);
         we: _WriteEntryOffsetDB(entry->index.offset,entry,hdb);
         finsertp(&entry->index,sizeof(entry->index),sizeof(DBHEADER),dbcast(hdb)->dbIndex);
      }
      else {
         index.offset = filelength(fileno(dbcast(hdb)->dbData));
         we2: _WriteEntryOffsetDB(index.offset,entry,hdb);
      }
      break;
      case DBERR_LEAK_FOUND:
      if (DBType(hdb) == DB_TYPE_VAR) {
         entry->index.offset = index.offset;
         index.offset += fix+var+sizeof(DBENTRYHEADER);
         if ((header.leakSize = header.varPartSize-(fix+var)) > 0) {
            fwritep(&index,sizeof(index),idxoff,SEEK_SET,dbcast(hdb)->dbIndex);
            fwritep(&header,sizeof(header),index.offset,SEEK_SET,dbcast(hdb)->dbData);
         }
         else fremovep(sizeof(DBINDEX),idxoff,dbcast(hdb)->dbIndex); /* No zero-sized leaks */
         goto we;
      }
      else goto we2;
      case DBERR_SUCCESS:
      default: return DBERR_ALREADY_EXISTS;
   }
   return DBERR_SUCCESS;
}

DBERR RemoveEntryDB(DBID id, HDB hdb) {
   DBENTRYHEADER header;
   DBOFFSET idxoff;
   DBINDEX index;
   if (!hdb) return DBERR_ACCESS;
   if (id == DBENTRY_RESERVEDID_LEAK) return DBERR_ARGUMENT;
   if (_ScanEntryIDDB(id,NULL,hdb,0,0,&index,&header,&idxoff) == DBERR_NOT_FOUND) return DBERR_NOT_FOUND;
   if (DBType(hdb) == DB_TYPE_VAR) {
      header.leakSize = header.varPartSize+EntryFixSize(hdb);
      fwritep(&header,sizeof(header),index.offset,SEEK_SET,dbcast(hdb)->dbData);
      fremovep(sizeof(DBINDEX),idxoff,dbcast(hdb)->dbIndex);
      index.id = DBENTRY_RESERVEDID_LEAK;
      fwritep(&index,sizeof(index),0,SEEK_END,dbcast(hdb)->dbIndex);
   }
   else {
      id = DBENTRY_RESERVEDID_LEAK;
      fwritep(&id,sizeof(id),index.offset,SEEK_SET,dbcast(hdb)->dbData);
   }
   return DBERR_SUCCESS;
}

int RebuildDBFunc(DBID id, DBOFFSET offset, HDB hdb, void* param) {
   DBENTRY entry;
   ReadEntryOffsetDB(offset,&entry,NULL,hdb);
   entry.index.id = id;
   InsertEntryDB(&entry,(HDB)param);
   return 1;
}

DBERR RebuildDB(const char* filename, const char* indexFile, HDB* dst, HDB hdb) {
   DBERR error;
   if ((error = CreateEmptyDB(filename,indexFile,DBType(hdb),dst)) != DBERR_SUCCESS) return error;
   if ((error = TraverseDB(RebuildDBFunc,0,hdb,(void*)*dst)) != DBERR_SUCCESS) return error;
   return DBERR_SUCCESS;
}
