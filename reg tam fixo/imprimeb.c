#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(void) {
    FILE *arq;
    int i;
    char nome[50];
    int idade, nregs;    
    arq = fopen("arqb", "r");
    fread( (void *) &nregs, sizeof(int), 1, arq);   
    for(i=0;i<nregs;i++) {
      fread( (void *) nome, 50*sizeof(char), 1, arq);
      fread( (void *) &idade, sizeof(int), 1, arq);
      printf("%s %i\n", nome, idade);
    }
    fclose(arq);
    return 0;
}    
      
