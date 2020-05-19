#include <stdlib.h>
#include <stdio.h>

int compare(const void* a, const void* b) {
   if (*((int*)a) > *((int*)b)) return 1;
   else if (*((int*)a) < *((int*)b)) return -1;
   else return 0;
}

int binsearch(int* buf, int x, int start, int end) {
   int mid = (start+end)/2;
   if (mid == start) {
      if (buf[start] > x) return -(start+1);
      else if (buf[start] < x) return -(end+1);
      return start;
   }
   if (buf[mid] > x) return binsearch(buf,x,start,mid);
   else if (buf[mid] < x) return binsearch(buf,x,mid,end);
   else return mid;
}

int main(int argc, char** argv) {
   int* ptr, *aux, count;
   if (argc < 3) return 0;
   for (ptr = NULL, count = 1; count < argc-1; ++count) {
      if ((aux = (int*)realloc(ptr,count*sizeof(int))) == NULL) {
         perror("Cannot realloc");
         if (ptr) free(ptr);
         return 1;
      }
      (ptr = aux)[count-1] = atoi(argv[count]);
   }
   qsort(ptr,argc-2,sizeof(int),compare);
   for (count = 0; count < argc-2; ++count) printf("%i ",ptr[count]);
   printf("\n%i\n",binsearch(ptr,atoi(argv[argc-1]),0,argc-2));
   free(ptr);
   return 0;
}

