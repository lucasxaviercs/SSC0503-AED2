#include "utils.h"


/*Lê e descarta a primeira linha do arquivo CSV que é apenas os nomes das colunas da tabela*/
void IgnorarLinhaZeroCSV(FILE *arquivoCSV){
    char buffer[256];
    if(fgets(buffer, 256, arquivoCSV) == NULL){
        // MENSAGEM EXIGIDA quando houver falha no processamento de algum arquivo
        MensagemErro();
        exit(1);
    }
}

/*Cria e prepara o array de armazenamento de nomes de estações únicas.
Retornando o endereço dessa nova lista na memória*/
ControleEstacoes *InicializarControleEstacoes()
{
    ControleEstacoes *controleEstacoes = malloc(sizeof(ControleEstacoes));
    controleEstacoes->capacidadeMaxima = 150;
    controleEstacoes->totalEstacoesUnicas = 0;
    controleEstacoes->listaNomesUnicos = malloc(controleEstacoes->capacidadeMaxima * sizeof(char *));
    return controleEstacoes;
}

/*Verifica se o nome da estação já está armazenado. Caso seja novidade, adiciona na lista e faz
uma verificação se é necessário aumentar o espaço.*/
void RegistrarEstacaoUnica(ControleEstacoes *controleEstacoes, const char *nomeEstacao){
    if (nomeEstacao == NULL)
        return;

    // Verifica se o nome já não consta na lista
    for (int i = 0; i < controleEstacoes->totalEstacoesUnicas; i++)
    {
        if (strcmp(controleEstacoes->listaNomesUnicos[i], nomeEstacao) == 0)
            return;
    }

    // Aumento da capacidade (DOBRANDO) se o limite do array for atingido
    if (controleEstacoes->totalEstacoesUnicas >= controleEstacoes->capacidadeMaxima)
    {
        controleEstacoes->capacidadeMaxima *= 2;
        controleEstacoes->listaNomesUnicos = realloc(controleEstacoes->listaNomesUnicos, controleEstacoes->capacidadeMaxima * sizeof(char *));
    }

    // Aloca o espaço do tamanho da string incuindo o '\0' para salvar a string dentro da lista
    int tamanhoStringComNulo = strlen(nomeEstacao) + 1;
    controleEstacoes->listaNomesUnicos[controleEstacoes->totalEstacoesUnicas] = malloc(tamanhoStringComNulo * sizeof(char));
    memcpy(controleEstacoes->listaNomesUnicos[controleEstacoes->totalEstacoesUnicas], nomeEstacao, tamanhoStringComNulo);

    // Incrementamos o contador de estações únicas, após a inserção
    controleEstacoes->totalEstacoesUnicas++;
}

/*Varre a lista de estações e limpando cada string individualmente e
destrói a lista principal para liberar memória*/
void LiberarControleEstacoes(ControleEstacoes *controleEstacoes){
    for (int i = 0; i < controleEstacoes->totalEstacoesUnicas; i++)
    {
        free(controleEstacoes->listaNomesUnicos[i]);
        controleEstacoes->listaNomesUnicos[i] = NULL;
    }
    free(controleEstacoes->listaNomesUnicos);
    controleEstacoes->listaNomesUnicos = NULL;
    free(controleEstacoes);
    controleEstacoes = NULL;
}


/*Cria a lista que irá guardar os trajetos únicos (COMBINAÇÃO DE ESTAÇÕES).
Retornando o endereço dessa estrutura.*/
ControlePares *InicializarControlePares(){
    ControlePares *controlePares = malloc(sizeof(ControlePares));
    controlePares->capacidadeMaxima = 150;
    controlePares->totalParesUnicos = 0;
    controlePares->listaParesUnicos = malloc(controlePares->capacidadeMaxima * sizeof(ParEstacao));
    return controlePares;
}

/*Avalia a combinação entre o código origem e destino, salvando se o trajeto for novidade.
Faz uma verificação se é necessário aumentar o espaço.*/
void RegistrarParUnico(ControlePares *controlePares, int codigoEstacaoOrigem, int codigoEstacaoDestino){
    // Ignora o registro se algum dos códigos for nulo
    if (codigoEstacaoOrigem == -1 || codigoEstacaoDestino == -1)
        return;

    // Verifica se a combinação desse trajeto já não está salva
    for (int i = 0; i < controlePares->totalParesUnicos; i++)
    {
        if (controlePares->listaParesUnicos[i].codEstacaoOrigem == codigoEstacaoOrigem && controlePares->listaParesUnicos[i].codEstacaoDestino == codigoEstacaoDestino)
        {
            return;
        }
    }

    // Dobra a capacidade se o limite do array foi atingido
    if (controlePares->totalParesUnicos >= controlePares->capacidadeMaxima)
    {
        controlePares->capacidadeMaxima *= 2;
        controlePares->listaParesUnicos = realloc(controlePares->listaParesUnicos, controlePares->capacidadeMaxima * sizeof(ParEstacao));
    }

    // Salva a origem e destino na próxima posição livre do array
    controlePares->listaParesUnicos[controlePares->totalParesUnicos].codEstacaoOrigem = codigoEstacaoOrigem;
    controlePares->listaParesUnicos[controlePares->totalParesUnicos].codEstacaoDestino = codigoEstacaoDestino;

    // Incrementa o contador pares únicos, após salvar no array
    controlePares->totalParesUnicos++;
}

