#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE *arq;    
    char byte;
    int i;
    arq = fopen("arq01", "r");
    while( fread( (void *) &byte, sizeof(char), 1, arq) ) {
        for(i=0;i<8;i++) { 
          if(byte & 0x100) printf("1"); else printf("0");
          byte = byte << 1;
        }
    }      
    fclose(arq);
    return 0;
}
