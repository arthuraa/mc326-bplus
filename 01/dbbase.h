/* DBBASE.H: Prototipos das funcoes e estruturas do banco de dados. */
/* Compilado usando mingw (g++ (GCC) 3.4.2 (mingw-special))         */
/* Aleksey V T Covacevice RA030845 */
/* Arthur Amorim          RA031339 */

#ifndef __DBBASE_H
#define __DBBASE_H

#include "dberr.h"

#define DB_VERSION              0x01000000
#define DB_MAGIC_SIGNATURE      0xA0B0C0D0

#define DBENTRY_RESERVEDID_LEAK 0x00000000
#define DBENTRY_FIXPARTLENGTH   0x20

#define DB_TYPE_FIX 0x00
#define DB_TYPE_VAR 0x01
#define DB_TYPE_IDX 0x02

typedef unsigned long  DBID,     *LPDBID;
typedef unsigned long  DBOFFSET, *LPDBOFFSET;
typedef unsigned short DBSIZE,   *LPDBSIZE;
typedef unsigned short DBWORD,   *LPDBWORD;
typedef char           DBFIX,    *LPDBFIX;
typedef char*          DBVAR,    *LPDBVAR;

typedef struct {
   DBID id;
   DBOFFSET offset;
} DBINDEX, *LPDBINDEX;

typedef struct {
   DBINDEX index;
   DBWORD age;
   union {
      struct {
         DBFIX name[DBENTRY_FIXPARTLENGTH];
         DBFIX address[DBENTRY_FIXPARTLENGTH];
      } fix;
      struct {
         DBVAR name;
         DBVAR address;
      } var;
   } data;
} DBENTRY, *LPDBENTRY;

typedef union {
   DBSIZE varPartSize;
   DBSIZE leakSize;
} DBENTRYHEADER, *LPDBENTRYHEADER;

typedef void* HDB;                     /* Tipo de dado abstrato para uma implementacao de DB */

unsigned long DBSignature(HDB);        /* Retorna a signature do DB           */
unsigned long DBVersion(HDB);          /* Retorna a versao do DB              */
unsigned char DBType(HDB);             /* Retorna o tipo do DB (fixo ou var.) */

DBSIZE EntryFixSize(HDB);              /* Retorna o tamanho da parte fixa de um registro     */
DBSIZE EntryVarSize(LPDBENTRY, HDB);   /* Retorna o tamanho da parte variavel de um registro */

DBERR CreateEmptyDB(const char*, const char*, unsigned char type, HDB*); /* Cria DB vazio e inicializa */
DBERR InitializeDB(const char*, const char*, HDB*);                      /* Carrega DB e inicializa    */
DBERR CloseDB(HDB);                                                      /* Fecha DB                   */

DBERR ReadEntryOffsetDB(DBOFFSET, LPDBENTRY, LPDBENTRYHEADER, HDB);      /* Le registro DB e header    */
DBERR ScanEntryIDDB(DBID, LPDBENTRY, HDB);                               /* Procura por registro no DB */
DBERR TraverseDB(int (*)(DBID, DBOFFSET, HDB, void*), int, HDB, void*);  /* Itera sobre os registros   */

DBERR InsertEntryDB(LPDBENTRY, HDB);                                     /* Insere registro no DB         */
DBERR RemoveEntryDB(DBID, HDB);                                          /* Remove registro do DB         */
DBERR RebuildDB(const char*, const char*, HDB*, HDB);                    /* Reconstroi DB removendo leaks */

#endif


/*

DB variavel: contem dois arquivos, o indice e o BD propriamente dito.
indice: [DBHEADER][ [DBINDEX] [DBINDEX] [DBINDEX] ... ]
        Os indices correspondentes aos registros ficam no inicio da lista, ao passo que
        os indices correspondentes aos leaks (fragmentos) ficam no final da lista.
BD:     [DBHEADER][ [DBENTRTYHEADER][DBENTRY]  [DBENTRTYHEADER][DBENTRY] ...  ]
        As estruturas de cabecalho dos registros contem informacoes sobre o tamanho da
        parte variavel ou sobre o tamanho do leak correspondente, em bytes.
        Todos os leaks tem o ID reservado, cujo valor e 0.
        
DB fixo: contem apenas um arquivo, o BD.
BD:     [DBHEADER][ [DBENTRY][DBENTRY][DBENTRY] ... ]
        As entradas de registro sao gravadas como estruturas de tamanhno fixo. Os leaks
        sempre sao reaproveitados ao maximo.
        
Todo o acesso ao DB e feito diretamente, sem carga em buffers e sem paginacao. Poderia
ser implementado mecanismos de ordenacao de indices para melhorar o desempenho das
operacoes padrao, assim como implementar um algoritmo de contiguacao de leaks (agrupar
leaks adjacentes).

As funcoes de DB foram implementadas de forma a funcionar como uma biblioteca, desvin-
culando-se do programa-interface principal. As regioes marcadas com 'implementation
specific' sao regioes do codigo especificas da implementacao atual do DB (i.e., caso
o registro de armazenamento padrao seja modificado, alteracoes deverao ser feitas nestas
regioes demarcadas para que o sistema funcione corretamente). Os tipos utilizados foram
criados para tornarem-se flexiveis para ilustrar uma possivel expansao futura.

*/