/*Desaloca o vetor contendo os trajetos únicos*/
void LiberarControlePares(ControlePares *controlePares){
    free(controlePares->listaParesUnicos);
    controlePares->listaParesUnicos = NULL;
    free(controlePares);
    controlePares = NULL;
}


/*Apaga as strings de um registro e zera os ponteiros por segurança*/
void LiberarStringRegistro(Registro *registroDados){
    if (registroDados == NULL) return;

    // Libera a string e anula o ponteiro diretamente
    if (registroDados->nomeEstacao != NULL) {
        free(registroDados->nomeEstacao);
        registroDados->nomeEstacao = NULL;
    }

    if (registroDados->nomeLinha != NULL) {
        free(registroDados->nomeLinha);
        registroDados->nomeLinha = NULL;
    }
}

/*Lê os pares de "campo" e "valor" solicitados na filtragem capturando a entrada com aspas 
e salva isso em um vetor de struct de tamanho dimensionado pela quantidade de critérios passados*/
void LerCriteriosBusca(CriterioBusca *criterios, int qtdCriterios){
    for(int i = 0; i < qtdCriterios; i++){
        scanf("%s", criterios[i].nomeDoCampo);

        // Se for string, usamos a função fornecida para tratar as aspas
        if(strcmp(criterios[i].nomeDoCampo, "nomeEstacao") == 0 || strcmp(criterios[i].nomeDoCampo, "nomeLinha") == 0){
            ScanQuoteString(criterios[i].valorBuscado);
        }
        else{ // Se for um campo de numéricos, mantemos o scanf normal como string, mas sem utilizar o ScanQuote
            scanf("%s", criterios[i].valorBuscado);

            // A nossa função de verificação de critério, espera que NULO seja uma string vazia ""
            // Portanto, padronizados o NULO para a funcao VerificaCriterioBusca
            if(strcmp(criterios[i].valorBuscado, "NULO") == 0){
                strcpy(criterios[i].valorBuscado, "");
            }
        }
    }
}

/*Compara o valor registrado em disco com a condição de busca exigida pelo usuário.
Retornando 1 caso o registro atenda a condição exigida, ou 0 caso não satisfaça as condições exigidas*/
int VerificaCriterioBusca(const Registro *registroDados, const char *nomeDoCampo, const char *valorBuscado){
    // Retornaremos 1 se o registro atende ao critério buscado, caso contrário retornaremos 0

    //A função ScanQuoteString transformará a entrada "NULO" em uma string vazia
    // Portanto, se o tamanho for 0, o usuário está buscando NULO
    int buscaPorNulo = (strlen(valorBuscado) == 0);

    // CRITÉRIO PARA CAMPOS DE TAMANHO FIXO
    if (strcmp(nomeDoCampo, "codEstacao") == 0){
        int valorInteiro = buscaPorNulo ? -1 : atoi(valorBuscado);
        return registroDados->codEstacao == valorInteiro;
    }
    else if (strcmp(nomeDoCampo, "codLinha") == 0){
        int valorInteiro = buscaPorNulo ? -1 : atoi(valorBuscado);
        return registroDados->codLinha == valorInteiro;
    }
    else if (strcmp(nomeDoCampo, "codProxEstacao") == 0){
        int valorInteiro = buscaPorNulo ? -1 : atoi(valorBuscado);
        return registroDados->codProxEstacao == valorInteiro;
    }
    else if (strcmp(nomeDoCampo, "distProxEstacao") == 0){
        int valorInteiro = buscaPorNulo ? -1 : atoi(valorBuscado);
        return registroDados->distProxEstacao == valorInteiro;
    }
    else if (strcmp(nomeDoCampo, "codLinhaIntegra") == 0){
        int valorInteiro = buscaPorNulo ? -1 : atoi(valorBuscado);
        return registroDados->codLinhaIntegra == valorInteiro;
    }
    else if (strcmp(nomeDoCampo, "codEstIntegra") == 0){
        int valorInteiro = buscaPorNulo ? -1 : atoi(valorBuscado);
        return registroDados->codEstIntegra == valorInteiro;
    }
    // CRITÉRIO PARA CAMPOS DE TAMANHO VARIÁVEL (as strings)
    else if (strcmp(nomeDoCampo, "nomeEstacao") == 0){
        // Se buscou por NULO, o registro deve estar apontando para NULL
        if (buscaPorNulo) return registroDados->nomeEstacao == NULL;

        // Se o campo do registro aponta para NULL, mas foi buscado uma palavra, então o critério não é atendido
        if (registroDados->nomeEstacao == NULL) return 0;

        // Se o campo e o valor buscado possuem dados, comparamos esse texto
        return strcmp(registroDados->nomeEstacao, valorBuscado) == 0;
    }
    else if (strcmp(nomeDoCampo, "nomeLinha") == 0){
        // Se buscou por NULO, o registro deve estar apontando para NULL
        if (buscaPorNulo) return registroDados->nomeLinha == NULL;

        // Se o campo do registro aponta para NULL, mas foi buscado uma palavra, então o critério não é atendido
        if (registroDados->nomeLinha == NULL) return 0;

        // Se o campo e o valor buscado possuem dados, comparamos esse texto
        return strcmp(registroDados->nomeLinha, valorBuscado) == 0;
    }

    // Caso algum erro aconteça, por exemplo, na passagem de argumento: algum campo de nome inválido
    return 0;
}

