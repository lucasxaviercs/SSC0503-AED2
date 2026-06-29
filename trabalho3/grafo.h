#pragma once

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

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

    