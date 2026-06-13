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

    // Inicializa a estrutura dinâmica para armazenar e contabilizar nomes de estações
    ControleEstacoes *InicializarControleEstacoes();
    // Adiciona o nome de uma estação à lista de controle caso ela ainda não exista lá
    void RegistrarEstacaoUnica(ControleEstacoes *controleEstacoes, const char *nomeEstacao);
    // Desaloca a lista de controle e todas as strings de estações da memória
    void LiberarControleEstacoes(ControleEstacoes *controleEstacoes);

    // Inicializa a estrutura dinâmica para armazenar e contabilizar as ligações (pares)
    ControlePares *InicializarControlePares();
    // Adiciona um trajeto à lista de pares (origem, destino) caso seja uma ligação inédita, ou seja, ainda não contabilizada
    void RegistrarParUnico(ControlePares *controlePares, int codigoEstacaoOrigem, int codigoEstacaoDestino);
    // Desaloca a estrutura de controle de pares da memória
    void LiberarControlePares(ControlePares *controlePares);

    // Desaloca as strings 'nomeEstacao' e 'nomeLinha' da struct
    void LiberarStringRegistro(Registro *registroDados);

    // Lê os campos e valores de busca, guardando em um vetor de critérios
    void LerCriteriosBusca(CriterioBusca *criterios, int qtdCriterios);
    // Retorna 1 se o campo do registro corresponde ao valor buscado, ou 0 caso contrário.
    int VerificaCriterioBusca(const Registro *registroDados, const char *nomeDoCampo, const char *valorBuscado);
    // Modifica os campos do registro em RAM com os novos valores
    void AplicarUpdates(Registro *reg, CriterioBusca *updates, int nroUpdates);

    // Varre o arquivo sequencialmente filtrando e imprimindo registros que atendam a todos os critério
    int BuscaSequencial(FILE *arquivoBIN, int proxRRN, CriterioBusca *criterios, int nroCriterios);
    // Varre o arquivo ignorando removidos para recontar do zero a quantidade real de estações e pares únicos
    void RecalcularContadoresCabecalho(FILE *arquivoDadosBIN, Header *cabecalhoDados);

    // Imprime na tela as informações do registro, formatando dados vazios/nulos como "NULO"
    void ImprimirRegistro(const Registro *registroDados);
    // Imprime a mensagem de erro padrão
    void MensagemErro();
    // Imprime a mensagem de registro não encontrado
    void MensagemRegistroNaoEncontrado();

    char VerificaEOF(FILE *f);
    void BinarioNaTela(char *arquivo);
    void ScanQuoteString(char *str);

    // Lê e descarta a primeira linha do CSV (nomes das colunas) para não colocar nos arquivos
    void IgnorarLinhaZeroCSV(FILE *arquivoCSV);