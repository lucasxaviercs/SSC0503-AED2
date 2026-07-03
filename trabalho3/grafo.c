#include "grafo.h"


static char* BuscarNomeNoDisco(FILE *arquivoBIN, int rrnDestino){
    long posAtual = ftell(arquivoBIN);
    long byteoffset = TAM_CABECALHO + (rrnDestino * TAM_REGISTRO);
    fseek(arquivoBIN, byteoffset, SEEK_SET);
    
    Registro regDestino;
    regDestino.nomeEstacao = NULL;
    regDestino.nomeLinha = NULL;
    LerRegistroBIN(arquivoBIN, &regDestino);
    
    char *nomeEncontrado = malloc( strlen(regDestino.nomeEstacao) + 1);
    strcpy(nomeEncontrado, regDestino.nomeEstacao);
    
    LiberarStringRegistro(&regDestino);
    fseek(arquivoBIN, posAtual, SEEK_SET);
    
    return nomeEncontrado;
}

Grafo* InicializarGrafo() {
    Grafo *g = malloc(sizeof(Grafo));
    if (g != NULL){
        g->n_vertices = 0;
        g->vertices = NULL;
    }

    return g;
}

Grafo* ConstruirGrafo(char *arquivoEntrada, char *arquivoIndice, int direcionado){
    FILE *arquivoBIN = fopen(arquivoEntrada, "rb");
    if (arquivoBIN == NULL) return NULL;

    Header cabecalho;
    LerCabecalhoBIN(arquivoBIN, &cabecalho);
    if (cabecalho.status == '0' || cabecalho.proxRRN == 0){
        fclose(arquivoBIN); return NULL;
    }

    Grafo *g = InicializarGrafo();
    IndexRegistro *vetorIndices = NULL;
    int totalIndices = 0;
    int indiceCarregadoDoArquivo = 0;

    // Com o arquivo de índice, carregamos os índices para memória primária
    if (arquivoIndice != NULL){
        FILE *arquivoIndexBIN = fopen(arquivoIndice, "rb");
        if (arquivoIndexBIN != NULL){
            CarregarIndex(arquivoIndexBIN, &vetorIndices, &totalIndices, &cabecalho);
            fclose(arquivoIndexBIN);
            indiceCarregadoDoArquivo = 1;
        }
    }

    // Caso o índice NÃO foi carregado, iremos montá-lo
    if (indiceCarregadoDoArquivo == 0){
        vetorIndices = malloc(cabecalho.proxRRN * sizeof(IndexRegistro));
    }

    fseek(arquivoBIN, TAM_CABECALHO, SEEK_SET);
    Registro reg;
    
    // Inserção dos vértices
    for (int i = 0; i < cabecalho.proxRRN; i++){
        reg.nomeEstacao = NULL;
        reg.nomeLinha = NULL;
        LerRegistroBIN(arquivoBIN, &reg);

        if (reg.removido == '0'){
            InserirVerticeOrdenado(g, reg.nomeEstacao);
            
            if (indiceCarregadoDoArquivo == 0){ // caso ainda NÃO carregamos o arquivo de índice
                vetorIndices[totalIndices].codEstacao = reg.codEstacao;
                vetorIndices[totalIndices].RRN = i;
                totalIndices++;
            }
        }
        LiberarStringRegistro(&reg);
    }

    if (indiceCarregadoDoArquivo == 0 && totalIndices > 0){ // Ordena apenas se criamos na hora
        qsort(vetorIndices, totalIndices, sizeof(IndexRegistro), CompararIndexRegistro);
    }

    fseek(arquivoBIN, TAM_CABECALHO, SEEK_SET);

    // Inserção das arestas
    for (int i = 0; i < cabecalho.proxRRN; i++){
        reg.nomeEstacao = NULL;
        reg.nomeLinha = NULL;
        LerRegistroBIN(arquivoBIN, &reg);

        if (reg.removido == '0'){
            int idxOrigem = BuscarVertice(g, reg.nomeEstacao);

            // Aresta comum
            if (reg.codProxEstacao != -1){
                int posIndex = BuscarRegistroIndex(vetorIndices, totalIndices, reg.codProxEstacao);
                if (posIndex != -1){
                    char *nomeProx = BuscarNomeNoDisco(arquivoBIN, vetorIndices[posIndex].RRN);
                    
                    // --- IDA ---
                    // Insere o caminho de ida do registro original (Origem -> Destino)
                    InserirArestaOrdenada(g, idxOrigem, nomeProx, reg.distProxEstacao, reg.nomeLinha);
                    
                    // --- VOLTA ---
                    // Insere o caminho inverso (Destino -> Origem) apenas se exigido
                    if (direcionado == NAO_DIRECIONADO) {
                        int idxDestino = BuscarVertice(g, nomeProx);
                        if (idxDestino != -1){
                            InserirArestaOrdenada(g, idxDestino, g->vertices[idxOrigem].nomeEstacao, reg.distProxEstacao, reg.nomeLinha);
                        }
                    }
                    free(nomeProx);
                }
            }

            // Aresta de integração
            if (reg.codEstIntegra != -1){
                int posIndex = BuscarRegistroIndex(vetorIndices, totalIndices, reg.codEstIntegra);
                if (posIndex != -1){
                    char *nomeIntegra = BuscarNomeNoDisco(arquivoBIN, vetorIndices[posIndex].RRN);
                    if (strcmp(reg.nomeEstacao, nomeIntegra) != 0){
                        
                        // --- IDA ---
                        // Insere o caminho de integração do registro original (Origem -> Destino)
                        InserirArestaOrdenada(g, idxOrigem, nomeIntegra, 0, "Integração");
                        
                        // --- VOLTA ---
                        // Insere o caminho inverso da integração (Destino -> Origem) apenas se exigido
                        if (direcionado == NAO_DIRECIONADO){
                            int idxIntegra = BuscarVertice(g, nomeIntegra);
                            if (idxIntegra != -1){
                                InserirArestaOrdenada(g, idxIntegra, g->vertices[idxOrigem].nomeEstacao, 0, "Integração");
                            }
                        }
                    }
                    free(nomeIntegra);
                }
            }
        }
        LiberarStringRegistro(&reg);
    }

    if (vetorIndices != NULL) free(vetorIndices);
    
    fclose(arquivoBIN);
    return g;
}

