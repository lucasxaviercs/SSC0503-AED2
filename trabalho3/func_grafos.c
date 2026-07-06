#include "funcionalidades.h"

#define INFINITO INT_MAX


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

void ArvoreGeradoraMinima(char *arquivoEntrada, char *arquivoIndice, char *campoOrigem, char* valorOrigem){
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndice, NAO_DIRECIONADO);
    if (g == NULL) { MensagemFalhaFuncionalidade(); return; }

    // pegamos o índice do vértice de origem no grafo
    int idxOrigem = BuscarVertice(g, valorOrigem);
    if (idxOrigem == -1) { MensagemFalhaFuncionalidade(); LiberarGrafo(g); return; }

    int n = g->n_vertices; 
    int *chave = malloc(n * sizeof(int));    // vetor para armazenar os pesos das arestas que conectam os vértices na AGM
    int *ant = malloc(n * sizeof(int));      // vetor para armazenar os antecessores dos vértices na AGM
    int *visitado = calloc(n, sizeof(int));  // controle do que já foi visitado (já foi inserido na AGM) para saber onde devemos analisar

    // inicializamos os vetores de peso com INFINITO  e antecessores com -1
    for (int i = 0; i < n; i++){
        chave[i] = INFINITO;
        ant[i] = -1;
    }

    // Começamos a partir da origem, então seu peso é 0 e marcamos como visitada
    // Como a AGM começa dela, não colocamos antecessor
    chave[idxOrigem] = 0;
    visitado[idxOrigem] = 1;

    // para cada vértice já visitado, vamos procurar o vizinho com menor peso que ainda não visitamos para conectar a AGM
    for (int i = 0; i < n - 1; i++){
        // variáveis temporárias para armazenar infos dos vizinho com menor peso até o momento
        // ao final, elas terão a informação do vizinho de menor peso que é o que vamos conectar a AGM
        int vizinho = -1;
        int menorPeso = INFINITO;
        char *nomeVizinho = NULL; 

        // percorremos o vetor de visitados para analisar os vizinhos dos vértices que já estão na AGM
        for (int j = 0; j < n; j++){
            if (visitado[j]) {
                // para cada vértice visitado, varremos sua lista de adjacência para encontrar o vizinho de menor peso
                Node *atual = g->vertices[j].arestas;
                while (atual != NULL) {
                    // pegamos o índice do vizinho
                    int idxVizinho = BuscarVertice(g, atual->nomeProxEstacao);
                    // checamos se ele já não foi visitado
                    if (!visitado[idxVizinho]) {
                        // se o peso para conectá-lo for menor que o menor peso encontrado até agora, inserimos ele
                        // caso haja empate, pegamos o vizinho que vem antes em ordem alfabética
                        if (atual->distProxEstacao < menorPeso || (atual->distProxEstacao == menorPeso && strcmp(atual->nomeProxEstacao, nomeVizinho) < 0)) {
                            menorPeso = atual->distProxEstacao;
                            vizinho = idxVizinho;
                            nomeVizinho = atual->nomeProxEstacao;
                            ant[vizinho] = j;
                        }
                    }
                    atual = atual->prox;
                }
            }
        }

        if (vizinho == -1) break;
        
        // ao final, marcamos o vizinho de menor peso como visitado e atualizamos o vetor de pesos
        visitado[vizinho] = 1;
        chave[vizinho] = menorPeso;
    }

    // após todo o algoritmo terminar, chamamos a função para imprimir a AGM no formato correto via busca em profundidade
    DFS_ImprimirAGM(g, idxOrigem, ant, chave, n);
    free(chave); free(ant); free(visitado); LiberarGrafo(g);
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