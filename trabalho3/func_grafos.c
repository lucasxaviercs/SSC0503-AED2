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