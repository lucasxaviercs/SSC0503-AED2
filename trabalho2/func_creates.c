#include "funcionalidades.h"


/*Lê os registros de um arquivo CSV e constrói a "tabela" no formato de um arquivo binário.
Tendo todo o controle de inicializar, contabilizar, atualizar os dados do cabeçalho e dos registros
para realizar a gravação no arquivo binário*/
void CreateTable(char *arquivoEntrada, char *arquivoSaida){
    // Abrindo arquivo CSV para leitura
    FILE *arquivoCSV = fopen(arquivoEntrada, "r");
    // Abortando funcionalidade caso ocorra erro na abertura do arquivo CSV
    if(arquivoCSV == NULL){
        MensagemErro();
        return;
    }

    // Abrindo arquivo BIN para escrita binária
    FILE *arquivoBIN = fopen(arquivoSaida, "wb");
    // Abortando funcionalidade caso ocorra erro na abertura do arquivo BIN
    if(arquivoBIN == NULL){
        MensagemErro();
        free(arquivoCSV);
        return;
    }

    // INICIALIZAÇÃO DE ESTRUTURAS NA MEMÓRIA PRIMÁRIA
    // inicialiamos os valores do cabeçalho
    Header *cabecalho = InicializarCabecalho();

    // inicializamos estruturas auxiliares que nos vão ajudar a determinar nroEstacoes e nroParesEstacoes
    ControleEstacoes *controleEstacoes = InicializarControleEstacoes();
    ControlePares *controlePares = InicializarControlePares();

    // "RESERVAMOS" OS PRIMEIROS 17 BYTES (0-16) DO ARQUIVO BINÁRIO PARA O CABEÇALHO
    fseek(arquivoBIN, TAM_CABECALHO, SEEK_SET);

    // DESCARTAMOS A LINHA ZERO DO CSV QUE CONTÉM APENAS AS NOMENCLATURAS DAS COLUNAS
    IgnorarLinhaZeroCSV(arquivoCSV);

    Registro registroDados; // Utilizaremos como registro auxiliar para não trazer tudo do disco deuma vez pra memória primária 

    // LOOP PARA PROCESSAR OS DADOS DO ARQUIVO DE ENTRADA
    // vamos obter os registros de dados do arquivo CSV e escrever eles no arquivo binário
    while(VerificaEOF(arquivoCSV)){

        LerRegistroCSV(arquivoCSV, &registroDados);
        if(registroDados.removido == '1'){ // se o registro está logicamente removido
            LiberarStringRegistro(&registroDados);
            break;
        }
        
        // Registra os nomes únicos e pares únicos das estações para contagem do nroEstacoes e nroParesEstacao
        RegistrarEstacaoUnica(controleEstacoes, registroDados.nomeEstacao);
        RegistrarParUnico(controlePares, registroDados.codEstacao, registroDados.codProxEstacao);

        EscreverRegistroBIN(arquivoBIN, &registroDados);

        cabecalho->proxRRN++;

        LiberarStringRegistro(&registroDados);
    }

    //ATUALIZACAO FINAL DO CABECALHO
    cabecalho->nroEstacoes = controleEstacoes->totalEstacoesUnicas;
    cabecalho->nroParesEstacao = controlePares->totalParesUnicos;
    cabecalho->status = '1'; // Finalizando o uso do arquivo

    EscreverCabecalhoBIN(arquivoBIN, cabecalho); // O fseek dentro da função nos garante que irá sobrescrever os primeiro 17 bytes que é refente ao próprio cabeçalho

    //REALIZANDO OS ÚLTIMOS DESALOCAMENTO DE MEMÓRIA E FECHANDO OS ARQUIVOS
    LiberarControleEstacoes(controleEstacoes);
    LiberarControlePares(controlePares);

    free(cabecalho);
    cabecalho = NULL;

    fclose(arquivoCSV);
    fclose(arquivoBIN);

    // Como exigido no PDF do trabalho
    BinarioNaTela(arquivoSaida);

}

/*Cria o arquivo de índice a partir do arquivo de dados, utilizando o codEstacao
como chave de busca e o RRN como referência, ordenando em ordem crescente os índice
em memória primária antes de salvar em disco */
void CreateIndex(char *arquivoDados, char *arquivoIndex){
    // Abre o arquivo de dados para leitura, se deu erro aborta
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb");
    if (arquivoDadosBIN == NULL) {
        MensagemErro();
        return;
    }

    // Verifica a consistência do arquivo de dados, se tiver inconsistente aborta
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0'){
        MensagemErro();
        fclose(arquivoDadosBIN);
        return;
    }

    // Abre o arquivo de índice para escrita, se deu erro aborta
    FILE *arquivoIndexBIN = fopen(arquivoIndex, "wb+");
    if (arquivoIndexBIN == NULL) {
        MensagemErro();
        fclose(arquivoDadosBIN);
        return;
    }

    // Marca como inconsistente durante o processo de criação do índice
    IndexHeader cabecalhoIndex;
    cabecalhoIndex.status = '0';
    fwrite(&cabecalhoIndex, sizeof(IndexHeader), 1, arquivoIndexBIN);

    // Alocamos o vetor na RAM com o máximo tamanho possível
    IndexRegistro *registrosIndex = malloc(cabecalhoDados.proxRRN * sizeof(IndexRegistro));
    if(!registrosIndex){
        MensagemErro(); fclose(arquivoDadosBIN); fclose(arquivoIndexBIN);
        return;
    }

    int totalRegs = 0;
    Registro regDados;

    // Posicionamos o ponteiro após o cabeçalho (pulando os primeiros 17 bytes)
    fseek(arquivoDadosBIN, TAM_CABECALHO, SEEK_SET);

    // Lemos registro por registro presentes em todo o arquivo 
    for(int i = 0; i < cabecalhoDados.proxRRN; i++){
        regDados.nomeEstacao = NULL;
        regDados.nomeLinha = NULL;

        //Leitura dos dados em disco para colocá-los em RAM
        LerRegistroBIN(arquivoDadosBIN, &regDados);

        //Se o registro estiver removido NÃO entra no índice
        if(regDados.removido != '1'){ // não está removido
            registrosIndex[totalRegs].codEstacao = regDados.codEstacao;
            registrosIndex[totalRegs].RRN = i;
            totalRegs++;
        }
        LiberarStringRegistro(&regDados);
    }

    // Ordenamos de forma crescente o vetor dos números do codEstacao
    qsort(registrosIndex, totalRegs, sizeof(IndexRegistro), CompararIndexRegistro);

    // Gravamos o índice ordenado no disco + marcamos como consistente
    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegs, arquivoIndex);

    // Liberamos a memória alocada
    free(registrosIndex);
    registrosIndex = NULL;

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoIndex);
}