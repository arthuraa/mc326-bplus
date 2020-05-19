#define RMVIDX_SUCESSO 0
#define RMVIDX_FALHA 1
#define RMVIDX_EMRPESTA_GALHO 2
#define RMVIDX_EMPRESTA_SS 3

#include <string.h>

typedef short int RMVIDX;


RMVIDX RemoveIndiceAux ( HDBINDEX baseId, LPDBDATA index,  DBOFFSET off, void **menor, void *param) {
  int i; /* contador */
  int posicao;
  DBOFFSET offBusca;  

  ReadIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
  posicao = NodeSearch( baseId, &idxcast(baseId)->blockBuffer[0], 0, idxcast(baseId)->blockBuffer[0].blockSize - 1, index, param );

  if ( idxcast(baseId)->blockBuffer[0].leafNode ) {
    
    if( posicao < 0 ) return RMVIDX_FALHA;

    /* o elemento esta na folha; pode ser removido */

    idxcast(baseId)->blockBuffer[0].blockSize--;
    for( i=posicao; i<idxcast(baseId)->blockBuffer[0].blockSize; i++) {
      memcpy( idxcast(baseId)->blockBuffer[0].data[i], idxcast(baseId)->blockBuffer[0].data[i+1], idxcast(baseId)->dataSize );
      idxcast(baseId)->blockBuffer[0].offset[i] = idxcast(baseId)->blockBuffer[0].offset[i+1];
    }

    WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0] );

    if ( posicao == 0 ) memcpy ( *menor, idxcast(baseId)->blockBuffer[0].data[0], idxcast(baseId)->dataSize );      

    if ( idxcast(baseId)->blockBuffer[0].blockSize < idxcast(baseId)->blockSize/2 ) return RMVIDX_EMPRESTA_SS;
    
  }

    else { /* nao esta na folha */

    offBusca = idxcast(baseId)->blockBuffer[0].offset[posicao];

    /* remove nos filhos */
    switch( RemoveIndiceAux ( baseId, index, offBusca, menor, param ) ) {

            case RMVIDX_SUCESSO:

	      if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
		ReadIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
		memcpy( idxcast(baseId)->blockBuffer[0].data[posicao - 1], *menor, idxcast(baseId)->dataSize);
		WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
		free(*menor);
		*menor = NULL;
	      }	/* troca delimitadores */
	      break;


	    case RMVIDX_EMPRESTA_SS:

	      ReadIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0] );

	      if ( posicao > 0 ) {
		ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao-1], &idxcast(baseId)->blockBuffer[1] );
                ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[2] );
		if ( idxcast(baseId)->blockBuffer[1].blockSize > idxcast(baseId)->blocksize/2 ) {/* empresta do irmao esquerdo */
		  for ( i=idxcast(baseId)->blockBuffer[2].blockSize++; i>0; i-- ) {
		    memcpy( idxcast(baseId)->blockBuffer[2].data[i], idxcast(baseId)->blockBuffer[2].data[i-1], idxcast(baseId)->dataSize );
		    idxcast(baseId)->blockBuffer[2].offset[i] = idxcast(baseId)->blockBuffer[2].offset[i-1];
		    /* desloca o bloco que ficou abaixo do piso para emprestar */
		  }		    
		  memcpy( idxcast(baseId)->blockBuffer[2].data[0], idxcast(baseId)->blockBuffer[1].data[--idxcast(baseId)->blockBuffer[1].blockSize],
			  idxcast(baseId)->dataSize );
		  memcpy( idxcast(baseId)->blockBuffer[0].data[posicao-1], idxcast(baseId)->blockBuffer[2].data[0], idxcast(baseId)->dataSize );
		  idxcast(baseId)->blockBuffer[2].offset[0] = idxcast(baseId)->blockBuffer[1].offset[idxcast(baseId)->blockBuffer[1].blockSize];
		  WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
		  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao-1], &idxcast(baseId)->blockBuffer[1]);
		  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[2]);
		  if( *menor != NULL ) {
		    free(*menor);
		    *menor = NULL;
		  }
		  return RMVIDX_SUCESSO;
		}
	     }

	     if ( posicao < idxcast(baseId)->blockBuffer[0]->blockSize ) {
	       ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[1] );
               ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao+1], &idxcast(baseId)->blockBuffer[2] );
	       if ( idxcast(baseId)->blockBuffer[2].blockSize > idxcast(baseId)->blocksize/2 ) {/* empresta do irmao direito */
		 if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
		   memcpy( idxcast(baseId)->blockBuffer[0].data[posicao - 1], *menor, idxcast(baseId)->dataSize );
		   free(*menor);
		   *menor = NULL;
		 } /* troca o delimitador se necessario */
		 memcpy(&idxcast(baseId)->blockBuffer[1].data[idxcast(baseId)->blockBuffer[1].blockSize],idxcast(baseId)->blockBuffer[2].data[0],idxcast(baseId)->dataSize);
		 memcpy(&idxcast(baseId)->blockBuffer[0].data[posicao], &idxcast(baseId)->blockBuffer[2].data[1], idxcast(baseId)->dataSize);
		 idxcast(baseId)->blockBuffer[1].offset[idxcast(baseId)->blockBuffer[1].blockSize++] = idxcast(baseId)->blockBuffer[2].offset[0];
		 idxcast(baseId)->blockBuffer[2].blockSize--;
		 for ( i=0;i<idxcast(baseId)->blockBuffer[2].blockSize;i++) {
		   memcpy( idxcast(baseId)->blockBuffer[2].data[i], idxcast(baseId)->blockBuffer[2].data[i+1], idxcast(baseId)->dataSize);
		   idxcast(baseId)->blockBuffer[2].offset[i] = idxcast(baseId)->blockBuffer[2].offset[i+1];
		 } /* desloca o irmao que emprestou */
		 WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
		 WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[1]);
		 WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao+1], &idxcast(baseId)->blockBuffer[2]);
		 return RMVIDX_SUCESSO;
	       }
	     }

	     if ( posicao > 0 ) { /* junta com o irmao esquerdo */
	       	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao-1], &idxcast(baseId)->blockBuffer[1] );
                ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[2] );
		for( i=0; i < idxcast(baseId)->blockBuffer[2]->blockSize; i++ ) { /* junta os dois blocos */
		  memcpy(idxcast(baseId)->blockBuffer[1]->data[i + idxcast(baseId)->blockBuffer[1]->blockSize],idxcast(baseId)->blockBuffer[2]->data[i],idxcast(baseId)->dataSize);
		  idxcast(baseId)->blockBuffer[1]->offset[i + idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
		}
		idxcast(baseId)->blockBuffer[1]->blockSize += idxcast(baseId)->blockBuffer[2]->blockBuffer;
		idxcast(baseId)->blockBuffer[0]->blockSize--;

		for( i=posicao-1; i < idxcast(baseId)->blockBuffer[0]->blockSize; i++ ) { /* ajusta o pai */
		  idxcast(baseId)->blockBuffer[0]->data[i] = idxcast(baseId)->blockBuffer[0]->data[i+1];
		  idxcast(baseId)->blockBuffer[0]->offset[i+1] = idxcast(baseId)->blockBuffer[0]->offset[i+2];
		}

		if ( ( idxcast(baseId)->blockBuffer[1]->adjacent[1] = idxcast(baseId)->blockBuffer[2]->adjacent[1] ) != NULL ) {
		  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao - 1], &idxcast(baseId)->blockBuffer[1] );
		  ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[2]->adjacent[1], &idxcast(baseId)->blockBuffer[1] );
		  idxcast(baseId)->blockBuffer[1]->adjacent[0] = idxcast(baseId)->blockBuffer[2]->adjacent[0];
		  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[2]->adjacent[1], &idxcast(baseId)->blockBuffer[1] );
		} /* arruma as referencias do SS se preciso */

		else WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao - 1], &idxcast(baseId)->blockBuffer[1] );

		FreeIndexBlock( baseId, offBusca );

		if ( *menor != NULL ) {
		  free(*menor);
		  *menor = NULL;
		}

		if( idxcast(baseId)->blockBuffer[0]->blockSize < idxcast(baseId)->blockSize/2 )  return RMVIDX_EMPRESTA_GALHO;
	     }

	     else { /* junta com o irmao direito */
	       ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[1] );
               ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao+1], &idxcast(baseId)->blockBuffer[2] );

	       if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
		   memcpy( idxcast(baseId)->blockBuffer[0]->data[posicao - 1], *menor, idxcast(baseId)->dataSize );
		   free(*menor);
		   *menor = NULL;
	       }

	       for( i=0; i < idxcast(baseId)->blockBuffer[2]->blockSize; i++ ) { /* junta os dois blocos */
		 memcpy(idxcast(baseId)->blockBuffer[1].data[i+idxcast(baseId)->blockBuffer[1].blockSize],idxcast(baseId)->blockBuffer[2].data[i],idxcast(baseId)->dataSize);
		 idxcast(baseId)->blockBuffer[1].offset[i + idxcast(baseId)->blockBuffer[1].blockSize] = idxcast(baseId)->blockBuffer[2].offset[i];
	       }
	       idxcast(baseId)->blockBuffer[1].blockSize += idxcast(baseId)->blockBuffer[2].blockBuffer;
	       idxcast(baseId)->blockBuffer[0].blockSize--;

	       offBusca = idxcast(baseId)->blockBuffer[0].offset[posicao + 1]; /* offset do bloco a ser inserido na lista de blocos disponiveis */
	       
	       for( i=posicao+1; i < idxcast(baseId)->blockBuffer[0].blockSize; i++ ) { /* ajusta o pai */
		 memcpy(idxcast(baseId)->blockBuffer[0].data[i-1],idxcast(baseId)->blockBuffer[0].data[i],idxcast(baseId)->dataSize);
		 idxcast(baseId)->blockBuffer[0].offset[i] = idxcast(baseId)->blockBuffer[0].offset[i+1];
	       }

	       if ( ( idxcast(baseId)->blockBuffer[1].adjacent[1] = idxcast(baseId)->blockBuffer[2].adjacent[1] ) != NULL ) {
		 WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[1] );
		 ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[2].adjacent[1], &idxcast(baseId)->blockBuffer[1] );
		 idxcast(baseId)->blockBuffer[1].adjacent[0] = idxcast(baseId)->blockBuffer[2].adjacent[0];
		 WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[2].adjacent[1], &idxcast(baseId)->blockBuffer[1]);	       
	       } /* arruma as referencias no SS se necessario */

	       else WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId)->blockBuffer[1]) ;
	       
	       FreeIndexBlock( baseId, offBusca );

	       if ( idxcast(baseId)->blockBuffer[0].blockSize < idxcast(baseId)->blockSize/2 ) return RMVIDX_EMPRESTA_GALHO;
	     }	     
	     break;

    case RMVIDX_EMPRESTA_GALHO:
      ReadIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0] );

     
      
      if ( posicao > 0 ) {
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao-1], &idxcast(baseId).blockBuffer[1] );
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId).blockBuffer[2] );
	if ( idxcast(baseId)->blockBuffer[1].blockSize > idxcast(baseId)->blockSize/2 ) { /* pode emprestar do irmao */
	  for ( i=idxcast(baseId)->blockBuffer[2].blockSize++; i>0; i--) {
	    memcpy(idxcast(baseId)->blockBuffer[2].data[i], idxcast(baseId)->blockBuffer[2].data[i-1], idxcast(baseId)->dataSize);
	    idxcast(baseId)->blockBuffer[2].offset[i+1] = idxcast(baseId)->blockBuffer[2].offset[i];
	  } /*desloca para inserir */
	  idxcast(baseId)->blockBuffer[2].offset[1] = idxcast(baseId)->blockBuffer[2].offset[0];
	  idxcast(baseId)->blockBuffer[2].offset[0] = idxcast(baseId)->blockBuffer[1].offset[idxcast(baseId)->blockBuffer[1].blockSize];
	  memcpy(idxcast(baseId)->blockBuffer[2].data[0], idxcast(baseId)->blockBuffer[0].data[posicao-1], idxcast(baseId)->dataSize);
	  memcpy(idxcast(baseId)->blockBuffer[0].data[posicao-1],idxcast(baseId)->blockBuffer[1].data[idxcast(baseId)->blockBuffer[1].blockSize--],idxcast(baseId)->dataSize);
	  WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
	  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao-1], &idxcast(baseId)->blockBuffer[1]);
	  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[2]);
	  if ( *menor != NULL ) {
	    free(*menor);
	    *menor = NULL;
	  }
	}
      }
      
      if ( posicao < idxcast(baseId)->blockBuffer[0]->blockSize ) {
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId).blockBuffer[1] );
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao+1], &idxcast(baseId).blockBuffer[2] );
	
	if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
	  memcpy(idxcast(baseId)->blockBuffer[0].data[posicao - 1], *menor, idxcast(baseId)->dataSize);
	  free(*menor);
	  *menor = NULL;
	}

	if ( idxcast(baseId)->blockBuffer[2].blockSize > idxcast(baseId)->blockSize/2 ) {
	  idxcast(baseId)->blockBuffer[1].offset[idxcast(baseId)->blockBuffer[1].blockSize+1] = idxcast(baseId)->blockBuffer[2].offset[0];
	  memcpy(idxcast(baseId)->blockBuffer[1].data[idxcast(baseId)->blockBuffer[1].blockSize-1], idxcast(baseId)->blockBuffer[0].data[posicao], idxcast(baseId)->dataSize);
	  memcpy(idxcast(baseId)->blockBuffer[0].data[posicao], idxcast(baseId)->blockBuffer[2].data[0], idxcast(baseId)->dataSize);
	  for( i=0;i<idxcast(baseId)->blockBuffer[2].blockSize-1;i++) {
	    memcpy(idxcast(baseId)->blockBuffer[2].data[i] = idxcast(baseId)->blockBuffer[2].data[i+1]);
	    idxcast(baseId)->blockBuffer[2].offset[i] = idxcast(baseId)->blockBuffer[2].data[i+1];
	  }
	  idxcast(baseId)->blockBuffer[2].data[idxcast(baseId)->blockBuffer[2].blockSize-1] = idxcast(baseId)->blockBuffer[2].offset[idxcast(baseId)->blockBuffer[2].blockSize];
	  idxcast(baseId)->blockBuffer[2].blockSize--;
	  idxcast(baseId)->blockBuffer[1].blockSize++;
	  WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
	  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[1]);
	  WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao+1], &idxcast(baseId)->blockBuffer[2]);
        }
     }

      if ( posicao > 0 ) {
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao-1], &idxcast(baseId).blockBuffer[1] );
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId).blockBuffer[2] );
	memcpy(idxcast(baseId)->blockBuffer[1]->data[idxcast(baseId)->blockBuffer[1]->blockSize],idxcast(baseId)->blockBuffer[0]->data[posicao-1],idxcast(baseId)->dataSize);
	for( i=0; i < idxcast(baseId)->blockBuffer[2]->blockSize; i++ ) { /* junta os dois blocos */
	  memcpy(idxcast(baseId)->blockBuffer[1]->data[i+idxcast(baseId)->blockBuffer[1]->blockSize+1],idxcast(baseId)->blockBuffer[2]->data[i],idxcast(baseId)->dataSize);
	  idxcast(baseId)->blockBuffer[1]->offset[i+idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
       	}
	idxcast(baseId)->blockBuffer[1]->offset[i+idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
	idxcast(baseId)->blockBuffer[1]->blockSize += idxcast(baseId)->blockBuffer[2]->blockSize + 1;
	for( i=posicao-1; i < idxcast(baseId)->blockBuffer[0]->blockSize-1; i++) {
	  memcpy(idxcast(baseId)->blockBuffer[0]->data[i],idxcast(baseId)->blockBuffer[0]->data[i+1],idxcast(baseId)->dataSize);
	  idxcast(baseId)->blockBuffer[0]->offset[i+1] = idxcast(baseId)->blockBuffer[0]->offset[i+2];
	}
	idxcast(baseId)->blockBuffer[0].blockSize--;
	WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
	WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao-1], &idxcast(baseId)->blockBuffer[1]);
	FreeIndexBlock( baseId, offBusca );
	if ( *menor != NULL ) {
	    free(*menor);
	    *menor = NULL;
        }
	if (idxcast(baseId)->blockBuffer[0].blockSize < idxcast(baseId)->blockSize/2 ) return RMVIDX_EMPRESTA_GALHO;
	}

      else {
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao], &idxcast(baseId).blockBuffer[1] );
	ReadIndexBlock( baseId, idxcast(baseId)->blockBuffer[0].offset[posicao+1], &idxcast(baseId).blockBuffer[2] );

	if ( ( posicao != 0 ) && ( *menor != NULL ) ) {
	  memcpy(idxcast(baseId)->blockBuffer[0].data[posicao - 1], *menor, idxcast(baseId)->dataSize);
	  free(*menor);
	  *menor = NULL;
	}
	offBusca = idxcast(baseId)->blockBuffer[0]->offset[posicao+1];
	memcpy(idxcast(baseId)->blockBuffer[1]->data[idxcast(baseId)->blockBuffer[1]->blockSize],idxcast(baseId)->blockBuffer[0]->data[posicao],idxcast(baseId)->dataSize);
	for( i=0; i < idxcast(baseId)->blockBuffer[2]->blockSize; i++ ) { /* junta os dois blocos */
	  memcpy(idxcast(baseId)->blockBuffer[1]->data[i+idxcast(baseId)->blockBuffer[1]->blockSize+1],idxcast(baseId)->blockBuffer[2]->data[i],idxcast(baseId)->dataSize);
	  idxcast(baseId)->blockBuffer[1]->offset[i+idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
       	}
	idxcast(baseId)->blockBuffer[1]->offset[i+idxcast(baseId)->blockBuffer[1]->blockSize] = idxcast(baseId)->blockBuffer[2]->offset[i];
	idxcast(baseId)->blockBuffer[1]->blockSize += idxcast(baseId)->blockBuffer[2]->blockSize + 1;
	for( i=posicao; i < idxcast(baseId)->blockBuffer[0]->blockSize-1; i++) {
	  memcpy(idxcast(baseId)->blockBuffer[0]->data[i],idxcast(baseId)->blockBuffer[0]->data[i+1],idxcast(baseId)->dataSize);
	  idxcast(baseId)->blockBuffer[0]->offset[i+1] = idxcast(baseId)->blockBuffer[0]->offset[i+2];
	}
	idxcast(baseId)->blockBuffer[0].blockSize--;
	WriteIndexBlock( baseId, off, &idxcast(baseId)->blockBuffer[0]);
	WriteIndexBlock( baseId, idxcast(baseId)->blockBuffer[0]->offset[posicao], &idxcast(baseId)->blockBuffer[1]);
	FreeIndexBlock( baseId, offBusca );
	if (idxcast(baseId)->blockBuffer[0].blockSize < idxcast(baseId)->blockSize/2 ) return RMVIDX_EMPRESTA_GALHO;
	}
      }
    }

  return RMVIDX_SUCESSO;

  }





  DBERR RemoveIndice ( HDBINDEX baseId, LPDBDATA index, void* param) {
    DBOFFSET temp;
    void *menor = malloc(idxcast(baseId)->dataSize);
    switch ( RemoveIndice ( baseId, index, idxcast(baseId)->rootBlock, &menor, param )) {
  case RMVIDX_SUCESSO: 
    if ( menor != NULL ) {
      free(menor);
      menor = NULL;
    }
    return DBERR_SUCESS;
    break;
  case RMVIDX_FALHA:
    free(menor);
    menor = NULL;    
    return DBERR_ACCESS;
    break;
  case RMVIDX_EMPRESTA_GALHO:
    if ( menor != NULL ) {
      free(menor);
      menor = NULL;
    }
    ReadIndexBlock(baseId, idxcast(baseId)->rootBlock, &idxcast(baseId)->blockBuffer[0]); /* raiz ficou sem elementos, troca por seu filho */
    if( idxcast(baseId)->blockBuffer[0].blockSize == 0 ) {
      temp = idxcast(baseId)->blockBuffer[0].offset[0];
      FreeIndexBlock( baseId, idxcast(baseId)->rootBlock );
      /* poe endereco do no na lista de leaks */
      idxcast(baseId)->rootBlock = temp;
    }
    return DBERR_SUCCESS;
    break;
  case RMVIDX_EMPRESTA_SS:
    if ( menor != NULL ) {
      free(menor);
      menor = NULL;
    }
    ReadIndexBlock(baseId, idxcast(baseId)->rootBlock, &idxcast(baseId)->blockBuffer[0]);
    if( idxcast(baseId)->blockBuffer[0]->blockSize == 0 ){
      FreeIndexBlock( baseId, offBusca );
      idxcast(baseId)->rootBlock = NULL;
    }
    return DBERR_SUCCESS;
    break;	
  }
    return DBERR_SUCCESS; 
}   


  idxcast
