#pragma once

    #include "header.h"
    #include "registro.h"
    #include "index.h"
    #include "utils.h"

    #define CREATE_TABLE             1
    #define SELECT_FROM              2
    #define SELECT_WHERE             3
    #define RECUPERACAO_RRN          4
    #define CREATE_INDEX             5
    #define SELECT_WHERE_USING_INDEX 6
    #define DELETE                   7
    #define INSERT_INTO              8
    #define UPDATE                   9


    void CreateTable(char *arquivoEntrada, char *arquivoSaida);
    void SelectFrom(char *arquivoEntrada);
    void SelectWhere(char *arquivoEntrada, int nroBuscas);
    void RecuperacaoRRN(char *arquivoEntrada, int RRN);
    
    // cria o arquivo de índice a partir do arquivo de dados, usando codEstacao para indexar
    void CriarIndex(FILE *arquivoDados, FILE* arquivoIndex);

    // Busca com índice primário/ chave (codEstação) ou sequencial (demais campos, como no SelectWhere)
    void SelectWhereIndex(char *arquivoDados, char *arquivoIndex, int nroBuscas);

    // Busca e remove os registros que dão match com os filtros
    void Delete(char *arquivoDados, char *arquivoIndex, int nroRemocoes);

    // Insere novos registros no arquivo de dados e atualiza o índice
    void InsertInto(char *arquivoDados, char *arquivoIndex, int nroInsercoes);
    