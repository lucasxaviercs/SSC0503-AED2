#include "grafo.h"


/* Aloca o espaço base na memória para o Grafo e zera os atributos. */
Grafo* InicializarGrafo() {
    Grafo *g = malloc(sizeof(Grafo));
    if (g != NULL){
        g->n_vertices = 0;
        g->vertices = NULL;
    }

    return g;
}

// ==== FUNÇÃO AUXILIAR PARA A ConstruirGrafo() ====
/* Busca o nome de uma estação diretamente no arquivo de dados utilizando o RRN associado.
É uma função auxiliar para quando temos apenas o ID (codProxEstacao) mas precisamos da string do nome. */
static char* BuscarNomeNoDisco(FILE *arquivoBIN, int rrnDestino){
    // Salva a posição atual do cursor para poder voltar depois da busca
    long posAtual = ftell(arquivoBIN);

    // Calcula a posição do registro alvo e move o cursor para lá
    long byteoffset = TAM_CABECALHO + (rrnDestino * TAM_REGISTRO);
    fseek(arquivoBIN, byteoffset, SEEK_SET);
    
    Registro regDestino;
    regDestino.nomeEstacao = NULL;
    regDestino.nomeLinha = NULL;
    LerRegistroBIN(arquivoBIN, &regDestino); // Traz o registro para a RAM
    
    // Extrai e aloca dinamicamente apenas o nome da estação encontrada
    char *nomeEncontrado = malloc( strlen(regDestino.nomeEstacao) + 1);
    strcpy(nomeEncontrado, regDestino.nomeEstacao);
    
    LiberarStringRegistro(&regDestino);

    // Restaura o cursor para a posição original de onde a função foi chamada
    fseek(arquivoBIN, posAtual, SEEK_SET);
    
    return nomeEncontrado;
}