// Busca binaria 
int BuscarVertice(Grafo *g, const char *nomeEstacao){
    if (g == NULL || g->n_vertices == 0 || nomeEstacao == NULL) return -1;

    int esq = 0;
    int dir = g->n_vertices - 1;

    while (esq <= dir){
        int meio = esq + (dir - esq) / 2;
        int comp = strcmp(g->vertices[meio].nomeEstacao, nomeEstacao);

        if (comp == 0){
            return meio;
        }
        else if (comp < 0) {
            esq = meio + 1;
        }
        else {
            dir = meio - 1;
        }
    }

    return -1;
}

int InserirVerticeOrdenado(Grafo *g, const char *nomeEstacao){
    if (g == NULL || nomeEstacao == NULL) return -1;

    int pos = BuscarVertice(g, nomeEstacao);
    if (pos != -1) return pos;

    g->vertices = realloc(g->vertices, (g->n_vertices + 1) * sizeof(Vertice));

    int i = g->n_vertices - 1;

    // Vértices maiores alfabeticamente vão ser deslocados para direita
    while (i>= 0 && strcmp(g->vertices[i].nomeEstacao, nomeEstacao) > 0){
        g->vertices[i + 1] = g->vertices[i];
        i--;
    }

    int novaPos = i + 1;

    g->vertices[novaPos].nomeEstacao = malloc( (strlen (nomeEstacao) + 1) * sizeof(char) );
    strcpy(g->vertices[novaPos].nomeEstacao, nomeEstacao);

    g->vertices[novaPos].arestas = NULL;
    g->n_vertices++;

    return novaPos;
}

