#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE *arq;
    int i;
    char nome[50];
    int idade;
    int comp;
    arq = fopen("arq01", "r");
    for(i=0;i<3;i++) {
      fread( (void *) &comp, sizeof(int), 1, arq);
      fread( (void *) nome, comp*sizeof(char), 1, arq);
      nome[comp] = '\0';
      fread( (void *) &idade, sizeof(int), 1, arq);
      printf("%s %i\n", nome, idade);
    }
    fclose(arq);
    return 0;
}
