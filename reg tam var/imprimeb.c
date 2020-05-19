#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE *arq;
    int i, idade, comp, nregs;
    char nome[50];    
    arq = fopen("arqb", "r");
    fread( (void *) &nregs, sizeof(int), 1, arq);
    for(i=0;i<nregs;i++) {
      fread( (void *) &comp, sizeof(int), 1, arq);
      fread( (void *) nome, comp*sizeof(char), 1, arq);
      nome[comp] = '\0';
      fread( (void *) &idade, sizeof(int), 1, arq);
      printf("%s %i\n", nome, idade);
    }
    fclose(arq);
    return 0;
}
