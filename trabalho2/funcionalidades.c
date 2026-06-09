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
        fclose(arquivoCSV);
        return;
    }

    // Abrindo arquivo BIN para escrita binária
    FILE *arquivoBIN = fopen(arquivoSaida, "wb");
    // Abortando funcionalidade caso ocorra erro na abertura do arquivo BIN
    if(arquivoBIN == NULL){
        MensagemErro();
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

/*Lê o arquivo binário do começo ao fim para mostrar tudo para o usuário ao imprimir na tela.*/
void SelectFrom(char *arquivoEntrada){
    // Abrindo arquivo BIN para leitura binária
    FILE *arquivoBIN = fopen(arquivoEntrada, "rb");
    // Abortando funcionalidade caso ocorra erro na abertura do arquivo BIN
    if(arquivoBIN == NULL){
        MensagemErro();
        return;
    }    

    // Leitura do cabecalho do arquivo binário
    Header cabecalho;
    LerCabecalhoBIN(arquivoBIN, &cabecalho);

    if(cabecalho.status == '0'){ //arquivo inconsistente 
        MensagemErro();
        fclose(arquivoBIN);
        return;
    }

    if(cabecalho.proxRRN == 0){ // não há nenhum dado gravado, apenas o arquivo foi criado
        MensagemRegistroNaoEncontrado();
        fclose(arquivoBIN);
        return;
    }

    int contadorImpressos = 0;
    Registro registroDados;

    // LOOP PARA PROCESSAR OS DADOS DO REGISTRO
    for(int i = 0; i < cabecalho.proxRRN; i++){

        // Fazemos a leitura do registro no arquivo (disco), salvamos na RAM
        // e fazemos a verificação se o registro está removido ou não
        LerRegistroBIN(arquivoBIN, &registroDados);

        // Só iremos imprimir se não estiver logicamente removido
        // '1' == LOGICAMENTE REMOVIDO e '0' == NÃO ESTÁ MARCADO COMO REMOVIDO
        if(registroDados.removido != '1'){ // apenas uma segurança a mais para confirmar que NÃO ESTÁ REMOVIDO
            ImprimirRegistro(&registroDados);
            contadorImpressos++;
        }

        // Libera as strings alocadas pelo RegistroBIN
        LiberarStringRegistro(&registroDados);
    }

    // Se o contador de registros impressos for ZERO ao final do loop,
    // significa que todos estavam marcados como logicamente removidos
    if(contadorImpressos == 0){
        MensagemRegistroNaoEncontrado();
    }

    fclose(arquivoBIN);
}

/*Faz uma busca personalizada usando critérios de busca (FILTROS) que o usuário passou.
Verificando linha por linha e apenas imprimindo na tela se o registro convergir com os critérios exigidos.*/
void SelectWhere(char *arquivoEntrada, int nroBuscas){
    // Abrindo arquivo BIN para leitura binária
    FILE *arquivoBIN = fopen(arquivoEntrada, "rb");
    // Abortando funcionalidade caso ocorra erro na abertura do arquivo BIN
    if(arquivoBIN == NULL){
        MensagemErro();
        return;
    }    

    // Leitura do cabecalho do arquivo binário
    Header cabecalho;
    LerCabecalhoBIN(arquivoBIN, &cabecalho); // Para sabermos o status e o proxRRN

    if(cabecalho.status == '0'){ //arquivo inconsistente 
        MensagemErro();
        fclose(arquivoBIN);
        return;
    }    

    // Executa o número de buscas solicitadas
    for(int buscaAtual = 0; buscaAtual < nroBuscas; buscaAtual++){
        // Lê quantos filtros serão aplicados nesta busca específica
        // Quantidade de vezes que o par de critério pode repetir em uma busca
        int m_nroCriterios;
        if(scanf("%d", &m_nroCriterios) != 1) break;

        // Armazeno em uma array de struct o par de filtro que serão aplicados
        CriterioBusca criterios[m_nroCriterios];

        // Lemos os critérios de busca passados (FILTRO PASSADOS)
        LerCriteriosBusca(criterios, m_nroCriterios);

        // Se for encontrado = 1
        // Se NÃO for encontrado = 0
        int encontrado = BuscaSequencial(arquivoBIN, cabecalho.proxRRN, criterios, m_nroCriterios);

        // Passei por todo o arquivo e a flag não se alterou, continuou 0, avisa o usuário que o registro não foi encontrado
        if(encontrado == 0){
            MensagemRegistroNaoEncontrado();
        }

        if(buscaAtual < nroBuscas - 1){
            printf("\n");
        }
    }

    fclose(arquivoBIN);
}

/*Por meio de cálculos envolvendo o RRN, é capaz de acessar direto o registro desejado.*/
void RecuperacaoRRN(char *arquivoEntrada, int RRN){
    // Abertura do arquivo BIN para leitura
    FILE *arquivoBIN = fopen(arquivoEntrada, "rb");
    // Abortando funcionalidade caso ocorra erro na abertura do arquivo BIN
    if(arquivoBIN == NULL){
        MensagemErro();
        return;
    }

    // Leitura do cabeçalho para obter o próximo RRN disponível
    Header cabecalho;
    LerCabecalhoBIN(arquivoBIN, &cabecalho);

    // Verificação se o arquivo está consistente
    if(cabecalho.status == '0'){
        MensagemErro();
        fclose(arquivoBIN);
        return;
    }

    // Se o RRN for inválido (negativo ou além dos registros existentes), aborta
    if(RRN < 0 || RRN >= cabecalho.proxRRN){
        MensagemRegistroNaoEncontrado();
        fclose(arquivoBIN);
        return;
    }

    // Cálculo do byte offset do registro para dar fseek
    int byteoffset = TAM_CABECALHO + (RRN * TAM_REGISTRO);
    fseek(arquivoBIN, byteoffset, SEEK_SET);

    // Leitura do registro na posição calculada
    Registro registroDados;
    registroDados.nomeEstacao = NULL;
    registroDados.nomeLinha = NULL;
    LerRegistroBIN(arquivoBIN, &registroDados);

    // Se o registro estiver logicamente removido, não deve ser exibido
    if(registroDados.removido == '1'){
        MensagemRegistroNaoEncontrado();
        LiberarStringRegistro(&registroDados);
        fclose(arquivoBIN);
        return;
    }

    // Chama ImprimirRegistro para exibir os campos do registro lido
    ImprimirRegistro(&registroDados);

    LiberarStringRegistro(&registroDados);
    fclose(arquivoBIN);
}

void CriarIndex(char *arquivoDados, char *arquivoIndex){
    // abre o arquivo de dados para leitura, se deu erro aborta
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb");
    if (arquivoDadosBIN == NULL) {
        MensagemErro();
        return;
    }

    // verifica a consistência do arquivo de dados, se tiver inconsistente aborta
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0'){
        MensagemErro();
        fclose(arquivoDadosBIN);
        return;
    }

    // abre o arquivo de índice para escrita, se deu erro aborta
    FILE *arquivoIndexBIN = fopen(arquivoIndex, "wb+");
    if (arquivoIndexBIN == NULL) {
        MensagemErro();
        fclose(arquivoDadosBIN);
        return;
    }

    // marca como inconsistente durante o processo de criação do índice
    IndexHeader cabecalhoIndex;
    cabecalhoIndex.status = '0';
    fwrite(&cabecalhoIndex, sizeof(IndexHeader), 1, arquivoIndexBIN);

    // Alocamos pensando no pior cenário em que todos possuem um ID
    IndexRegistro *registrosIndex = malloc(cabecalhoDados.proxRRN * sizeof(IndexRegistro));
    if(!registrosIndex){
        MensagemErro();
        fclose(arquivoDadosBIN);
        fclose(arquivoIndexBIN);
        return;
    }

    int totalRegs = 0;
    Registro regDados;


    fseek(arquivoDadosBIN, TAM_CABECALHO, SEEK_SET);
    for(int i = 0; i < cabecalhoDados.proxRRN; i++){
        regDados.nomeEstacao == NULL;
        regDados.nomeLinha == NULL;

        //Leitura total dos dados em disco, para colocá-los em RAM
        LerRegistroBIN(arquivoDadosBIN, &regDados);

        //Se o registro NÃO estiver removido, então entra no índice
        if(regDados.removido != 1){
            registrosIndex[totalRegs].codEstacao = regDados.codEstacao;
            registrosIndex[totalRegs].RRN = i;
            totalRegs++;
        }
        LiberarStringRegistro(&regDados);
    }

    // Ordenamos de forma crescente o vetor dos números do codEstacao
    qsort(registrosIndex, totalRegs, sizeof(IndexRegistro), CompararIndexRegistro);

    // Gravamos o índice ordenado no disco + marcamos como consistente
    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegs);

    free(registrosIndex);
    regDados.nomeEstacao = NULL;
    regDados.nomeLinha = NULL;
    registrosIndex = NULL;

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoIndex);
}

