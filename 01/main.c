/* MAIN.C: Programa de uso das rotinas do banco de dados.   */
/* Compilado usando mingw (g++ (GCC) 3.4.2 (mingw-special)) */
/* Aleksey V T Covacevice RA030845 */
/* Arthur Amorim          RA031339 */

#include <stdlib.h>
#include <stdio.h>
#include "dbbase.h"
#include "ptime.h"

#define SWP_DB  "tmp.dat"
#define SWP_IDX "tmp.idx"

HDB hdb;

char* DBError(DBERR code) {
   static char buffer[0x40], *ptr;
   switch (code) {
      case DBERR_ACCESS: ptr = "ponteiro invalido"; break;
      case DBERR_MEMORY: ptr = "sem memoria"; break;
      case DBERR_IO: ptr = "I/O"; break;
      case DBERR_ARGUMENT: ptr = "argumento invalido"; break;
      case DBERR_FORMAT: ptr = "formato do arquivo/entrada invalido"; break;
      case DBERR_NOT_FOUND: ptr = "registro nao encontrado"; break;
      case DBERR_ALREADY_EXISTS: ptr = "registro existente"; break;
      case DBERR_LEAK_FOUND: ptr = "leak encontrado"; break;
      case DBERR_SUCCESS: default: ptr = "sem erro"; break;
   }
   strcpy(buffer,ptr);
   return buffer;
}

void dbassert(DBERR err) {
   if (err == DBERR_SUCCESS) return;
   fprintf(stderr,"Erro %08lX: %s.\r\n",err,DBError(err));
   if (hdb) CloseDB(hdb);
   exit(1);
}

void printentry(LPDBENTRY entry) {
   if (!entry) return;
   printf("Registro %08lX $%08lX\r\nNome: %s\r\nIdade: %i\r\nEndereco: %s\r\n",entry->index.id,entry->index.offset,DBType(hdb) == DB_TYPE_VAR ? entry->data.var.name : entry->data.fix.name,entry->age,DBType(hdb) == DB_TYPE_VAR ? entry->data.var.address : entry->data.fix.address);
}

int printdb(DBID id, DBOFFSET offset, HDB hdb, void* param) {
   DBENTRYHEADER header;
   DBENTRY entry;
   if (id == DBENTRY_RESERVEDID_LEAK) {
      if (DBType(hdb) == DB_TYPE_VAR) dbassert(ReadEntryOffsetDB(offset,NULL,&header,hdb));
      else header.leakSize = EntryFixSize(hdb);
      printf("Leak %08lX $%08lX, tamanho: %li bytes\r\n---\r\n",id,offset,header.leakSize);
   }
   else {
      dbassert(ReadEntryOffsetDB(offset,&entry,NULL,hdb));
      entry.index.id = id; entry.index.offset = offset;
      printentry(&entry);
      printf("---\r\n");
   }
   return 1;
}

int main(int argc, char** argv) {
   DBENTRY entry;
   char* aux;
   if (argc < 4) {
      printf("Uso: %s <cmd> <dat> <idx> [args]\r\n\r\n"
             "Comandos:\r\n"
             "v: cria uma nova base de dados de registros variaveis.\r\n"
             "f: cria uma nova base de dados de registros fixos. O argumento <idx> e ignorado.\r\n"
             "i <id> <age> <name> <addr>: insere um registro na base de dados.\r\n"
             "r <id>: remove um registro na base de dados.\r\n"
             "s <id>: procura e recupera as informacoes de um registro.\r\n"
             "e: exibe todos os registros armazenados, assim como os fragmentos (leaks) na base de dados.\r\n"
             "m: reconstroi a base de dados, removendo a fragmentacao. Usa um swap.\r\n"            
             ,argv[0]);
      return 0;
   }
   hdb = NULL;
   ptime();
   switch (argv[1][0] | 0x40) {
      case 'f': dbassert(CreateEmptyDB(argv[2],argv[3],DB_TYPE_FIX,&hdb)); break;      
      case 'v': dbassert(CreateEmptyDB(argv[2],argv[3],DB_TYPE_VAR,&hdb)); break;
      default:
      dbassert(InitializeDB(argv[2],argv[3],&hdb));
      switch (argv[1][0] | 0x40) {
         case 'i':
         if (argc < 8) goto argerr;
         entry.index.id = (DBID)strtoul(argv[4],&aux,0x10); if (*aux != 0) goto argerr;
         entry.age = (DBWORD)strtoul(argv[5],&aux,10); if (*aux != 0) goto argerr;
         if (DBType(hdb) == DB_TYPE_VAR) {
            entry.data.var.name = argv[6];
            entry.data.var.address = argv[7];
         }
         else {
            strncpy(entry.data.fix.name,argv[6],sizeof(entry.data.fix.name)-1);
            strncpy(entry.data.fix.address,argv[7],sizeof(entry.data.fix.address)-1);
         }
         dbassert(InsertEntryDB(&entry,hdb));
         printentry(&entry);
         break;
         case 'r':
         if (argc < 5) goto argerr;
         entry.index.id = (DBID)strtoul(argv[4],&aux,0x10); if (*aux != 0) goto argerr;
         dbassert(RemoveEntryDB(entry.index.id,hdb));
         break;
         case 's':
         if (argc < 5) goto argerr;
         entry.index.id = (DBID)strtoul(argv[4],&aux,0x10); if (*aux != 0) goto argerr;
         dbassert(ScanEntryIDDB(entry.index.id,&entry,hdb));
         printentry(&entry);
         if (DBType(hdb) == DB_TYPE_VAR) { free(entry.data.var.name); free(entry.data.var.address); }
         break;
         case 'e':
         dbassert(TraverseDB(printdb,1,hdb,NULL));
         break;
         case 'm': {
            HDB tmp;
            dbassert(RebuildDB(SWP_DB,SWP_IDX,&tmp,hdb));
            CloseDB(tmp);
            CloseDB(hdb);
            unlink(argv[2]); unlink(argv[3]);
            rename(SWP_DB,argv[2]); rename(SWP_IDX,argv[3]);
            hdb = NULL;
         }            
         break;
         default: argerr: fprintf(stderr,"Argumento invalido ou insuficiente.\r\n"); break;
      }
      break;
   }
   if (hdb) CloseDB(hdb);
   printf("Tempo de processo: %fs\r\n",ptime());
   return 0;
}
