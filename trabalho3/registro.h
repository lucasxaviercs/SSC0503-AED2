#pragma once

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <ctype.h>

    #define TAM_REGISTRO 80

    typedef struct {
        char removido; // indica se o registro foi logicamente removido ou não
        int proximo; // armazena o RRN do próximo registro logicamente removido
        int codEstacao; // código da estação
        int codLinha; // código da linha
        int codProxEstacao; // código da próxima estação
        int distProxEstacao; // distância para a próxima estação
        int codLinhaIntegra; // código da linha que faz a integração entre as linhas
        int codEstIntegra; // código da estação que faz a integração entre as linhas
        int tamNomeEstacao; // tamanho do campo nomeEstacao
        char *nomeEstacao; // nome da estação
        int tamNomeLinha; // tamanho do campo nomeLinha
        char *nomeLinha; // nome da linha
    } Registro;

    // Lê 80 bytes do disco e extrai os campos para a struct, alocando as strings dinâmicamente
    void LerRegistroBIN(FILE *arquivoBIN, Registro *registroDados);

    // Extrai e converte os dados de uma linha de texto do CSV para a struct do registro
    void LerRegistroCSV(FILE *arquivoCSV, Registro *registroDados);

    // Grava a struct no disco, preenchendo o espaço que sobra com lixo ('$')
    void EscreverRegistroBIN(FILE* arquivoBIN, const Registro* registroDados);