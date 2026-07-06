#pragma once
    

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <limits.h>
    #include "header.h"
    #include "registro.h"
    #include "index.h"
    #include "utils.h"

    #define INFINITO         INT_MAX
    #define DIRECIONADO      1
    #define NAO_DIRECIONADO  0

    /* Estrutura que representa uma aresta (CONEXÃO) na lista de adjacência.
    Guarda os dados do destino e as linhas que operam neste trajeto.*/
    typedef struct node {
        char *nomeProxEstacao; // Nome da estação de destino
        int distProxEstacao; // Peso da aresta (distância até o destino)
        int n_linhas;   // Quantidade de linhas que fazem esse trajeto
        char **linhas; // Vetor de strings com os nomes das linhas
        struct node *prox; // Ponteiro para a próxima aresta (estrutura de lista encadeada)
    } Node;

    /* Estrutura que representa um vértice (estação) do grafo.*/
    typedef struct {
        char *nomeEstacao; // Nome da estação
        Node *arestas;  // Ponteiro para a cabeça da lista de adjacências (arestas de saída)
    } Vertice;

    /* Estrutura principal do grafo.
    Implementado usando um vetor dinâmico de vértices.*/
    typedef struct {
        int n_vertices; // Número total de vértices inseridos no grafo
        Vertice *vertices; // Vetor dinâmico contendo todos os vértices ordenados alfabeticamente
    } Grafo;

    /* Aloca dinamicamente a estrutura base do Grafo e inicializa seus valores. */
    Grafo* InicializarGrafo();

    /* Lê os registros válidos do arquivo binário e constrói a estrutura completa do Grafo,
    podendo ser gerado como DIRECIONADO (apenas ida) ou NAO_DIRECIONADO (ida e volta). */
    Grafo* ConstruirGrafo(char *arquivoEntrada, char *arquivoIndice, int direcionado);

    /* Realiza uma busca binária no vetor de vértices pelo nome da estação.
    Retorna o índice do vértice no vetor, ou -1 caso não seja encontrado. */
    int BuscarVertice(Grafo *g, const char *nomeEstacao);

    /* Insere um novo vértice no grafo garantindo que o array de vértices 
    permaneça ordenado alfabeticamente para permitir buscas binárias. */
    int InserirVerticeOrdenado(Grafo *g, const char *nomeEstacao);

    /* Insere uma nova aresta na lista de adjacências de um vértice de origem.
    Mantém a lista ordenada pelo nome do destino e, em caso de empate, insere a nova linha. */
    void InserirArestaOrdenada(Grafo *g, int idxOrigem, const char *nomeProxEstacao,
                              int distancia, const char *nomeLinha);

    /* Percorre o array de vértices e suas listas de adjacência imprimindo o grafo
    no formato exigido pela especificação. */                          
    void ImprimirGrafo(Grafo *g);

    /* Desaloca toda a memória utilizada pelo grafo, incluindo o array de vértices,
    as listas de adjacências e todas as strings internas. */
    void LiberarGrafo(Grafo *g);

    /* Percorre o grafo usando Busca em Profundidade (DFS) com backtracking para contar
    quantos ciclos simples começam e terminam na estação de origem. */
    void ChecaCiclos(Grafo *g, int atual, int origem, int *visitado, int *total);