#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE *arq;    
    char byte;
    int i, c=0;
    arq = fopen("arqb", "r");
    while( fread( (void *) &byte, sizeof(char), 1, arq) ) {
        for(i=0;i<8;i++) { 
          (byte & 128) ? printf("1") : printf("0");
          byte = byte << 1; c++;
        }
    }
    printf("\n%i", c/8);     
    fclose(arq);
    return 0;
}