void SelectWhereIndex(char *arquivoDados, char *arquivoIndex, int nroBuscas){
    // Checagem na abertura dos arquivos
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb");
    if(arquivoDadosBIN == NULL){
        MensagemErro();
        return;
    }

    FILE *arquivoIndexBIN= fopen(arquivoIndex, "rb");
    if(arquivoIndexBIN == NULL){
        MensagemErro();
        fclose(arquivoDadosBIN);
        return;
    }

    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0'){ // verificando consistência do arquivo
        MensagemErro();
        fclose(arquivoDadosBIN);
        fclose(arquivoIndexBIN);
        return;
    }

    IndexHeader cabecalhoIndex;
    LerCabecalhoIndex(arquivoIndexBIN, &cabecalhoIndex);
    if (cabecalhoIndex.status == '0'){ // verificando consistência do arquivo
        MensagemErro();
        fclose(arquivoDadosBIN);
        fclose(arquivoIndexBIN);
        return;
    }

    // Indice carregado em memoria primaria apenas quando necessario
    IndexRegistro *registrosIndex = NULL;
    int totalRegsIndex = 0;
    int indiceCarregado = 0;

    for(int buscaAtual = 0; buscaAtual < nroBuscas; buscaAtual++){
        int m_nroCriterios;
        if(scanf("%d", &m_nroCriterios) != 1) break;

        CriterioBusca criterios[m_nroCriterios];
        LerCriteriosBusca(criterios, m_nroCriterios);

        int usarIndice = 0;
        int valorCodEstacao = -1;
        for(int criterioAtual = 0; criterioAtual < m_nroCriterios; criterioAtual++){ // verificando se o id codEstacao está nos critérios
            if(strcmp(criterios[criterioAtual].nomeDoCampo, "codEstacao") == 0 ){
                usarIndice = 1;
                valorCodEstacao = atoi(criterios[criterioAtual].valorBuscado);
                break;
            }
        }

        int registroEncontrado = 0; // flag true ou false

        if(usarIndice == 1){
            if(!indiceCarregado){
                CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegsIndex, cabecalhoDados);
                indiceCarregado = 1;
            }

            // busca bin no índice para obter o RRN
            int RRN = BuscarRegistroIndex(registrosIndex, totalRegsIndex, valorCodEstacao);
            if(RRN != -1){
                long byteoffset = TAM_CABECALHO + ( (long) RRN * TAM_REGISTRO );
                fseek(arquivoDadosBIN, byteoffset, SEEK_SET);

                Registro reg;
                reg.nomeEstacao = NULL;
                reg.nomeLinha = NULL;
                LerRegistroBIN(arquivoDadosBIN, &reg);

                if(reg.removido != '1'){
                    // verificamos os demais critérios além do codEstacao
                    int atendeTodosCriterios = 1;
                    for(int criterioAtual = 0; criterioAtual < m_nroCriterios; criterioAtual++){
                        if(VerificaCriterioBusca(&reg, criterios[criterioAtual].nomeDoCampo, criterios[criterioAtual].valorBuscado) != 1){
                            atendeTodosCriterios = 0;
                            break;
                        }
                    }
                    if(atendeTodosCriterios){
                        ImprimirRegistro(&reg);
                        registroEncontrado = 1;
                    }
                }
                LiberarStringRegistro(&reg);
            }
        }
        else{ // caso nao possua codEstacao (id), realizaremos a BuscaSequencial
            registroEncontrado = BuscaSequencial(arquivoDadosBIN, cabecalhoDados.proxRRN, criterios, m_nroCriterios);
        }

        if(registroEncontrado == 0) MensagemRegistroNaoEncontrado();
        if(buscaAtual < nroBuscas - 1){
            printf("\n");
        }
    }
    free(registrosIndex);
    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);
}