/* Modifica os campos do registro em RAM com os novos valores solicitados*/
void AplicarUpdates(Registro *reg, CriterioBusca *updates, int nroUpdates){
    for(int updt = 0; updt < nroUpdates; updt++){
        const char *campo = updates[updt].nomeDoCampo;
        const char *valor = updates[updt].valorBuscado;

        //LerCriterioBusca já converte NULO para ""
        int ehNulo = (strlen(valor) == 0);

        /*CAMPOS FIXOS
        Se forem NULOS -1, SE NÃO convertermos a string para inteiro*/
        if(strcmp(campo, "codEstacao") == 0){
            // codEstacao é chave primária e não aceita valor nulo
            // só atualizamos se o valor for NÃO nulo
            if(!ehNulo) {
                reg->codEstacao = atoi(valor);
            }
        }

        else if(strcmp(campo, "codLinha") == 0){
            reg->codLinha = ehNulo ? -1 : atoi(valor);
        
        }

        else if(strcmp(campo, "codProxEstacao") == 0){
            reg->codProxEstacao = ehNulo ? -1 : atoi(valor);
        
        }
        
        else if(strcmp(campo, "distProxEstacao") == 0){
            reg->distProxEstacao = ehNulo ? -1 : atoi(valor);
        
        }
        
        else if(strcmp(campo, "codLinhaIntegra") == 0){
            reg->codLinhaIntegra = ehNulo ? -1 : atoi(valor);
        
        }
        
        else if(strcmp(campo, "codEstIntegra") == 0){
            reg->codEstIntegra = ehNulo ? -1 : atoi(valor);
        
        }

        /* CAMPOS VARIÁVEIS
        liberamos a string antiga, antes de alocarmos a nova*/
        else if(strcmp(campo, "nomeEstacao") == 0){
            free(reg->nomeEstacao);
            if(ehNulo){ //caso seja nulo
                reg->nomeEstacao = NULL;
                reg->tamNomeEstacao = 0;
            }
            else { //não nulo
                reg->nomeEstacao = strdup(valor); // aloca e copia o novo valor
                reg->tamNomeEstacao = strlen(valor);
            }
        }

        else if(strcmp(campo, "nomeLinha") == 0){
            free(reg->nomeLinha);
            if(ehNulo){ //caso seja nulo
                reg->nomeLinha = NULL;
                reg->tamNomeLinha = 0;
            }
            else { //não nulo
                reg->nomeLinha = strdup(valor); // aloca e copia o novo valor
                reg->tamNomeLinha = strlen(valor);
            }
        }
        
    }
}

