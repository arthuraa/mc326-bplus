#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>

#if defined(_BCPPB) && defined(_DEBUG)
#define DEBUG
#define __STRING(x)  #x
#define __FUNCTION__ __FUNC__
#endif

#ifdef DEBUG
#define var(x,y) fprintf(stderr,__STRING(x)" = "y,x)
#define value(x,y) fprintf(stderr,y,x)
#define out(x) fprintf(stderr,x)
#define stop() fgetc(stdin)
#define mark(x) fprintf(stderr,"%s@%s:%i: %s",__FUNCTION__,__FILE__,__LINE__,x)
#define nl fprintf(stderr,"\n")
void dump(void*, int);
#else
#define var(x,y)   ((void)0)
#define value(x,y) ((void)0)
#define out(x)     ((void)0)
#define stop()     ((void)0)
#define mark(x)    ((void)0)
#define dump(x,y)  ((void)0)
#define nl         ((void)0)
#endif
#endif

