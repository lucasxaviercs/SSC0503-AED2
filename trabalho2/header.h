#pragma once

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>

    #define TAM_CABECALHO 17

    typedef struct {
        char status; // indica se o arquivo está consistente ('1') ou inconsistente ('0')
        int topo; // armazena o RRN de um registro logicamente removido ou '-1' caso não haja nenhum registro logicamente removido
        int proxRRN; // próximo RRN disponível
        int nroEstacoes; // indica a quantidade de estações diferentes no arquivo de dados
        int nroParesEstacao; // indica a quantidade de pares (codEstacao, codProxEstacao) 
    } Header;

    // Aloca e inicializa a struct do cabeçalho na RAM com os valores padrão
    Header *InicializarCabecalho();

    // Posiciona o cursor no byte zero e lê os 17 bytes do cabeçalho para a struct
    void LerCabecalhoBIN(FILE *arquivoBIN, Header *cabecalho);

    // Posiciona o cursor no byte zero e graas os dados da struct no disco
    void EscreverCabecalhoBIN(FILE* arquivoBIN, const Header* cabecalho);