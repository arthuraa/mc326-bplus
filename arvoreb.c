#include <stdio.h>
#include <stdlib.h>
#define MAX_ELEM 5

typedef struct {
  int chave;
  int offset;
} indice;

typedef struct no{
  int num_ind;
  indice indices[MAX_ELEM];
  int filhos[MAX_ELEM + 1];
}

static FILE *arvore;
static no atual;
static int dir;
static boolean subiu;

int BuscaIndice(int comeco, int tam, int chave) {
// retorna a posicao do elemento no bloco da arvore b
// ou -(1+pontodeincercao)
  int pos=comeco+tam/2;
  if(!tam) return 
  if(atual.indice[pos]==chave) return pos;
  if(atual.indice[pos]>chave) return BuscaIndice(comeco, pos-com+1, chave);
  
int BuscaBloco(void) {
// guarda = o endereço para gravação de um
// novo bloco da arvore b, removendo-o da lista
// de blocos disponiveis se necessario
// poe o cursor de leitura nessa posicao
  fseek(arvore, sizeof(int), SEEK_SET);
  fread(&grava, sizeof(int), 1, arvore);
  if(grava != -1) { //remove um bloco da lista de blocos disponiveis
    fseek(arvore, grava, SEEK_SET);
    fread(&i, sizeof(int), 1, arvore);
    fseek(arvore, sizeof(int), SEEK_SET);
    fwrite(&i, sizeof(int), 1, arvore);
  }
  else {
    fseek(arvore, 0, SEEK_END);
    grava = ftell(arvore);
  }
}

boolean InsereArvore(indice ind) {
  int end;
  fseek(arvore, 0, SEEK_SET);
  fread(&end, sizeof(int), 1, arvore);
  if(!InsereArvoreAux(end, &ind)) return false;
  if(subiu) {
    atual.num_ind = 1;
    atual.filhos[0] = end;
    atual.filhos[1] = dir;
    atual.indices[0] = ind;
    BuscaBloco();
    fwrite(&atual, sizeof(no), 1, arvore);
    fseek(arvore, 0, SEEK_SET);
    fwrite(&grava, sizeof(int), 1, arvore);
  }
  return true;
}

boolean InsereArvoreAux(int end, indice *ind) {
  
  int i;
  int ponto; // ponto de insercao de um indice que sobe
  int tint; // auxiliar de deslocamento dos filhos
  indice tind; // auxiliar de deslocamento dos indices

  if(end == -1) {
    subiu = true;
    return true;
  }
 
  fseek(arvore, end, SEEK_SET);
  fread(&atual, sizeof(no), 1, arvore);
  if((ponto = BuscaIndices(0, atual.num_ind, ind->chave)) >=0) return false; //indice ja existe na arvore
  else {    
    if(!InsereArvoreAux(atual.filhos[ponto], ind)) return false;    
    if(subiu) {
      ponto = -(ponto+1);
      if(atual.num_ind == MAX_ELEM) {
        atual.num_ind /= 2;
        if(ponto < atual.num_ind) {
          tind = atual.indices[atual.num_ind-1];
          tint = atual.filhos[atual.num_ind];
          for(i=atual.num_ind-1;i>ponto;i--) {
            atual.indices[i] = atual.indices[i-1];
            atual.filhos[i+1] = atual.filhos[i];
          }
          atual.indices[ponto] = ind;
          atual.filhos[ponto+1] = dir;
          ind = tind;
          dir = tint;
          ponto = atual.num_ind;
        }        
        fseek(arvore, end, SEEK_SET);        
        fwrite(&atual, sizeof(no), 1, arvore);
        if(ponto == atual.num_ind) {***
          atual.num_ind = MAX_ELEM - atual.num_ind;
          atual.filhos[0] = dir;
          for(i=0;i<(tual.num_ind;i++) {
            atual.indices[i] = atual.indices[i+atual.num_ind]



        }
      else {
        for(i=atual.num_ind;i<ponto;i--) atual.indices[i] = atual.indices[i-1];
        atual.indices[ponto] = ind;
        subiu = false;
      }
    }
    return true;
  }




}
    