void InsertInto(char *arquivoDados, char *arquivoIndex, int nroInsercoes){

    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb+");
    if (arquivoDadosBIN == NULL) {
        MensagemErro();
        return;
    }

    FILE *arquivoIndexBIN = fopen(arquivoIndex, "rb+");
    if (arquivoIndexBIN == NULL) {
        MensagemErro();
        return;
    }

    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0') {
        MensagemErro();
        fclose(arquivoDadosBIN);
        fclose(arquivoIndexBIN);
        return;
    }

    // marcar como inconsistente
    IndexHeader cabecalhoIndex;
    cabecalhoIndex.status = '0';
    fwrite(&cabecalhoIndex, sizeof(IndexHeader), 1, arquivoIndexBIN);

    cabecalhoDados.status = '0';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    // carregar index em RAM
    IndexRegistro *registrosIndex = NULL;
    int totalRegs = 0;
    CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegs, &cabecalhoDados);

    for(int i = 0; i < nroInsercoes; i++){

        Registro novo;
        novo.removido = '0';
        novo.proximo = -1;

        scanf("%d", &novo.codEstacao);

        char bufferLeitura[100]; // buffer para armazenar as leituras feitas com ScanQuoteString

        ScanQuoteString(bufferLeitura);
        if (bufferLeitura[0] == '\0'){
            novo.nomeEstacao = NULL;
            novo.tamNomeEstacao = 0;
        } else {
            novo.nomeEstacao = strdup(bufferLeitura);
            novo.tamNomeEstacao = strlen(bufferLeitura);
        }

        scanf("%d", &novo.codLinha);

        ScanQuoteString(bufferLeitura);
        if (bufferLeitura[0] == '\0'){
            novo.nomeLinha = NULL;
            novo.tamNomeLinha = 0;
        } else {
            novo.nomeLinha = strdup(bufferLeitura);
            novo.tamNomeLinha = strlen(bufferLeitura);
        }

        // Campos numéricos que podem ser nulo
        scanf("%s", bufferLeitura);
        if (strcmp(bufferLeitura, "NULO") == 0) {
            novo.codProxEstacao = -1;
        } else {
            novo.codProxEstacao = atoi(bufferLeitura);
        }

        scanf("%s", bufferLeitura);
        if (strcmp(bufferLeitura, "NULO") == 0) {
            novo.distProxEstacao = -1;
        } else {
            novo.distProxEstacao = atoi(bufferLeitura);
        }

        scanf("%s", bufferLeitura);
        if (strcmp(bufferLeitura, "NULO") == 0) {
            novo.codLinhaIntegra = -1;
        } else {
            novo.codLinhaIntegra = atoi(bufferLeitura);
        }

        scanf("%s", bufferLeitura);
        if (strcmp(bufferLeitura, "NULO") == 0) {
            novo.codEstIntegra = -1;
        } else {
            novo.codEstIntegra = atoi(bufferLeitura);
        }

        int rrnNovoRegistro;
        if (cabecalhoDados.topo != -1) {
            rrnNovoRegistro = cabecalhoDados.topo;

            int byteoffset = TAM_CABECALHO + (rrnNovoRegistro * TAM_REGISTRO);
            fseek(arquivoDadosBIN, byteoffset, SEEK_SET);

            Registro removido;
            removido.nomeEstacao = NULL;
            removido.nomeLinha = NULL;
            LerRegistroBIN(arquivoDadosBIN, &removido);

            cabecalhoDados.topo = removido.proximo; // atualiza o topo com o último removido da pilha de removidos
            LiberarStringRegistro(&removido);
        } else {
            rrnNovoRegistro = cabecalhoDados.proxRRN;
            cabecalhoDados.proxRRN++;
        }

        int byteoffset = TAM_CABECALHO + (rrnNovoRegistro * TAM_REGISTRO);
        fseek(arquivoDadosBIN, byteoffset, SEEK_SET);
        EscreverRegistroBIN(arquivoDadosBIN, &novo);

        InserirRegistroIndex(&registrosIndex, novo.codEstacao, rrnNovoRegistro, &totalRegs);
        LiberarStringRegistro(&novo);
    }

    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegs);
    free(registrosIndex);

    // reescreve o cabecalho do arquivo de dados marcando como consistente ao final da inserção
    // no arquivo de índices, isso já é feito pela ReescritaIndex
    cabecalhoDados.status = '1';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);   
    
    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoDados);
    BinarioNaTela(arquivoIndex); 
}

