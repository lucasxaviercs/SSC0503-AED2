#include "index.h"

void CarregarIndex(FILE *arquivoIndex, IndexRegistro **registros, int *totalRegs) {
    if (arquivoIndex == NULL) return;

    IndexHeader cabecalhoIndex;
    fseek(arquivoIndex, 0, SEEK_SET);
    fread(&cabecalhoIndex, sizeof(IndexHeader), 1, arquivoIndex);

    if (cabecalhoIndex.status == '0') {
        mensagemErro();
        return;
    }

    *totalRegs = cabecalhoDados->nroEstacoes;
    *registros = (IndexRegistro *)malloc(*totalRegs * sizeof(IndexRegistro));

    for (int i = 0; i < *totalRegs; i++) {
        LerRegistroIndex(arquivoIndex, &(*registros)[i]);
    }
}
   

void ReescritaIndex(FILE *arquivoIndex, IndexRegistro *registros, int totalRegs) {
    if (arquivoIndex == NULL) return;

    fseek(arquivoIndex, 0, SEEK_SET);
    IndexHeader cabecalhoIndex;
    cabecalhoIndex.status = '1'; // marca como consistente
    
    fwrite(&cabecalhoIndex, sizeof(IndexHeader), 1, arquivoIndex);
    for (int i = 0; i < totalRegs; i++) {
        EscreverRegistroIndex(arquivoIndex, &registros[i]);
    }
}

int BuscarRegistroIndex(IndexRegistro *registros, int totalRegs, int codEstacao) {
    int esq = 0;
    int dir = totalRegs - 1;

    while (esq <= dir) {
        int meio = esq + (dir - esq)/2;

        if (registros[meio].codEstacao == codEstacao) {
            return registros[meio].RRN;
        } else if (registros[meio].codEstacao < codEstacao) {
            esq = meio + 1;
        } else {
            dir = meio - 1;
        }
    }

    return -1;
}

void InserirRegistroIndex(IndexRegistro **registros, int codEstacao, int RRN, int *totalRegs) {
    *registros = realloc(*registros, (*totalRegs + 1) * sizeof(IndexRegistro));
    
    int i = *totalRegs - 1;
    while (i >= 0 && (*registros)[i].codEstacao > codEstacao) {
        (*registros)[i + 1] = (*registros)[i];
        i--;
    }

    (*registros)[i + 1].codEstacao = codEstacao;
    (*registros)[i + 1].RRN = RRN;
    (*totalRegs)++;
}

void RemoverRegistroIndex(IndexRegistro **registros, int *totalRegs, int codEstacao) {
    int pos = BuscarRegistroIndex(registros, *totalRegs, codEstacao);
    if (pos == -1) return;

    for (int i = pos; i < *totalRegs - 1; i++) {
        (*registros)[i] = (*registros)[i + 1];
    }

    (*totalRegs)--;
    *registros = realloc(*registros, (*totalRegs) * sizeof(IndexRegistro));
}

void LerRegistroIndex(FILE *arquivoIndex, IndexRegistro *registro) {
    fread(&registro->codEstacao, sizeof(int), 1, arquivoIndex);
    fread(&registro->RRN, sizeof(int), 1, arquivoIndex);
}

void EscreverRegistroIndex(FILE *arquivoIndex, IndexRegistro *registro) {
    fwrite(&registro->codEstacao, sizeof(int), 1, arquivoIndex);
    fwrite(&registro->RRN, sizeof(int), 1, arquivoIndex);
}

void LerCabecalhoIndex(FILE *arquivoIndex, IndexHeader *cabecalhoIndex){
    fseek(arquivoIndex, 0, SEEK_SET);
    fread(&cabecalhoIndex->status, sizeof(char), 1, arquivoIndex);
}

int CompararIndexRegistro(const void *A, const void *B){
    IndexRegistro *RegA = (IndexRegistro *)A;
    IndexRegistro *RegB = (IndexRegistro *)B;
    return RegA->codEstacao - RegB->codEstacao;
}