#include <dbf.h>

size_t fwritep(const void* ptr, size_t size, long off, int whence, FILE* stream) {
   if (fseek(stream,off,whence)) return 0;
   return fwrite(ptr,size,1,stream);
}

size_t freadp(void* ptr, size_t size, long off, int whence, FILE* stream) {
   if (whence != SEEK_CUR || off) if (fseek(stream,off,whence)) return 0;
   return fread(ptr,size,1,stream);
}

size_t fwrited(const void* ptr, size_t size, long off, int whence, FILE* stream) {
   if (!fwritep(&size,sizeof(size),off,whence,stream)) return 0;
   return fwrite(ptr,size,1,stream);
}

size_t freadd(void* ptr, size_t* size, long off, int whence, FILE* stream) {
   if (!freadp(size,sizeof(size_t),off,whence,stream)) return 0;
   return fread(ptr,*size,1,stream);
}

