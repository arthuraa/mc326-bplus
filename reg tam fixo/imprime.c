#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(void) {
    FILE *arq;
    int i;
    char nome[50];
    int idade;    
    arq = fopen("arqa", "r");    
    for(i=0;i<3;i++) {
      fread( (void *) nome, 50*sizeof(char), 1, arq);
      fread( (void *) &idade, sizeof(int), 1, arq);
      printf("%s %i\n", nome, idade);
    }
    getchar();
    fclose(arq);
    return 0;
}    
      
