#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
