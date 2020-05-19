/* DBERR.H: Codigos de erro para o banco de dados.          */
/* Compilado usando mingw (g++ (GCC) 3.4.2 (mingw-special)) */
/* Aleksey V T Covacevice RA030845 */
/* Arthur Amorim          RA031339 */

#ifndef __DBERR_H
#define __DBERR_H

#define DBERR_SUCCESS        0
#define DBERR_ACCESS         1
#define DBERR_MEMORY         2
#define DBERR_IO             3
#define DBERR_ARGUMENT       4
#define DBERR_FORMAT         5
#define DBERR_NOT_FOUND      6
#define DBERR_ALREADY_EXISTS 7
#define DBERR_LEAK_FOUND     8

typedef unsigned int DBERR;

#endif
