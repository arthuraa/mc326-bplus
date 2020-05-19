#include <stdlib.h>


#define DIMENSAO_NO 5
#define DIMENSAO_BLOCO 10
#define DIMENSAO_CHAVE_NOME 10

/* arvore b+
   cabecalho: 
   int altura da arvore; nao conta o nivel dos indices
   int primeiro disponivel; lista ligada de blocos disponiveis
*/

// enderecos em RBN

// tipos para arvore b+ primaria e secundaria de idade

typedef struct {
  int chave;
  int endereco;
} Indice;

typedef struct {
  int numero;
  int delimitadores[DIMENSAO_NO];
  int enderecos[DIMENSAO_NO + 1];
} NoPrimario; 

typedef struct {
  int numero;
  Indice indices[DIMENSAO_BLOCO];
  int ant, prox;
} BlocoPrimario;

// tipos para arvore b+ secundaria de nome

typedef struct {
  char nome[DIMENSAO_CHAVE_NOME];
  int endereco;
} IndiceSecundarioNome;

typedef struct {
  int numero;
  char delimitadores[DIMENSAO_NO][DIMENSAO_CHAVE_NOME];
  int enderecos[DIMENSAO_NO + 1];
} NoSecundarioNome;

typedef struct {
  int numero;
  Indice2Nome indices[DIMENSAO_BLOCO];
  int ant, prox;
} BlocoSecundarioNome;

//

void *blocoAtual;

FILE *arquivo;

int Posicao(char[] arq) {
  int pos;
  arquivo = fopen(arq, "r+");
  fread(&pos, sizeof(int), 1, arquivo);
  if(pos == -1) {
    

   