/* Varre o arquivo linearmente inteiro filtrando os registros que atendam aos
critérios. Imprime os que bateram com a busca e retorna 1 (sucesso) se encontrar algo,
ou 0 caso contrário */
int BuscaSequencial(FILE *arquivoBIN, int proxRRN, CriterioBusca *criterios, int nroCriterios){
    int BuscaPorID = 0; // Flag de ID (codEstacao) para parar a busca quando for encotrado
    for(int c = 0; c < nroCriterios; c++){ // Verificamos se a busca atual tem o campo codEstacao
        if(strcmp(criterios[c].nomeDoCampo, "codEstacao") == 0){
            BuscaPorID = 1;
            break;
        }
    }

    int registroEncontrado = 0; // Flag de controle ( 0 = false, 1 = true)
    Registro reg;

    // Posicionamos o ponteiro do arquivo para o ínicio dos dados (IGNORANDO O CABEÇALHO = 17 Bytes)
    fseek(arquivoBIN, TAM_CABECALHO, SEEK_SET);

    for(int rrnAtual = 0; rrnAtual < proxRRN; rrnAtual++){
        // "Zera" os ponteiros para prevenir lixo da memoria na leitura
        reg.nomeEstacao =NULL;
        reg.nomeLinha = NULL;

        // Preenche a struct com os dados lido do arquvio e verifica se não está removido
        LerRegistroBIN(arquivoBIN, &reg);

        // Ignoramos todos os registros inválidos
        if(reg.removido != '1'){ // apenas uma segurança a mais para confirmar que NÃO ESTÁ REMOVIDO
            int atendeTodosCriterios = 1; // Assumimos que registro serve (true), até que se prove o contrário

            // Verifica o registro atual a partir dos criterios de busca que foram passados
            for(int criterioAtual = 0; criterioAtual < nroCriterios; criterioAtual++){
                // Retornaremos 1 se o campo atende ao critério buscado
                if(VerificaCriterioBusca(&reg, criterios[criterioAtual].nomeDoCampo, criterios[criterioAtual].valorBuscado) != 1){
                    atendeTodosCriterios = 0; // Falhou em um dos pares de criterios (filtro) passados
                    break; // Descartamos
                }
            }

            // Se passou pelas verificações, imprimimos na tela
            if(atendeTodosCriterios == 1){
                ImprimirRegistro(&reg);
                registroEncontrado = 1; // Sinalizamos que encontramos um que atende ao filtro passado

                if(BuscaPorID == 1){ // Caso o ID da busca (codEstacao) bater, não ficamos percorrendo os demais registro
                    LiberarStringRegistro(&reg);
                    return registroEncontrado; // Retornará 1, ou seja, foi encontrado
                }
            }
        }
        // Desaloco as strings alocadas pelo LerRegistroBIN
        LiberarStringRegistro(&reg);
    }

    // Se chegar aqui retorará 0, ou seja, NÃO foi encontrado
    return registroEncontrado;
}

/*Relê todos os registros não removidos do arquivo de dados para recalcular do zero
os campos nroEstacoes e nroParesEstacao do cabeçalho */
void RecalcularContadoresCabecalho(FILE *arquivoDadosBIN, Header *cabecalhoDados){
    // Reutilizamos as mesmas estruturas auxiliares do CreateTable
    ControleEstacoes *controleEstacoes = InicializarControleEstacoes();
    ControlePares *controlePares = InicializarControlePares();
 
    Registro reg;
 
    // Percorremos todos os registros válidos (não-removidos)
    fseek(arquivoDadosBIN, TAM_CABECALHO, SEEK_SET);
    for(int i = 0; i < cabecalhoDados->proxRRN; i++){
        reg.nomeEstacao = NULL;
        reg.nomeLinha = NULL;
        LerRegistroBIN(arquivoDadosBIN, &reg);

        if(reg.removido != '1'){
            RegistrarEstacaoUnica(controleEstacoes, reg.nomeEstacao);
            RegistrarParUnico(controlePares, reg.codEstacao, reg.codProxEstacao);
        }
        LiberarStringRegistro(&reg);
    }
 
    // Atualiza os campos do cabeçalho em memória primária
    cabecalhoDados->nroEstacoes = controleEstacoes->totalEstacoesUnicas;
    cabecalhoDados->nroParesEstacao = controlePares->totalParesUnicos;
 
    LiberarControleEstacoes(controleEstacoes);
    LiberarControlePares(controlePares);
}

