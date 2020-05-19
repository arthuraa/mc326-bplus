#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbbase.h>
#include <debug.h>

static int allocCount = 0;

void* _malloc(unsigned long size) { ++allocCount; return malloc((size_t)size); }
void* _realloc(void* ptr, unsigned long size) { if (!ptr) return _malloc(size); else return realloc(ptr,(size_t)size); }
void  _free(void* ptr) { --allocCount; free(ptr); }
unsigned long randomID(void) { return 1; }

int traverse(HDB hdb, DBID id, const LPDBDATA data, void* param) {
   var(id,"%i\n");
   var(data->size,"%i\n");
   var(data->data,"%s\n");
   out("-----\n");
   return 1;
}

int main(int argc, char** argv) {
   HDB hdb = NULL;
   DBDATA data = {17,"Test Database0000"};
   InitializeDBCore(_malloc,_realloc,_free,randomID);
   GenerateDB("Test.db",&data,3,&hdb);
   while (1) {
      char b[0x100], x[0x100];
      fgets(b,sizeof(b),stdin);
      if (!strcmp(b,"quit\n")) break;
      fgets(x,sizeof(x),stdin);
      data.data = b;
      data.size = strlen(b);
      if (InsertDataDB(hdb,atoi(x),&data,NULL) != DBERR_SUCCESS) printf("cannot insert !\n");
   }
   TraverseDB(hdb,traverse,NULL);
   stop();
   if (hdb) CloseDB(&hdb);
   ReleaseDBCore();
   mark("process end, "); var(allocCount,"%i\n");
   return 0;
}

