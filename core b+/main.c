#include <stdio.h>
#include <stdlib.h>

/* banco de dados: dados.dat
   contem no cabecalho um int que da o offset do primeiro
   bloco de uma lista ligada de blocos disponíveis.
   um registro de uma pessoa contem tres campos: 
   o tamanho da parte variavel do registro, em bytes (int),
   o nome da pessoa (string) e sua idade (int).
   um bloco removido contem dois campos, ambos do tipo int:
   o tamanho do bloco em bytes e o offset do proximo bloco
   disponivel. usa-se a tecnica de first fit para reaproveitamento de 
   espacos livres para a insercao de novos registros
   o bloco podera ser utilizado apenas se o registro a ser inserido
   tiver exatamente seu tamanho ou se a diferenca entre os dois
   tamanhos for maior ou igual a 2*sizeof(int), o que permite 
   a insercao de um novo bloco na lista de blocos disponiveis.
   se nao ha blocos disponiveis para reaproveitamento, o registro e inserido
   no final do arquivo de dados.
*/
   
void LeRegistro(){

}

void GravaRegistro(){

}
