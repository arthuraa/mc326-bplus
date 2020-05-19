#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    FILE *arq;
    int i;
    char nome[50];
    int idade;
    int comp;
    arq = fopen("arq01", "w");
    for(i=0;i<3;i++) {
      fflush(stdin);               
      fgets( (void *) nome, 50, stdin); //printf("#-%s", nome);
      comp = strlen(nome);
      fwrite( (void *) &comp, sizeof(int), 1, arq);
      fwrite( (void *) nome, comp*sizeof(char), 1, arq);
      scanf( "%i", &idade);
      fwrite( (void *) &idade, sizeof(int), 1, arq);
    }
    fclose(arq);
    return 0;
}
