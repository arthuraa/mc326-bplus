#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE *arq;
    int i, idade, comp, nregs;
    char nome[50];    
    arq = fopen("arqb", "w");
    scanf("%i", &nregs);
    fwrite( (void *) &nregs, sizeof(int), 1, arq);
    for(i=0;i<nregs;i++) {
      fflush(stdin);               
      fgets( (void *) nome, 50, stdin);
      comp = strlen(nome);
      fwrite( (void *) &comp, sizeof(int), 1, arq);
      fwrite( (void *) nome, comp*sizeof(char), 1, arq);
      scanf( "%i", &idade);
      fwrite( (void *) &idade, sizeof(int), 1, arq);
    }
    fclose(arq);
    return 0;
}