void Delete(char *arquivoDados, char *arquivoIndex, int nroRemocoes){
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb+");
    if (arquivoDadosBIN == NULL) {
        MensagemErro();
        return;
    }

    FILE *arquivoIndexBIN = fopen(arquivoIndex, "rb+");
    if (arquivoIndexBIN == NULL) {
        MensagemErro();
        return;
    }

    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    // checagem da consistência do arquivo de dados antes de iniciar o delete
    if (cabecalhoDados.status == '0') {
        MensagemErro();
        fclose(arquivoDadosBIN);
        fclose(arquivoIndexBIN);
        return;
    }

    // marcar arquivo de dados como inconsistente
    cabecalho.status = '0';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalho);

    // marcar índice como inconsistente também
    IndexHeader cabecalhoIndex;
    cabecalhoIndex.status = '0';
    fseek(arquivoIndexBIN, 0, SEEK_SET);
    fwrite(&cabecalhoIndex, sizeof(IndexHeader), 1, arquivoIndexBIN);

    IndexRegistro *registrosIndex = NULL;
    int totalRegs = 0;
    CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegs, &cabecalhoDados);

    for (int i = 0; i < nroRemocoes; i++){
        int nroCriterios;
        scanf("%d", &nroCriterios);

        CriterioBusca criterios[nroCriterios];
        LerCriteriosBusca(criterios, nroCriterios);

        fseek(arquivoDadosBIN, TAM_CABECALHO, SEEK_SET);

        for(int rrnAtual = 0; rrnAtual < cabecalhoDados.proxRRN; rrnAtual++){
            // reposiciona para o registro atual antes de ler
            // isso é necessário porque no caso de remoção voltamos para atualizar o campo de removido, o que atrapalharia a leitura
            long byteOffset = TAM_CABECALHO + (rrnAtual * TAM_REGISTRO);
            fseek(arquivoDadosBIN, byteOffset, SEEK_SET); 

            Registro reg;
            reg.nomeEstacao = NULL;
            reg.nomeLinha = NULL;
            LerRegistroBIN(arquivoDadosBIN, &reg);

            if (reg.removido == '1'){
                LiberarStringRegistro(&reg);
                continue;
            }

            int atendeTodos = 1;
            for(int criterio = 0; criterio < nroCriterios; criterio++){
                if(VerificaCriterioBusca(&reg, criterios[criterio].nomeDoCampo, criterios[criterio].valorBuscado) != 1){
                    atendeTodos = 0;
                    break;
                }
            }

            if (atendeTodos == 1){
                // se atendeu os critérios, marca como removido e empilha
                reg.removido = '1';
                reg.proximo = cabecalhoDados.topo; 
                cabecalhoDados.topo = rrnAtual;

                int byteOffset = TAM_CABECALHO + (rrnAtual * TAM_REGISTRO);
                fseek(arquivoDadosBIN, byteOffset, SEEK_SET);

                fwrite(&reg.removido, sizeof(char), 1, arquivoDadosBIN);
                fwrite(&reg.proximo, sizeof(int), 1, arquivoDadosBIN);
                
                cabecalhoDados.nroEstacoes--; // atualiza o número de estações no cabeçalho
                RemoverRegistroIndex(&registrosIndex, &totalRegs, reg.codEstacao);
            }

            LiberarStringRegistro(&reg);

        }
    }

    // reescreve o índice após as remoções
    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegs);
    free(registrosIndex);

    cabecalhoDados.status = '1';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoDados);
    BinarioNaTela(arquivoIndex);
}
        
