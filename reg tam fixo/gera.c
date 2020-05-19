#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *arq;

int main(void) {
  int i;
  int idade;
  char nome[50];  
  arq = fopen("arqa", "w");
  for(i=0;i<3;i++) {
    fflush(stdin); 
    fgets((void *) nome, 50, stdin);
    fwrite( (void *) nome, 50*sizeof(char), 1, arq);
    scanf("%i", &idade);
    fwrite( (void *) &idade, sizeof(int), 1, arq);
  }
  fclose(arq);
  return 0;
}
