#include "funcionalidades.h"


/*Constrói o grafo direcionado a partir do arquivo e o imprime na tela. */
void GerarGrafo(char *arquivoEntrada, char *arquivoIndex){
    // Constrói o grafo estritamente como direcionado (apenas ida)
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex, DIRECIONADO);
    
    // Aborta se houve erro na construção ou abertura do arquivo
    if (g == NULL) { MensagemFalhaFuncionalidade(); return; }
    // Imprime as listas de adjacências
    ImprimirGrafo(g);
    LiberarGrafo(g); // Desaloca a memória alocada para o grafo (destrói o grafo)
}

/* Encontra e imprime o caminho mais curto entre duas estações usando o Algoritmo de Dijkstra. */
void Dijkstra(char *arquivoEntrada, char *arquivoIndex, char *campoOrigem, char* valorOrigem, char *campoDestino, char* valorDestino){
    // Constrói o grafo como não-direcionado (ida e volta)
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndex, NAO_DIRECIONADO);
    if (g == NULL) { MensagemFalhaFuncionalidade(); return; }

    // Busca os índices de origem e destino no vetor de vértices
    int idxOrigem = BuscarVertice(g, valorOrigem);
    int idxDestino = BuscarVertice(g, valorDestino);

    // Valida se ambas as estações realmente existem no grafo
    if (idxOrigem == -1 || idxDestino == -1){
        printf("Não existe caminho entre as estações solicitadas.\n");
        LiberarGrafo(g); return;
    }

    int n = g->n_vertices;
    // Vetor de distâncias mínimas conhecidas da origem até cada vértice
    int *dist = malloc(n * sizeof(int));
    // Vetor que guarda o caminho (quem é o pai de quem no percurso)
    int *ant = malloc(n * sizeof(int));
    // Vetor de flags para marcar estações já processadas (0 = não, 1 = sim)
    int *visitado = calloc(n, sizeof(int));

    // Inicializa as distâncias com "infinito" e os antecessores com -1
    for (int i = 0; i < n; i++){ 
        dist[i] = INFINITO;
        ant[i] = -1;
    }

    dist[idxOrigem] = 0; // Distância da origem pra ela mesma é sempre zero

    // Loop principal do Dijkstra para processar todos os vértices
    for (int i = 0; i < n; i++){
        int u = -1;
        int min_dist = INFINITO;

        // Encontra o vértice não visitado com a menor distância acumulada
        for (int j = 0; j < n; j++){
            if (!visitado[j] && dist[j] < min_dist){
                min_dist = dist[j]; 
                u = j; 
            }
        }
        // Se não alcançou ninguém ou a distância é infinita, esgotou as conexões possíveis
        if (u == -1 || dist[u] == INFINITO) break;

        // Marca o vértice atual como processado
        visitado[u] = 1;

        // Se chegou no destino procurado, nós interrompemos o algoritmo como forma de otimizar
        if (u == idxDestino) break;

        // Analisa todos os vizinhos conectados ao vértice atual
        Node *atual = g->vertices[u].arestas;
        while (atual != NULL){
            int v = BuscarVertice(g, atual->nomeProxEstacao);

            // Se o vizinho é válido, não foi visitado e o peso (distância) é conhecido
            if (v != -1 && !visitado[v] && atual->distProxEstacao != -1){
                int peso = atual->distProxEstacao;

                // Atualiza os vetores caso tenha encontrado uma rota mais curta
                if (dist[u] + peso < dist[v]){
                    dist[v] = dist[u] + peso;
                    ant[v] = u;
                } 
                // DESEMPATE: caminhos de pesos iguais priorizam o antecessor de menor ID (ordem alfabética)
                else if (dist[u] + peso == dist[v]){
                    if (ant[v] != -1 && strcmp(g->vertices[u].nomeEstacao, g->vertices[ant[v]].nomeEstacao) < 0){
                        ant[v] = u;
                    }
                }
            }
            atual = atual->prox; // Avança para o próximo vizinho da lista
        }
    }

    // Valida se o destino permaneceu inalcançável após processar todo o grafo
    if (dist[idxDestino] == INFINITO){
        printf("Não existe caminho entre as estações solicitadas.\n");
    } else {
        // Reconstrução do caminho partindo do destino e voltando através dos antecessores
        int *caminho = malloc(n * sizeof(int));
        int tam = 0;
        for (int curr = idxDestino; curr != -1; curr = ant[curr]){
            caminho[tam++] = curr;
        }
        // Exclui a origem na quantidade de estações percorridas (tam - 1)
        printf("Numero de estacoes que serao percorridas: %d\n", tam - 1);
        printf("Distancia que sera percorrida: %d\n", dist[idxDestino]);

        // Imprime o caminho na ordem correta, ou seja, iterando o vetor de trás para frente
        for (int i = tam - 1; i >= 0; i--){
            printf("%s%s", g->vertices[caminho[i]].nomeEstacao, (i > 0) ? ", " : "");
        }
        printf("\n");
        free(caminho);
    }

    free(dist); free(ant); free(visitado); LiberarGrafo(g);
}


