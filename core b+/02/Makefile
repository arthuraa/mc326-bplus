DEBUGPATH = ../../src:../../src/include
VPATH = src:include:$(DEBUGPATH)
INCLUDEDIR = $(patsubst %,-I%,$(subst :, ,$(VPATH)))
DEBUG =
CC = gcc
CFLAGS = -Wall -pedantic-errors -ansi $(INCLUDEDIR) $(DEBUG)

all :		pbackup db binsearch

db :		main.o dbcore.o dbf.o debug.o
		$(CC) $^ -o db $(CFLAGS) $(CPPFLAGS)

main.o :	main.c dbbase.h
		$(CC) $< -c $(CFLAGS) $(CPPFLAGS)

dbcore.o :	dbcore.c dbcore.h dbbase.h dberr.h dbf.h debug.h
		$(CC) $< -c $(CFLAGS) $(CPPFLAGS)

dbf.o :		dbf.c dbf.h
		$(CC) $< -c $(CFLAGS) $(CPPFLAGS)

debug.o :	debug.c debug.h
		$(CC) $< -c $(CFLAGS) $(CPPFLAGS)

binsearch :	binsearch.c
		$(CC) $^ -o $@ $(CFLAGS) $(CPPFLAGS)

debug :		; $(MAKE) DEBUG=-DDEBUG

pbackup :	;
		@echo "Persistent backup..."
		cp -r -u include src pbackup

backup :	;
		@echo "Backup..."
		cp -r -u include src backup

distclean clean : ; -$(RM) *.o db binsearch

.PHONY:   clean distclean debug backup pbackup

