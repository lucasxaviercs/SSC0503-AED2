#pragma once

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>

    #define TAM_INDEX_HEADER 1
    #define TAM_INDEX_REGISTRO 8
  
    typedef struct {
        char status; // indica a consistência do arquivo de dados
    } IndexHeader;

    typedef struct {
        int codEstacao; // código único que identifica a estação
        int RRN; // RRN do registro correspondente ao código da estação
    } IndexRegistro;

    // carrega o arquivo de índice em memória primária para uso
    void CarregarIndex(FILE *arquivoIndex, IndexRegistro **registros, int *totalRegs);
    // reescreve o arquivo de índice no disco após operações
    void ReescritaIndex(FILE *arquivoIndex, IndexRegistro *registros, int totalRegs);

    // operações de busca, inserção e remoção no índice
    int BuscarRegistroIndex(FILE *arquivoIndex, int codEstacao);
    void InserirRegistroIndex(IndexRegistro **registros, int codEstacao, int RRN, int *totalRegs);
    void RemoverRegistroIndex(FILE *arquivoIndex, int codEstacao);

    // função auxiliar para ler um registro do arquivo de índices para struct
    void LerRegistroIndex(FILE *arquivoIndex, IndexRegistro *registro);
    // função auxiliar para escrever um registro da struct para o arquivo de índices
    void EscreverRegistroIndex(FILE *arquivoIndex, IndexRegistro *registro);

    // realiza a leitura da estrutura de cabeçalho dos índices
    void LerCabecalhoIndex(FILE *arquivoIndex, IndexHeader *cabecalhoIndex);

    // função auxiliar para o uso do quicksort da stdlib
    int CompararIndexRegistro(const void *A, const void *B);