/* Constrói o grafo lendo os vértices e, em seguida, as arestas, a partir do arquivo de dados.
Dependendo da flag 'direcionado', cria ou não a aresta de volta para simular um grafo bidirecional. */
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
    
    //  ---- INSERÇÃO DOS VÉRTICES ----
    // Varre o arquivo uma primeira vez apenas para cadastrar as estações principais
    for (int i = 0; i < cabecalho.proxRRN; i++){
        reg.nomeEstacao = NULL;
        reg.nomeLinha = NULL;
        LerRegistroBIN(arquivoBIN, &reg);

        if (reg.removido == '0'){
            InserirVerticeOrdenado(g, reg.nomeEstacao);
            
            if (indiceCarregadoDoArquivo == 0){ // caso ainda NÃO carregamos o arquivo de índice
                // Salvamos o ID (codEstacao e o RRN)
                vetorIndices[totalIndices].codEstacao = reg.codEstacao;
                vetorIndices[totalIndices].RRN = i;
                totalIndices++;
            }
        }
        LiberarStringRegistro(&reg);
    }

    // Ordenamos para permitir a busca binária na etapa da inserção de arestas
    if (indiceCarregadoDoArquivo == 0 && totalIndices > 0){ // Ordena apenas se criamos na hora
        qsort(vetorIndices, totalIndices, sizeof(IndexRegistro), CompararIndexRegistro);
    }

    // Voltamos o cursos para o início dos dados para o loop de inserção de arestas
    fseek(arquivoBIN, TAM_CABECALHO, SEEK_SET);

    //  ---- INSERÇÃO DAS ARESTAS ----
    // Varre o arquivo novamente, agora criando as ligações entre as estações cadastradas
    for (int i = 0; i < cabecalho.proxRRN; i++){
        reg.nomeEstacao = NULL;
        reg.nomeLinha = NULL;
        LerRegistroBIN(arquivoBIN, &reg);

        if (reg.removido == '0'){
            // Pega o índice do vértice de origem no vetor do grafo
            int idxOrigem = BuscarVertice(g, reg.nomeEstacao);

            // PROCESSAMENTO DA ARESTA COMUM (codProxEstacao)
            if (reg.codProxEstacao != -1){
                // Busca no vetor de índices o RRN da próxima estação para descobrir o seu nome
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

            // PROCESSAMENTO DA ARESTA DE INTEGRAÇÃO (codEstIntegra)
            if (reg.codEstIntegra != -1){
                // Busca o RRN, vai no disco e resgata o nome da estação de integração
                int posIndex = BuscarRegistroIndex(vetorIndices, totalIndices, reg.codEstIntegra);
                if (posIndex != -1){
                    char *nomeIntegra = BuscarNomeNoDisco(arquivoBIN, vetorIndices[posIndex].RRN);

                    // Só cria aresta se a estação de integração tiver um nome diferente da estação atual
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

    if (vetorIndices != NULL) free(vetorIndices); // Desaloca o vet. auxiliar de índice, caso ele tenha sido alocado
    
    fclose(arquivoBIN);
    return g;
}

/* Realiza uma busca binária no array de vértices para encontrar a estação desejada. */
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

    return -1; // Não encontrado
}

/* Insere um novo vértice mantendo o array de vértices ordenado alfabeticamente.
Retorna a posição (índice) onde o vértice foi inserido. */
int InserirVerticeOrdenado(Grafo *g, const char *nomeEstacao){
    if (g == NULL || nomeEstacao == NULL) return -1;

    // Se já existe, não insere e retorna a posição atual
    int pos = BuscarVertice(g, nomeEstacao);
    if (pos != -1) return pos;

    // Realoca espaço para +1 vértice
    g->vertices = realloc(g->vertices, (g->n_vertices + 1) * sizeof(Vertice));

    int i = g->n_vertices - 1;

    // Vértices maiores alfabeticamente vão ser deslocados para direita
    while (i>= 0 && strcmp(g->vertices[i].nomeEstacao, nomeEstacao) > 0){
        g->vertices[i + 1] = g->vertices[i];
        i--;
    }

    int novaPos = i + 1; // Posição correta encontrada após o deslocamento

    // Aloca e copia o nome da nova estação
    g->vertices[novaPos].nomeEstacao = malloc( (strlen (nomeEstacao) + 1) * sizeof(char) );
    strcpy(g->vertices[novaPos].nomeEstacao, nomeEstacao);

    g->vertices[novaPos].arestas = NULL; // Inicializa a lista de adjacências como vazia
    g->n_vertices++;

    return novaPos;
}

/* Insere uma nova conexão na lista encadeada de arestas de um vértice.
A lista é ordenada pelo nome da estação de destino. */
void InserirArestaOrdenada(Grafo *g, int idxOrigem, const char *nomeProxEstacao, int distancia, const char *nomeLinha){
    if (g == NULL || idxOrigem == -1 || nomeProxEstacao == NULL || nomeLinha == NULL) return;

    // Obtém o ponteiro direto para o vértice de origem dentro do array do grafo
    Vertice *origem = &g->vertices[idxOrigem];
    // Ponteiro auxiliar para percorrer os nós da lista encadeada a partir da cabeça
    Node *atual = origem->arestas;
    // Guarda o nó anterior
    Node *anterior = NULL;

    // Percorre a lista encadeada até encontrar o ponto de inserção correto (ordem alfabética do destino)
    while (atual != NULL && strcmp(atual->nomeProxEstacao, nomeProxEstacao) < 0){
        anterior = atual;
        atual = atual->prox;
    }

    // Se a aresta para essa estação já existir, apenas atualizamos a distância e as linhas
    if (atual != NULL && strcmp(atual->nomeProxEstacao, nomeProxEstacao) == 0){
        // Atualiza a menor distância possível, ignorando nulos (-1)
        if (distancia != -1) {
            if (atual->distProxEstacao == -1 || distancia < atual->distProxEstacao){
                atual->distProxEstacao = distancia;
            }
        }
        
        // Verifica se a linha específica já está cadastrada para essa aresta
        for (int i = 0; i < atual->n_linhas; i++){
            if (strcmp(atual->linhas[i], nomeLinha) == 0) return; // Linha já existe, sai sem duplicar
        }

        // Se a linha é novidade para esta aresta, aloca e insere de forma ordenada
        atual->linhas = (char**) realloc(atual->linhas, (atual->n_linhas + 1) * sizeof(char*));
        int j = atual->n_linhas - 1;

        // Desloca linhas maiores alfabeticamente para a direita
        while (j >= 0 && strcmp(atual->linhas[j], nomeLinha) > 0){
            atual->linhas[j + 1] = atual->linhas[j];
            j--;
        }
        // Insere a nova linha
        atual->linhas[j + 1] = (char*) malloc((strlen(nomeLinha) + 1) * sizeof(char));
        strcpy(atual->linhas[j + 1], nomeLinha);
        atual->n_linhas++;
        return; // Acabou, aresta base já existia
    }

    // Se chegou aqui, a aresta não existia ainda. Criamos um novo nó.
    Node *novoNode = malloc(sizeof(Node));
    novoNode->nomeProxEstacao = malloc( (strlen(nomeProxEstacao) + 1) * sizeof(char));
    strcpy(novoNode->nomeProxEstacao, nomeProxEstacao);
    
    novoNode->distProxEstacao = distancia;

    // Configura a primeira linha desta nova aresta
    novoNode->n_linhas = 1;
    novoNode->linhas = (char**) malloc(sizeof(char*));
    novoNode->linhas[0] = (char*) malloc( (strlen(nomeLinha) + 1) * sizeof(char));
    strcpy(novoNode->linhas[0], nomeLinha);
    novoNode->prox = atual;

    // Ajusta os ponteiros para inserir o nó na posição correta da lista encadeada
    if (anterior == NULL) origem->arestas = novoNode; // Inserção na cabeça da linked list
    else anterior->prox = novoNode; // Inserção no meio ou no fim da linked list
}

/* Imprime as informações do grafo conforme o formato exigido. */
 void ImprimirGrafo(Grafo *g){
    if (g == NULL || g->n_vertices == 0) return;

    for (int i = 0; i < g->n_vertices; i++){
        Node *atual = g->vertices[i].arestas;
        
        // Se não tiver nenhuma conexão de saída, só ignoramos e não imprimimos
        if (atual == NULL) { continue;}

        // Imprime o vértice de origem
        printf("%s", g->vertices[i].nomeEstacao);

        // Percorre todas as conexões da lista de adjacências
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

 /* Limpa todas as alocações dinâmicas contidas no grafo para evitar vazamento de memória. */
 void LiberarGrafo(Grafo *g){
    if (g == NULL) return;

    // Para cada vértice
    for (int i = 0; i < g->n_vertices; i++){
        // Libera string do vértice principal
        free(g->vertices[i].nomeEstacao);

        
        Node *atual = g->vertices[i].arestas;
        // Varre a lista de adjacências liberando nó a nó
        while (atual != NULL) {
            Node *prox = atual->prox;
            
            free(atual->nomeProxEstacao);
            
            // Libera cada linha vinculada a essa aresta
            for (int k = 0; k < atual->n_linhas; k++){
                free(atual->linhas[k]);
            }
            free(atual->linhas); // Libera o array de ponteiros de linhas
            free(atual); // Libera o nó da aresta em si
            
            atual = prox;
        }
    }
    
    free(g->vertices); // Libera o array de vértices principal
    free(g); // Libera a struct do grafo
 }

 /* Função recursiva de Busca em Profundidade (DFS) que contabiliza os ciclos simples.
Um ciclo simples é aquele que inicia e termina na mesma origem, sem repetir nós pelo meio. */
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
           
            // BACKTRACKING: desmarca o nó após a recursão voltar, 
            // isso permite ele seja usado em outros possíveis caminhos que formem novos ciclos
            visitado[v] = 0;          

        }
        // CASO 3: v já foi visitado, portanto não precisamos fazer nada
        a = a->prox; // próxima aresta do vértice atual
    }
}