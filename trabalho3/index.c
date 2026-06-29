#include "index.h"


/* Lê o arquivo de índice do disco e o traz para um vetor em RAM*/
void CarregarIndex(FILE *arquivoIndex, IndexRegistro **registros, int *totalRegs, Header *cabecalhoDados) {
    if (arquivoIndex == NULL) return;

    // Realiza a leitura do índice de status do arquivo de índice, caso seja '0'
    // ele está inconsistente e imprimimos a mensagem de erro
    IndexHeader cabecalhoIndex;
    fseek(arquivoIndex, 0, SEEK_SET);
    if (fread(&cabecalhoIndex, sizeof(IndexHeader), 1, arquivoIndex) != 1) return;

    if (cabecalhoIndex.status == '0') { // checagem de consistência
        MensagemErro();
        return;
    }

    *totalRegs = 0;
    int capacidade = 10; // Começa com espaço para 10 registros
    *registros = malloc(capacidade * sizeof(IndexRegistro));

    IndexRegistro temp;
    // Realizamos a leitura dos registros de índice (codEstacao + RRN) até o fim do arquivo
    while(fread(&temp.codEstacao, sizeof(int), 1, arquivoIndex) == 1 && fread(&temp.RRN, sizeof(int), 1, arquivoIndex) == 1) {
        
        // Se atingir a capacidade máxima, ele dobra a capacidade
        if (*totalRegs >= capacidade) {
            capacidade *= 2;
            *registros = realloc(*registros, capacidade * sizeof(IndexRegistro));
        }
        
        // Adiciona o registro recém lido (último) na última posição e incrementa o total de registros
        (*registros)[*totalRegs] = temp;
        (*totalRegs)++;
    }

    // Realocamos a memória para o tamanho exato, para não haver consumo desnecessário de memória
    if (*totalRegs == 0) {
        free(*registros);
        *registros = NULL;
    } else {
        *registros = realloc(*registros, (*totalRegs) * sizeof(IndexRegistro));
    }
}
   
/* Reescreve todo o índice no disco, após apagar tudo nele com o modo "wb+" do freopen()*/
void ReescritaIndex(FILE *arquivoIndex, IndexRegistro *registros, int totalRegs, char *nomeArquivo) {
    if (arquivoIndex == NULL) return;

    // Fecha o fluxo atual e reabre o mesmo arquivo zerando seu conteúdo (truncamento)
    arquivoIndex = freopen(nomeArquivo, "wb+", arquivoIndex); // utilizado para evitar possíveis lixos no arq.
    if (arquivoIndex == NULL) return;

    // Escreve o cabeçalho marcando ele como consistente '1' após as operações
    IndexHeader cabecalhoIndex;
    cabecalhoIndex.status = '1'; 
    fwrite(&cabecalhoIndex, sizeof(IndexHeader), 1, arquivoIndex);
    
    // Escremos um número X de registros de índice conforme passado o valor por totalRegs como argumento da função
    for (int i = 0; i < totalRegs; i++) {
        EscreverRegistroIndex(arquivoIndex, &registros[i]);
    }
}

/* Implementação de uma busca binária em um conjunto de registros de índice.
Retorna a POSIÇÃO no vetor de registros de índice*/
int BuscarRegistroIndex(IndexRegistro *registros, int totalRegs, int codEstacao) {
    int esq = 0;
    int dir = totalRegs - 1;

    // -- ALGORITMO DE DIVISÃO E CONQUISTA -- 
    // Diminuimos o espaço de busca para acelerar o encontro do que buscamos 
    while (esq <= dir) {
        int meio = esq + (dir - esq)/2;

        if (registros[meio].codEstacao == codEstacao) {
            return meio; // ENCONTRAMOS
        } else if (registros[meio].codEstacao < codEstacao) {
            esq = meio + 1; // BUSCA NA METADE DA DIREITA
        } else {
            dir = meio - 1; // BUSCA NA METADE DA ESQUERDA
        }
    }

    return -1; // NÃO ENCONTRAMOS
}

