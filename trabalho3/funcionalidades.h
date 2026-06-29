#pragma once

    #include "header.h"
    #include "registro.h"
    #include "index.h"
    #include "utils.h"

    #define CREATE_TABLE               1
    #define SELECT_FROM                2
    #define SELECT_WHERE               3
    #define RECUPERACAO_RRN            4
    #define CREATE_INDEX               5
    #define SELECT_WHERE_USING_INDEX   6
    #define DELETE                     7
    #define INSERT_INTO                8
    #define UPDATE                     9

    // ==================== FUNC_CREATES ====================

    // Lê um CSV e converte todos os seus dados para um novo arquivo binário padronizado
    void CreateTable(char *arquivoEntrada, char *arquivoSaida);
    // Extrai os codEstacao e RRN para criar o índice, ordena em RAM e salva em um arquivo de índice
    void CreateIndex(char *arquivoDados, char *arquivoIndex); 


    // ================== FUNC_SELECTS_RRN ==================

    // Varre o arquivo binário do início ao fim e imprime todos os registros que estão ativos
    void SelectFrom(char *arquivoEntrada);
    // Busca sequencialmente e imprime registros que satisfaçam todos os filtros informados
    void SelectWhere(char *arquivoEntrada, int nroBuscas);
    // Busca otimizada com índice primário/chave (codEstação)  ou sequencial (demais campos, como no SelectWhere)
    void SelectWhereIndex(char *arquivoDados, char *arquivoIndex, int nroBuscas);
    // Salta matematicamente direto para a posição física de um RRN no disco e imprime seus dados
    void RecuperacaoRRN(char *arquivoEntrada, int RRN);

    // ================== FUNC_DEL_INS_UPDT =================
    
    // Encontra registros usando índices ou busca sequencial e os remove logicamente empilhando o espaço livre no topo da struct
    void Delete(char *arquivoDados, char *arquivoIndex, int nroRemocoes);
    // Lê novos dados e os grava no arquivo físico, reaproveitando espaços apagados (topo) ou inserindo no final
    void InsertInto(char *arquivoDados, char *arquivoIndex, int nroInsercoes);
    // Atualiza os registro com base nos filtros de buscas aplicados
    void Update(char *arquivoDados, char *arquivoIndex, int nroAtualizacoes);