// ==== FUNÇÕES AUXILIARES PARA A ÁRVORE GERADORA MÍNIMA ====
/* Função auxiliar que percorre a Árvore Geradora Mínima (AGM) em pré-ordem para impressão. */
static void DFS_ImprimirAGM(Grafo *g, int u, int *ant, int *chave, int n) {
    for (int v = 0; v < n; v++) {
        // Se 'u' for o pai direto de 'v' na árvore gerada
        if (ant[v] == u) {
            // Imprime a aresta obedecendo a formatação: Pai, Filho, Peso (Distância)
            printf("%s, %s, %d\n", g->vertices[u].nomeEstacao, g->vertices[v].nomeEstacao, chave[v]);
            
            // Aprofunda na árvore chamando a DFS para os descendentes de 'v'
            DFS_ImprimirAGM(g, v, ant, chave, n);
        }
    }
}

// Estrutura temporária para armazenar as arestas de forma não-direcionada (simétrica)
typedef struct {
    int u; // Índice do vértice de origem no vetor do grafo (PAI)
    int v; // Índice do vértice de destino no vetor do grafo (FILHO)
    int peso; // Distância/Custo da ligação entre as duas estações
} ArestaND;

void ArvoreGeradoraMinima(char *arquivoEntrada, char *arquivoIndice, char *campoOrigem, char* valorOrigem){
    // Construir como DIRECIONADO para conseguirmos identificar os sumidouros (folhas puras -> estações sem arestas de saída)
    Grafo *g = ConstruirGrafo(arquivoEntrada, arquivoIndice, DIRECIONADO);
    if (g == NULL) { MensagemFalhaFuncionalidade(); return; }

    // Busca o índice numérico da estação de origem informada
    int idxOrigem = BuscarVertice(g, valorOrigem);
    if (idxOrigem == -1) { MensagemFalhaFuncionalidade(); LiberarGrafo(g); return; }

    int n = g->n_vertices;
    
    // Contar total de arestas para alocação da estrutura auxiliar
    int max_arestas = 0;
    for(int i = 0; i < n; i++) {
        Node *atual = g->vertices[i].arestas;
        while(atual){ 
            max_arestas++; atual = atual->prox; 
        }
    }

    // Estrutura temporária para armazenar as arestas de forma não-direcionada (simétrica)
    ArestaND *arestas = malloc(max_arestas * sizeof(ArestaND));
    int m = 0;
    
    // Coleta as arestas filtrando e descartando ativamente estações terminas (sumidouros puros)
    for (int i = 0; i < n; i++) {
        Node *atual = g->vertices[i].arestas;
        while (atual != NULL){
            int v = BuscarVertice(g, atual->nomeProxEstacao);
            // Estratégia: ignora arestas cujo destino não possui saídas (lista vazia)
            if (v != -1 && g->vertices[v].arestas != NULL){
                arestas[m].u = i;
                arestas[m].v = v;
                arestas[m].peso = atual->distProxEstacao;
                m++;
            }
            atual = atual->prox; // Avança o ponteiro da lista encadeada
        }
    }

    // ==== ALOCAÇÃO DOS VETORES DE CONTROLE PARA O Algoritmo de Prim ====
    int *chave = malloc(n * sizeof(int)); // Guarda o menor peso para conectar um vértice a AGM
    int *ant = malloc(n * sizeof(int)); // Guarda o ID de quem conectou aquele vértice a AGM (Pai)
    int *visitado = calloc(n, sizeof(int)); // Flag para sinalizar quais vértices já entraram na AGM

    // Inicializa todos os custos com "infinito" e os antecessores como nulos (-1)
    for (int i = 0; i < n; i++){
        chave[i] = INFINITO;
        ant[i] = -1;
    }

    chave[idxOrigem] = 0;
    visitado[idxOrigem] = 1;
    int count = 1; // Contador de vértices que já pertencem a AGM

    // Algoritmo de Prim sobre a lista de arestas filtrada
    while (count < n) {
        int melhorU = -1, melhorV = -1;
        int menorPeso = INFINITO;

        // Varre a lista procurando a melhor aresta de fronteira (A QUE POSSUIR MENOR CUSTO)
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

        // Se não encontrou nenhum vizinho válido, todos os vértices possíveis já foram incorporados
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