/* Insere um registro de índice no vetor em RAM, mantendo a ordenação crescente dos índices para não quebrar a busca binária*/
void InserirRegistroIndex(IndexRegistro **registros, int codEstacao, int RRN, int *totalRegs) {
    // Aumentamos 1 nova posição no vetor para o novo registro de índice
    *registros = realloc(*registros, (*totalRegs + 1) * sizeof(IndexRegistro));
    
    int i = *totalRegs - 1;
    // Percorreremos o vetor da esquerda para direita e enquanto o ID (codEstacao)
    // do registro já presente no vetor for maior do que o ID que queremos inserir,
    // irremos "empurar" ele para a direita, para acharmos um espaço para o novo ID
    while (i >= 0 && (*registros)[i].codEstacao > codEstacao) {
        (*registros)[i + 1] = (*registros)[i];
        i--;
    }

    // Espaço encontrado -> inserimos o novo registro de índice
    (*registros)[i + 1].codEstacao = codEstacao;
    (*registros)[i + 1].RRN = RRN;
    (*totalRegs)++;
}

/* Realiza a remoção lógica no vetor.
Encontra a posição do registro a ser removido e puxa todos os elementos à sua
direita 1 casa para à esquerda.*/
void RemoverRegistroIndex(IndexRegistro **registros, int *totalRegs, int codEstacao) {
    // Realiza a busca bin. para descobrir o índice a ser removido
    int pos = BuscarRegistroIndex(*registros, *totalRegs, codEstacao);
    if (pos == -1) return;

    // Desloca os elementos à direita para a esquerda
    for (int i = pos; i < *totalRegs - 1; i++) {
        (*registros)[i] = (*registros)[i + 1];
    }

    (*totalRegs)--;
    // Ajusta tamanho da memória alocada para evitar memory leaks
    if (*totalRegs == 0){
        free(*registros);
        *registros = NULL;
    } else {
        *registros = realloc(*registros, (*totalRegs) * sizeof(IndexRegistro));
    }
}

/* Função auxiliar que apenas lê a struct do registro de índice*/
void LerRegistroIndex(FILE *arquivoIndex, IndexRegistro *registro) {
    fread(&registro->codEstacao, sizeof(int), 1, arquivoIndex);
    fread(&registro->RRN, sizeof(int), 1, arquivoIndex);
}

/* Função auxiliar que apenas escreve a struct do registro de índice*/
void EscreverRegistroIndex(FILE *arquivoIndex, IndexRegistro *registro) {
    fwrite(&registro->codEstacao, sizeof(int), 1, arquivoIndex);
    fwrite(&registro->RRN, sizeof(int), 1, arquivoIndex);
}

/* Função auxiliar que lê apenas a struct do cabeçalho de índice*/
void LerCabecalhoIndex(FILE *arquivoIndex, IndexHeader *cabecalhoIndex){
    fseek(arquivoIndex, 0, SEEK_SET); // garante que está no ínicio do arquivo
    fread(&cabecalhoIndex->status, sizeof(char), 1, arquivoIndex);
}

/* Função auxiliar que escreve/sobrescreve a struct do cabeçalho de índice */
void EscreverCabecalhoIndex(FILE *arquivoIndex, IndexHeader *cabecalhoIndex){
    fseek(arquivoIndex, 0, SEEK_SET); // garante que está no ínicio do arquivo
    fwrite(&cabecalhoIndex->status, sizeof(char), 1, arquivoIndex);
}

/* Função auxiliar utilizada na hora do quicksort (qsort) para que a biblioteca
do qsort saiba organizar de forma crescente a ordenação*/
int CompararIndexRegistro(const void *A, const void *B){
    IndexRegistro *RegA = (IndexRegistro *)A;
    IndexRegistro *RegB = (IndexRegistro *)B;
    return RegA->codEstacao - RegB->codEstacao;
}