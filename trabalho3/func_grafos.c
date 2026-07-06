#include "funcionalidades.h"


void GerarGrafo(char *arquivoEntrada, char *arquivoIndex){
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex, DIRECIONADO);
    
    if (g == NULL) { MensagemFalhaFuncionalidade(); return; }

    ImprimirGrafo(g);
    LiberarGrafo(g);
}

void Dijkstra(char *arquivoEntrada, char *arquivoIndex, char *campoOrigem, char* valorOrigem, char *campoDestino, char* valorDestino){
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex, NAO_DIRECIONADO);
    if (g == NULL) { MensagemFalhaFuncionalidade(); return; }

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
            
            if (v != -1 && !visitado[v] && atual->distProxEstacao != -1){
                int peso = atual->distProxEstacao;
                if (dist[u] + peso < dist[v]){
                    dist[v] = dist[u] + peso;
                    ant[v] = u;
                } 
                else if (dist[u] + peso == dist[v]){
                    if (ant[v] != -1 && strcmp(g->vertices[u].nomeEstacao, g->vertices[ant[v]].nomeEstacao) < 0){
                        ant[v] = u;
                    }
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


// ==== FUNÇÕES AUXILIARES PARA A ÁRVORE GERADORA MÍNIMA ====

static void DFS_ImprimirAGM(Grafo *g, int u, int *ant, int *chave, int n) {
    for (int v = 0; v < n; v++) {
        // Se 'u' for o pai direto de 'v' na árvore gerada
        if (ant[v] == u) {
            printf("%s, %s, %d\n", g->vertices[u].nomeEstacao, g->vertices[v].nomeEstacao, chave[v]);
            
            // Aprofunda na árvore chamando a DFS para os descendentes de 'v'
            DFS_ImprimirAGM(g, v, ant, chave, n);
        }
    }
}

// Estrutura temporária para armazenar as arestas de forma não-direcionada (simétrica)
typedef struct {
    int u;
    int v;
    int peso;
} ArestaND;

void ArvoreGeradoraMinima(char *arquivoEntrada, char *arquivoIndice, char *campoOrigem, char* valorOrigem){
    // Construir como DIRECIONADO para conseguirmos identificar os sumidouros (folhas puras)
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndice, DIRECIONADO);
    if (g == NULL) { MensagemFalhaFuncionalidade(); return; }

    int idxOrigem = BuscarVertice(g, valorOrigem);
    if (idxOrigem == -1) { MensagemFalhaFuncionalidade(); LiberarGrafo(g); return; }

    int n = g->n_vertices;
    
    // Contar total de arestas para alocação da estrutura auxiliar
    int max_arestas = 0;
    for(int i = 0; i < n; i++) {
        Node *atual = g->vertices[i].arestas;
        while(atual) { max_arestas++; atual = atual->prox; }
    }

    // Estrutura temporária para armazenar as arestas de forma não-direcionada (simétrica)
    ArestaND *arestas = malloc(max_arestas * sizeof(ArestaND));
    int m = 0;
    
    // Coletar arestas ignorando os sumidouros puros
    for (int i = 0; i < n; i++) {
        Node *atual = g->vertices[i].arestas;
        while (atual != NULL) {
            int v = BuscarVertice(g, atual->nomeProxEstacao);
            // Estratégia: ignora arestas cujo destino não possui saídas (lista vazia)
            if (v != -1 && g->vertices[v].arestas != NULL) {
                arestas[m].u = i;
                arestas[m].v = v;
                arestas[m].peso = atual->distProxEstacao;
                m++;
            }
            atual = atual->prox;
        }
    }

    int *chave = malloc(n * sizeof(int));
    int *ant = malloc(n * sizeof(int));
    int *visitado = calloc(n, sizeof(int));

    for (int i = 0; i < n; i++){
        chave[i] = INFINITO;
        ant[i] = -1;
    }

    chave[idxOrigem] = 0;
    visitado[idxOrigem] = 1;
    int count = 1;

    // Algoritmo de Prim sobre a lista de arestas
    while (count < n) {
        int melhorU = -1, melhorV = -1;
        int menorPeso = INFINITO;

        // Varre a lista procurando a melhor aresta de fronteira
        for (int i = 0; i < m; i++) {
            int u = arestas[i].u;
            int v = arestas[i].v;
            int peso = arestas[i].peso;

            // Aresta deve ter exatamente um extremo dentro da árvore (visitado) e outro fora
            if (visitado[u] == visitado[v]) continue;

            int interno = visitado[u] ? u : v;
            int externo = visitado[u] ? v : u;

            // Critérios de desempate estritos: 
            // Menor peso -> Menor ID do vértice interno -> Menor ID do vértice externo
            if (peso < menorPeso ||
               (peso == menorPeso && interno < melhorU) ||
               (peso == menorPeso && interno == melhorU && externo < melhorV)) {
                menorPeso = peso;
                melhorU = interno;
                melhorV = externo;
            }
        }

        // Se não encontrou vizinho válido, o componente conexo esgotou
        if (melhorV == -1) break;

        visitado[melhorV] = 1;
        chave[melhorV] = menorPeso;
        ant[melhorV] = melhorU;
        count++;
    }

    // Imprime via DFS mantendo a lógica de ordem pre-existente
    DFS_ImprimirAGM(g, idxOrigem, ant, chave, n);

    free(arestas);
    free(chave); free(ant); free(visitado);
    LiberarGrafo(g);
}

void ContarCiclos(char *arquivoEntrada, char *arquivoIndex, char *campoOrigem, char *valorOrigem) {
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex, DIRECIONADO);
    if (g == NULL) { MensagemFalhaFuncionalidade(); return; }

    // busca o índice do vértice de origem no grafo
    int origem = BuscarVertice(g, valorOrigem);
    if (origem == -1) { LiberarGrafo(g); MensagemFalhaFuncionalidade(); return; }

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