void InserirArestaOrdenada(Grafo *g, int idxOrigem, const char *nomeProxEstacao, int distancia, const char *nomeLinha){
    if (g == NULL || idxOrigem == -1 || nomeProxEstacao == NULL || nomeLinha == NULL) return;

    Vertice *origem = &g->vertices[idxOrigem];
    Node *atual = origem->arestas;
    Node *anterior = NULL;

    while (atual != NULL && strcmp(atual->nomeProxEstacao, nomeProxEstacao) < 0){
        anterior = atual;
        atual = atual->prox;
    }

    if (atual != NULL && strcmp(atual->nomeProxEstacao, nomeProxEstacao) == 0){
        // PROTEÇÃO CONTRA O NULO (-1)
        if (distancia != -1) {
            if (atual->distProxEstacao == -1 || distancia < atual->distProxEstacao){
                atual->distProxEstacao = distancia;
            }
        }
        
        for (int i = 0; i < atual->n_linhas; i++){
            if (strcmp(atual->linhas[i], nomeLinha) == 0) return;
        }

        atual->linhas = (char**) realloc(atual->linhas, (atual->n_linhas + 1) * sizeof(char*));
        int j = atual->n_linhas - 1;
        while (j >= 0 && strcmp(atual->linhas[j], nomeLinha) > 0){
            atual->linhas[j + 1] = atual->linhas[j];
            j--;
        }
        atual->linhas[j + 1] = (char*) malloc((strlen(nomeLinha) + 1) * sizeof(char));
        strcpy(atual->linhas[j + 1], nomeLinha);
        atual->n_linhas++;
        return;
    }

    Node *novoNode = malloc(sizeof(Node));
    novoNode->nomeProxEstacao = malloc( (strlen(nomeProxEstacao) + 1) * sizeof(char));
    strcpy(novoNode->nomeProxEstacao, nomeProxEstacao);
    novoNode->distProxEstacao = distancia;
    novoNode->n_linhas = 1;
    novoNode->linhas = (char**) malloc(sizeof(char*));
    novoNode->linhas[0] = (char*) malloc( (strlen(nomeLinha) + 1) * sizeof(char));
    strcpy(novoNode->linhas[0], nomeLinha);
    novoNode->prox = atual;

    if (anterior == NULL) origem->arestas = novoNode;
    else anterior->prox = novoNode;
}

 void ImprimirGrafo(Grafo *g){
    if (g == NULL || g->n_vertices == 0) return;

    for (int i = 0; i < g->n_vertices; i++){
        Node *atual = g->vertices[i].arestas;
        
        // Se não tiver nenhuma conexão de saída, só ignoramos e não imprimimos
        if (atual == NULL) { continue;}

        // Imprime o vértice de origem
        printf("%s", g->vertices[i].nomeEstacao);

        // Percorre a lista de adjacências
        while (atual != NULL){
            // Imprime o destino e a distância
            printf(", %s, %d", atual->nomeProxEstacao, atual->distProxEstacao);
            
            // Imprime todas as linhas associadas a essa aresta
            for (int k = 0; k < atual->n_linhas; k++) {
                printf(", %s", atual->linhas[k]);
            }
            
            atual = atual->prox;
        }
        printf("\n");
    }
 }

 void LiberarGrafo(Grafo *g){
    if (g == NULL) return;

    for (int i = 0; i < g->n_vertices; i++){
        // Libera string do vértice principal
        free(g->vertices[i].nomeEstacao);

        // Varre e libera a lista encadeada
        Node *atual = g->vertices[i].arestas;
        while (atual != NULL) {
            Node *prox = atual->prox;
            
            free(atual->nomeProxEstacao);
            
            for (int k = 0; k < atual->n_linhas; k++){
                free(atual->linhas[k]);
            }
            free(atual->linhas);
            free(atual);
            
            atual = prox;
        }
    }
    
    free(g->vertices);
    free(g);
 }

 void ChecaCiclos(Grafo *g, int atual, int origem, int *visitado, int *total) {
    // Percorre todas as arestas que saem do vértice atual
    Node *a = g->vertices[atual].arestas;
    while (a != NULL) {
        int v = BuscarVertice(g, a->nomeProxEstacao);

        // CASO 1: o vértice vizinho do atual é a própria origem, logo encontramos um ciclo
        if (v == origem) {
            (*total)++;

        } else if (v != -1 && !visitado[v]) {
            // CASO 2: o vértice vizinho do atual existe e ainda não foi visitado
            visitado[v] = 1;          // marcamos v como visitado agora faz parte do caminho atual
            ChecaCiclos(g, v, origem, visitado, total); // chama ChecaCiclos novamente passando v como parâmetro para ver o restante do caminho 
            visitado[v] = 0;          // 

        }
        // CASO 3: v já foi visitado, portanto não precisamos fazer nada
        a = a->prox; // próxima aresta do vértice atual
    }
}