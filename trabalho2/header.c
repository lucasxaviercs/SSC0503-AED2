#include "header.h"


/*Inicializa a struct do cabeçalho na memória com valores inicias definido nas instruções do trabalho.
Retornando o endereço de memória reservado para a struct*/
Header *InicializarCabecalho(){
    Header *cabecalho = malloc(sizeof(Header));

    if (cabecalho != NULL){
        // Inicialização do cabeçalho na memória primária (RAM)
        cabecalho->status = '0'; // Para quando abrir para escrita estar inconsistente
        cabecalho->topo = -1; // Não há registros logicamente removidos
        cabecalho->proxRRN = 0; // O próximo RRN disponível deve ser iniciado com o valor 0
        cabecalho->nroEstacoes = 0; // Indica a quantidade de estações
        cabecalho->nroParesEstacao = 0; // Indica a quantidade de pares
    }
    
    return cabecalho;
}

/*Posiciona a leitura sempre no início do arquivo por meio do fseek e extrai os campos da struct do cabeçalho.
Atualizando os valores lidos por meio da passagem por referência (endereço de memória do cabeçalho)*/
void LerCabecalhoBIN(FILE *arquivoBIN, Header *cabecalho){
    if(cabecalho == NULL) return;

    // Garante estar no início do arquivo
    fseek(arquivoBIN, 0, SEEK_SET);

    fread(&cabecalho->status, sizeof(char), 1, arquivoBIN);
    fread(&cabecalho->topo, sizeof(int), 1, arquivoBIN);
    fread(&cabecalho->proxRRN, sizeof(int), 1, arquivoBIN);
    fread(&cabecalho->nroEstacoes, sizeof(int), 1, arquivoBIN);
    fread(&cabecalho->nroParesEstacao, sizeof(int), 1, arquivoBIN);
}

/*Posiciona o cursor do arquivo no ínicio do arquivo e escreve os dados
atuais do cabeçalho no disco*/
void EscreverCabecalhoBIN(FILE* arquivoBIN, const Header* cabecalho){
    // Garante estar no início do arquivo
    fseek(arquivoBIN, 0, SEEK_SET);

    fwrite(&cabecalho->status, sizeof(char), 1, arquivoBIN);
    fwrite(&cabecalho->topo, sizeof(int), 1, arquivoBIN);
    fwrite(&cabecalho->proxRRN, sizeof(int), 1, arquivoBIN);
    fwrite(&cabecalho->nroEstacoes, sizeof(int), 1, arquivoBIN);
    fwrite(&cabecalho->nroParesEstacao, sizeof(int), 1, arquivoBIN);
}