/* gera uma base de dados com 6 registros */
/* com os nomes de dbdat.dat e dbidx.dat  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    char input[100];
    int i;
    if ( argc < 3 ) printf ("Uso: <nome do executavel> <modo fixo ou variavel (v ou f)>\n");
    else {         
         strcpy(input, argv[1]);
         strcat(input, " ");
         strcat(input, argv[2]);
         strcat(input, " dbdat.dat dbidx.dat");
         system(input); // inicializa o arquivo
         
         strcpy(input, argv[1]);
         strcat(input, " i dbdat.dat dbidx.dat 1 13 \"Carlos da Silva\" \"rua Legal\"");
         system(input);
         strcpy(input, argv[1]);
         strcat(input, " i dbdat.dat dbidx.dat 2 78 \"Lucia Romao\" \"rua Feliz\"");
         system(input);
         strcpy(input, argv[1]);
         strcat(input, " i dbdat.dat dbidx.dat 3 98 \"Paulo Costa\" \"avenida Oba Oba\"");
         system(input);
         strcpy(input, argv[1]);
         strcat(input, " i dbdat.dat dbidx.dat 4 35 \"Marilia Nevez\" \"rua 4769\"");
         system(input);
         strcpy(input, argv[1]);
         strcat(input, " i dbdat.dat dbidx.dat 5 45 \"Apollonio Siqueira\" \"travessa Toc Toc\"");
         system(input);
         strcpy(input, argv[1]);
         strcat(input, " i dbdat.dat dbidx.dat 6 20 \"Quem Bate\" \"rua Nao Sei\"");            
         system(input);
    }
}