void Update(char *arquivoDados, char *arquivoIndex, int nroAtualizacoes){
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb+");
    if(!arquivoDadosBIN){
        MensagemErro();
        return;
    }

    FILE *arquivoIndexBIN = fopen(arquivoIndex, "rb+");
    if(!arquivoIndexBIN){
        MensagemErro();
        fclose(arquivoDadosBIN);
        fclose(arquivoIndexBIN);
        return;
    }

    Header cabecalho;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalho);
    if(cabecalho.status == '0'){
        MensagemErro();
        fclose(arquivoDadosBIN);
        fclose(arquivoIndexBIN);
        return;
    }

    // Marcamos ambos arquivos como inconsistente enquanto realizamos as operações
    cabecalho.status = '0';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalho);

    IndexHeader cabecalhoIndex;
    cabecalhoIndex.status = 0;
    fseek(arquivoIndexBIN, 0, SEEK_SET);
    fwrite(&cabecalhoIndex.status, sizeof(char), 1, arquivoIndexBIN);

    // Carregamos o índice inteiro em memória primária
    IndexRegistro *registrosIndex = NULL;
    int totalRegsIndex = 0;
    CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegsIndex);

    for(int op=0; op < nroAtualizacoes; op++){
        // Lê o os critérios de busca
        int m_nroCriterios;
        if(scanf("%d", &m_nroCriterios) != 1) break;
        CriterioBusca criteriosBusca[m_nroCriterios];
        LerCriteriosBusca(criteriosBusca, m_nroCriterios);

        // Lê os campos para atualizar
        int p_nroUpdates;
        if(scanf("%d", &p_nroUpdates) != 1) break;
        CriterioBusca criteriosUpdates[p_nroUpdates];
        LerCriteriosBusca(criteriosUpdates, p_nroUpdates);

        for(int RRN_Atual = 0; RRN_Atual < cabecalho.proxRRN; RRN_Atual++){
            // Calculamos o byteoffset antes de ler, para caso seja preciso reposicionar para escrever
            long byteoffset = TAM_CABECALHO + (long) (RRN_Atual * TAM_REGISTRO);
            fseek(arquivoDadosBIN, byteoffset, SEEK_SET);

            Registro reg;
            reg.nomeEstacao = NULL;
            reg.nomeLinha = NULL;
            LerRegistroBIN(arquivoDadosBIN, &reg);
            if(reg.removido == '1'){
                LiberarStringRegistro(&reg);
                continue;
            }

            int atendeTodos = 1; //flag de controle de atender os critérios de busca
            // Loop para verificação se o registro atual atende a todos os critérios de busca
            for(int c = 0; c < m_nroCriterios; c++){
                if(VerificaCriterioBusca(&reg, criteriosBusca[c].nomeDoCampo, criteriosBusca[c].valorBuscado) != 1){
                    atendeTodos = 0;
                    break;
                }
            }

            if(atendeTodos == 1){
                int codEstacaoAntiga = reg.codEstacao; //guardamos antes de alterar

                //Aplicamos as atualizações em memória primária
                AplicarUpdates(&reg, criteriosUpdates, p_nroUpdates);

                //Reposicionamos o cursor p/ o início deste registro e sobreescrevemos
                fseek(arquivoDadosBIN, byteoffset, SEEK_SET);
                EscreverRegistroBIN(arquivoDadosBIN, &reg);

                //Se houve alteração no ID (codEstacao), ajustamos o índice
                if(reg.codEstacao != codEstacaoAntiga){
                    RemoverRegistroIndex(&registrosIndex, &totalRegsIndex, codEstacaoAntiga);
                    InserirRegistroIndex(&registrosIndex, reg.codEstacao, RRN_Atual, &totalRegsIndex);
                }
            }
            LiberarStringRegistro(&reg);
        }
    }

    //Reescrevemos o índice atualizado e alteramos o arquivo para consistente
    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegsIndex);
    free(registrosIndex);
    registrosIndex = NULL;

    cabecalho.status = '1';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalho);

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoDados);
    BinarioNaTela(arquivoIndex);
}
