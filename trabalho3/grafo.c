#include "grafo.h"


Grafo* InicializarGrafo() {
    Grafo *g = malloc(sizeof(Grafo));
    if (g != NULL){
        g->n_vertices = 0;
        g->vertices = NULL;
    }

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

    if (g == NULL || nomeProxEstacao == NULL || nomeLinha == NULL) return;

    Vertice *origem = &g->vertices[idxOrigem];
    Node *atual = origem->arestas;
    Node *anterior = NULL;

    // Busca na linked list a posição alfabética correta ou um nó já existente
    while (atual != NULL && strcmp(atual->nomeProxEstacao, nomeProxEstacao) < 0){
        anterior = atual;
        atual = atual->prox;
    }

    // SITUAÇÃO 1: nó de destino já existe (mesmo par de estação, linha diferente)
    if (atual != NULL && strcmp(atual->nomeProxEstacao, nomeProxEstacao) == 0){
        // Evitar inserir linhas duplicadas na msm conexão
        for (int i = 0; i < atual->n_linhas; i++){
            if (strcmp(atual->linhas[i], nomeLinha) == 0) return;
        }

        // Reajustamos a matriz de strings (realocando) para ter a nova linha
        atual->linhas = (char**) realloc(atual->linhas, (atual->n_linhas + 1) * sizeof(char*));

        int j = atual->n_linhas - 1;
        // Deslocamento para direita, para manter a ordem alfabética
        while (j >= 0 && strcmp(atual->linhas[j], nomeLinha) > 0){
            atual->linhas[j + 1] = atual->linhas[j];
            j--;
        }

        atual->linhas[j + 1] = (char*) malloc((strlen(nomeLinha) + 1) * sizeof(char));
        strcpy(atual->linhas[j + 1], nomeLinha);
        atual->n_linhas++;

        return;
    }

    // SITUAÇÃO 2: nó de destino aparece pela primeira vez, então aloca-se um novo nó
    Node *novoNode = malloc(sizeof(Node));
    novoNode->nomeProxEstacao = malloc( ( strlen(nomeProxEstacao ) + 1) * sizeof(char));
    strcpy(novoNode->nomeProxEstacao, nomeProxEstacao);

    novoNode->distProxEstacao = distancia;
    novoNode->n_linhas = 1;
    novoNode->linhas = (char**) malloc(sizeof(char*));
    novoNode->linhas[0] = (char*) malloc( ( strlen(nomeLinha) + 1) * sizeof(char));
    strcpy(novoNode->linhas[0], nomeLinha);

    novoNode->prox = atual;

    if (anterior == NULL){
        origem->arestas = novoNode; // Inserção no início da lista
    } else {
        anterior->prox = novoNode; // Inserção no meio/fim
    }
}

 void ImprimirGrafo(Grafo *g){
    if (g == NULL || g->n_vertices == 0) return;

    for (int i = 0; i < g->n_vertices; i++){
        // Imprime o vértice atual (origem)
        printf("%s", g->vertices[i].nomeEstacao);
        
        Node *atual = g->vertices[i].arestas;
        
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