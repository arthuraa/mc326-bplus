#include "debug.h"

#ifdef DEBUG
void dump(void* mem, int len) {
   int count;
   for (count = 0; count < len; ++count) fprintf(stderr,"%02X ",*(((unsigned char*)mem)+count));
}
#endif

