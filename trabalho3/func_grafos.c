#include "funcionalidades.h"

#define INFINITO INT_MAX


void GerarGrafo(char *arquivoEntrada, char *arquivoIndex){
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex);
    
    if (g == NULL){
        MensagemErro();
        return;
    }

    ImprimirGrafo(g);
    LiberarGrafo(g);
}

void Dijkstra(char *arquivoEntrada, char *arquivoIndex, char *campoOrigem, char* valorOrigem, char *campoDestino, char* valorDestino){
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex);
    if (g == NULL) { MensagemErro(); return; }

    int idxOrigem = BuscarVertice(g, valorOrigem);
    int idxDestino = BuscarVertice(g, valorDestino);

    if (idxOrigem == -1 || idxDestino == -1) {
        printf("Não existe caminho entre as estações solicitadas.\n");
        LiberarGrafo(g); return;
    }

    int n = g->n_vertices;
    int *dist = malloc(n * sizeof(int));
    int *ant = malloc(n * sizeof(int));
    int *visitado = calloc(n, sizeof(int));

    for (int i = 0; i < n; i++){ 
        dist[i] = INFINITO;
        ant[i] = -1;
    }
    dist[idxOrigem] = 0;

    for (int i = 0; i < n; i++){
        int u = -1;
        int min_dist = INFINITO;

        for (int j = 0; j < n; j++){
            if (!visitado[j] && dist[j] < min_dist){
                min_dist = dist[j]; 
                u = j; 
            }
        }

        if (u == -1 || dist[u] == INFINITO) break;
        visitado[u] = 1;
        if (u == idxDestino) break;

        Node *atual = g->vertices[u].arestas;
        while (atual != NULL){
            int v = BuscarVertice(g, atual->nomeProxEstacao);
            if (v != -1 && !visitado[v]){
                int peso = atual->distProxEstacao;
                if (dist[u] + peso < dist[v]){
                    dist[v] = dist[u] + peso;
                    ant[v] = u;
                }
            }
            atual = atual->prox;
        }
    }

    if (dist[idxDestino] == INFINITO) {
        printf("Não existe caminho entre as estações solicitadas.\n");
    } else {
        int *caminho = malloc(n * sizeof(int));
        int tam = 0;
        for (int curr = idxDestino; curr != -1; curr = ant[curr]){
            caminho[tam++] = curr;
        }

        printf("Numero de estacoes que serao percorridas: %d\n", tam - 1);
        printf("Distancia que sera percorrida: %d\n", dist[idxDestino]);
        for (int i = tam - 1; i >= 0; i--) {
            printf("%s%s", g->vertices[caminho[i]].nomeEstacao, (i > 0) ? ", " : "");
        }
        printf("\n");
        free(caminho);
    }

    free(dist); free(ant); free(visitado); LiberarGrafo(g);
}

void ContarCiclos(char *arquivoEntrada, char *arquivoIndex, char *campoOrigem, char *valorOrigem) {
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex);
    if (g == NULL) { printf("Falha na execução da funcionalidade.\n"); return; }

    // busca o índice do vértice de origem no grafo
    int origem = BuscarVertice(g, valorOrigem);
    if (origem == -1) { LiberarGrafo(g); printf("Falha na execução da funcionalidade.\n"); return; }

    // cria um array de flags para marcar os vértices que já visitamos
    // calloc já inicializa com 0, ou seja, não visitado
    int *visitado = calloc(g->n_vertices, sizeof(int));
    int total = 0; // variável para armazenar o número de ciclos

    visitado[origem] = 1;

    ChecaCiclos(g, origem, origem, visitado, &total);

    // printa -1 se não há ciclos e o total de ciclos caso contrário
    printf("Quantidade de ciclos: %d\n", total == 0 ? -1 : total);

    free(visitado);
    LiberarGrafo(g);
}