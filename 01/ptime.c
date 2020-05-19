#include <time.h>
#include "ptime.h"

float ptime(void) {
   static clock_t reg; static int flag = 0;
   if (!flag) { flag = 1; reg = clock(); }
   return ((float)(reg = clock()-reg))/CLK_TCK;
}