/*Exibe os campos de um registro na tela, substituindo -1 ou NULL,
pela palavra "NULO"*/
void ImprimirRegistro(const Registro *registroDados){
    // Campos fixos, quando -1, imprime NULO
    // Campos variáveis, quando NULL, imprime NULO

    if (registroDados->codEstacao == -1)
        printf("NULO ");
    else
        printf("%d ", registroDados->codEstacao);

    if (registroDados->nomeEstacao == NULL)
        printf("NULO ");
    else
        printf("%s ", registroDados->nomeEstacao);

    if (registroDados->codLinha == -1)
        printf("NULO ");
    else
        printf("%d ", registroDados->codLinha);

    if (registroDados->nomeLinha == NULL)
        printf("NULO ");
    else
        printf("%s ", registroDados->nomeLinha);

    if (registroDados->codProxEstacao == -1)
        printf("NULO ");
    else
        printf("%d ", registroDados->codProxEstacao);

    if (registroDados->distProxEstacao == -1)
        printf("NULO ");
    else
        printf("%d ", registroDados->distProxEstacao);

    if (registroDados->codLinhaIntegra == -1)
        printf("NULO ");
    else
        printf("%d ", registroDados->codLinhaIntegra);

    // Último campo sem espaço no final e /n
    if (registroDados->codEstIntegra == -1)
        printf("NULO\n");
    else
        printf("%d\n", registroDados->codEstIntegra);
}

/*Imprime a mensagem caso haja algum erro na abertura do arquivo*/
void MensagemErro()
{
    printf("Falha no processamento do arquivo.\n");
}

/*Imprime a mensagem quando a busca do usuário não encontra nenhum resultado*/
void MensagemRegistroNaoEncontrado()
{
    printf("Registro inexistente.\n");
}

/*Imprime a mensagem de falha na funcionalidade*/
void MensagemFalhaFuncionalidade()
{
    printf("Falha na execução da funcionalidade.\n");
}

char VerificaEOF(FILE *arquivo)
{
    char ch = 0;
    ch = fgetc(arquivo); // Lê os caracteres do arquivo e retorna (-1) ou um caractere válido

    // Após a leitura, o ponteiro é reposicionado, ou seja, ele verifica se já é o final do arquivo
    // sem consumir o caractere válido("visualização antecipada do próximo caractere")
    fseek(arquivo, -1, SEEK_CUR);

    if (ch == EOF)
    {
        return 0; // EOF atingido
    }
    else
    {
        return 1; // Ainda tem dados
    }
}

// FUNÇÕES JÁ FORNECIDAS

/*
 * Você não precisa entender o código dessa função.
 *
 * Use essa função para comparação no run.codes.
 * Lembre-se de ter fechado (fclose) o arquivo anteriormente.
 *
 * Ela vai abrir de novo para leitura e depois fechar
 * (você não vai perder pontos por isso se usar ela).
 */
void BinarioNaTela(char *arquivo)
{
    FILE *fs;
    if (arquivo == NULL || !(fs = fopen(arquivo, "rb")))
    {
        fprintf(stderr,
                "ERRO AO ESCREVER O BINARIO NA TELA (função binarioNaTela): "
                "não foi possível abrir o arquivo que me passou para leitura. "
                "Ele existe e você tá passando o nome certo? Você lembrou de "
                "fechar ele com fclose depois de usar?\n");
        return;
    }

    fseek(fs, 0, SEEK_END);
    size_t fl = ftell(fs);

    fseek(fs, 0, SEEK_SET);
    unsigned char *mb = (unsigned char *)malloc(fl);
    fread(mb, 1, fl, fs);

    unsigned long cs = 0;
    for (unsigned long i = 0; i < fl; i++)
    {
        cs += (unsigned long)mb[i];
    }

    printf("%lf\n", (cs / (double)100));

    free(mb);
    fclose(fs);
}

/*
 *	Use essa função para ler um campo string delimitado entre aspas (").
 *	Chame ela na hora que for ler tal campo. Por exemplo:
 *
 *	A entrada está da seguinte forma:
 *		nomeDoCampo "MARIA DA SILVA"
 *
 *	Para ler isso para as strings já alocadas str1 e str2 do seu programa,
 * você faz: scanf("%s", str1); // Vai salvar nomeDoCampo em str1
 *		scan_quote_string(str2); // Vai salvar MARIA DA SILVA em str2
 * (sem as aspas)
 *
 */
void ScanQuoteString(char *str)
{
    char R;

    while ((R = getchar()) != EOF && isspace(R))
        ; // ignorar espaços, \r, \n...

    if (R == 'N' || R == 'n')
    { // campo NULO
        getchar();
        getchar();
        getchar();       // ignorar o "ULO" de NULO.
        strcpy(str, ""); // copia string vazia
    }
    else if (R == '\"')
    {
        if (scanf("%[^\"]", str) != 1)
        { // ler até o fechamento das aspas
            strcpy(str, "");
        }
        getchar(); // ignorar aspas fechando
    }
    else if (R != EOF)
    { // vc tá tentando ler uma string que não tá entre
      // aspas! Fazer leitura normal %s então, pois deve
      // ser algum inteiro ou algo assim...
        str[0] = R;
        scanf("%s", &str[1]);
    }
    else
    { // EOF
        strcpy(str, "");
    }
}