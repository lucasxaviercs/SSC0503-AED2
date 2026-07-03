#pragma once
    

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <limits.h>
    #include "header.h"
    #include "registro.h"
    #include "index.h"
    #include "utils.h"

    #define DIRECIONADO      1
    #define NAO_DIRECIONADO  0

    typedef struct node {
        char *nomeProxEstacao;
        int distProxEstacao;
        int n_linhas;
        char **linhas;
        struct node *prox;
    } Node;

    typedef struct {
        char *nomeEstacao;
        Node *arestas;
    } Vertice;

    typedef struct {
        int n_vertices;
        Vertice *vertices;
    } Grafo;

    
    Grafo* InicializarGrafo();

    Grafo* ConstruirGrafo(char *arquivoEntrada, char *arquivoIndice, int direcionado);

    int BuscarVertice(Grafo *g, const char *nomeEstacao);

    int InserirVerticeOrdenado(Grafo *g, const char *nomeEstacao);

    void InserirArestaOrdenada(Grafo *g, int idxOrigem, const char *nomeProxEstacao,
                              int distancia, const char *nomeLinha);

    void ImprimirGrafo(Grafo *g);

    void LiberarGrafo(Grafo *g);

    void ChecaCiclos(Grafo *g, int atual, int origem, int *visitado, int *total);