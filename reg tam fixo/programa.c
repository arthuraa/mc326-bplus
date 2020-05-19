#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
        char marca; //marca de delecao
        char nome[50];
        int idade;
} Pessoa;

// gera registros com tamanho fixo

FILE *arq;

Pessoa p;

void gera(void) {
  int i, nregs, c=0;
  p.marca = 0;
  printf("Numero de registros:");
  scanf("%i", &nregs);
  scanf("\n");
  arq = fopen("arqa", "w");
  fwrite((void *) &c, sizeof(int), 2, arq); // apontadores para lista ligada de posicoes livres
  fwrite((void *) &nregs, sizeof(int), 1, arq);
  for(i=0;i<nregs;i++) {
    printf("Nome:"); fflush(stdin);
    fgets((void *) p.nome, 50, stdin);
    printf("Idade:");
    scanf("%i", &p.idade);
    fwrite((void *) &p, sizeof(Pessoa), 1, arq);
  }
  fclose(arq);
}

void imprime(void) {
  char c;  
  arq = fopen("arqa", "r");
  fseek(arq, 2*sizeof(int), SEEK_CUR);
  while(fread((void *) &c, sizeof(char), 1, arq))
    if(!c) {                   
      fread( (void *) p.nome, 50*sizeof(char), 1, arq);
      fread( (void *) &(p.idade), sizeof(int), 1, arq);
      printf("%s %i\n", p.nome, p.idade);
    }
  fclose(arq);
}

void consulta(void) {
     int n, nregs;     
     scanf("%i", &n);
     arq = fopen("arqa", "r");
     fseek(arq, 2*sizeof(int), SEEK_CUR);
     fread((void *) &nregs, sizeof(int), 1, arq);
     if(n>=nregs || n<0) { printf("registro nao encontrado"); return; }
     fseek(arq, n*sizeof(Pessoa), SEEK_CUR);
     fread((void *) &(p.marca), sizeof(char), 1, arq);
     if(!p.marca) printf("registro nao encontrado");
     else {
       fread( (void *) p.nome, 50*sizeof(char), 1, arq);
       fread( (void *) &(p.idade), sizeof(int), 1, arq);
       printf("%s %i\n", p.nome, p.idade);
       }
     fclose(arq);
}

void insere(void) {
     int ap;
     arq = fopen("arqa", "r+");
     p.marca = 0;
     fread((void *) &ap, sizeof(int), 1, arq);
     if(!ap) {
             fseek(arq, ap*sizeof(char), SEEK
     printf("Nome:"); fflush(stdin);
     fgets((void *) p.nome, 50, stdin);
     printf("Idade:");
     scanf("%i", &p.idade);
     
     
     
     
     fwrite((void *) &p, sizeof(Pessoa), 1, arq);



void deleta(void) {
        




int main(void) {
  char op;
  printf("Gerenciador de Registros - versao para tamanho fixo\nEscolha uma opcao('h' para ajuda)");
  scanf("%c", &op);
  while(1) switch (c) {
               case 'h':
               case 'H':
                 printf("(g)era arquivo\n(i)nsere registro\nim(p)rime arquivo\n(c)onsulta arquivo\n(d)eleta registro");
                 break;
               case 'g':
               case 'G':
                    gera();
                    break;
               case 'p':
               case 'P':
                    imprime();
                    break;
               case 'c':
               case 'C':
                    consulta();
               case 'd':
               case 'D':
                    deleta();
                    break;
               case 'i':
               case 'I':
                    insere();
                    break;
               default: return 0;
               }
}
              

