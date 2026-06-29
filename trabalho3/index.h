#pragma once

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>
    #include "header.h"
    #include "utils.h"
    
    #define TAM_INDEX_HEADER 1
    #define TAM_INDEX_REGISTRO 8
  
    typedef struct {
        char status; // indica a consistência do arquivo de dados
    } IndexHeader;

    typedef struct {
        int codEstacao; // código único que identifica a estação
        int RRN; // RRN do registro correspondente ao código da estação
    } IndexRegistro;

    // Carrega o arquivo de índice em memória primária para uso
    void CarregarIndex(FILE *arquivoIndex, IndexRegistro **registros, int *totalRegs, Header *cabecalhoDados);

    // Reescreve o arquivo de índice no disco após operações
    void ReescritaIndex(FILE *arquivoIndex, IndexRegistro *registros, int totalRegs, char *nomeArquivo);

    // Realiza busca binária no vetor de índices
    int BuscarRegistroIndex(IndexRegistro *registros, int totalRegs, int codEstacao);

    // Insere ordenadamente um novo ID e RRN no vetor de índices, abrindo espaço
    void InserirRegistroIndex(IndexRegistro **registros, int codEstacao, int RRN, int *totalRegs);

    // Remove um ID do vetor de índices, realocando e puxando os elementos restantes para a esquerda
    void RemoverRegistroIndex(IndexRegistro **registros, int *totalRegs, int codEstacao);

    // Função auxiliar para ler um registro do arquivo de índices para struct
    void LerRegistroIndex(FILE *arquivoIndex, IndexRegistro *registro);

    // Função auxiliar para escrever um registro da struct para o arquivo de índices
    void EscreverRegistroIndex(FILE *arquivoIndex, IndexRegistro *registro);

    // Realiza a leitura da estrutura de cabeçalho dos índices
    void LerCabecalhoIndex(FILE *arquivoIndex, IndexHeader *cabecalhoIndex);

    // Escreve a estrutura do cabeçalho do índice, reposicionando o cursor no ínicio do arquivo com fseek()
    void EscreverCabecalhoIndex(FILE *arquivoIndex, IndexHeader *cabecalhoIndex);

    // Função auxiliar para o uso do quicksort da stdlib
    int CompararIndexRegistro(const void *A, const void *B);
