#include "funcionalidades.h"


void GerarGrafo(char *arquivoEntrada, char *arquivoIndex){
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex);
    
    if (g == NULL){
        MensagemErro();
        return;
    }

    ImprimirGrafo(g);
    LiberarGrafo(g);
}