#pragma once

    #include "header.h"
    #include "registro.h"

    typedef struct CriterioBusca {
        char nomeDoCampo[100];
        char valorBuscado[150];
    } CriterioBusca;

    typedef struct ControleEstacoes {
        char **listaNomesUnicos; // Array para guardar as strings
        int capacidadeMaxima; // Qtd maxima de espaço alocado na memória
        int totalEstacoesUnicas; // Qtd de estações validadas e guardadas
    } ControleEstacoes;

    typedef struct ParEstacao {
        int codEstacaoOrigem;
        int codEstacaoDestino;
    } ParEstacao;

    typedef struct ControlePares {
        ParEstacao *listaParesUnicos; //Array que guarda os trajetos
        int capacidadeMaxima; // Qtd máxima de espaço alocado na memória 
        int totalParesUnicos; // Qtd de trajetos validados e inseridos
    } ControlePares;

    ControleEstacoes *InicializarControleEstacoes();
    void RegistrarEstacaoUnica(ControleEstacoes *controleEstacoes, const char *nomeEstacao);
    void LiberarControleEstacoes(ControleEstacoes *controleEstacoes);

    ControlePares *InicializarControlePares();
    void RegistrarParUnico(ControlePares *controlePares, int codigoEstacaoOrigem, int codigoEstacaoDestino);
    void LiberarControlePares(ControlePares *controlePares);

    void LiberarStringRegistro(Registro *registroDados);

    void LerCriteriosBusca(CriterioBusca *criterios, int qtdCriterios);
    int VerificaCriterioBusca(const Registro *registroDados, const char *nomeDoCampo, const char *valorBuscado);
    void AplicarUpdates(Registro *reg, CriterioBusca *updates, int nroUpdates);

    int BuscaSequencial(FILE *arquivoBIN, int proxRRN, CriterioBusca *criterios, int nroCriterios);
    void RecalcularContadoresCabecalho(FILE *arquivoDadosBIN, Header *cabecalhoDados);

    void ImprimirRegistro(const Registro *registroDados);
    void MensagemErro();
    void MensagemRegistroNaoEncontrado();

    char VerificaEOF(FILE *f);
    void BinarioNaTela(char *arquivo);
    void ScanQuoteString(char *str);

    void IgnorarLinhaZeroCSV(FILE *arquivoCSV);