#include "funcionalidades.h"


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
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoBIN, &cabecalhoDados);

    if(cabecalhoDados.status == '0'){ //arquivo inconsistente 
        MensagemErro();
        fclose(arquivoBIN);
        return;
    }

    if(cabecalhoDados.proxRRN == 0){ // não há nenhum dado gravado, apenas o arquivo foi criado
        MensagemRegistroNaoEncontrado();
        fclose(arquivoBIN);
        return;
    }

    int contadorImpressos = 0;
    Registro registroDados;

    // LOOP PARA PROCESSAR OS DADOS DO REGISTRO
    for(int i = 0; i < cabecalhoDados.proxRRN; i++){

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
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoBIN, &cabecalhoDados); // Para sabermos o status e o proxRRN

    if(cabecalhoDados.status == '0'){ //arquivo inconsistente 
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
        int encontrado = BuscaSequencial(arquivoBIN, cabecalhoDados.proxRRN, criterios, m_nroCriterios);

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

/* Faz uma busca personalizada usando critérios de busca (FILTROS) que o usuário passou.
Porém, se o filtro contiver um ID (codEstacao), utilizamos a busca binária (O(log n)) no arq. de índice
para encontrar o registro procurado, caso contrário recorremos a busca sequencial(O(n))*/
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

    // Lê o cabeçalho de dados para verificar a consitência (se foram fechado corretamente na última execução)
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0'){
        MensagemErro(); fclose(arquivoDadosBIN); fclose(arquivoIndexBIN);
        return;
    }

    // Lê o cabeçalho de índice para verificar a consistência (se foram fechado corretamente na última execução)
    IndexHeader cabecalhoIndex;
    LerCabecalhoIndex(arquivoIndexBIN, &cabecalhoIndex);
    if (cabecalhoIndex.status == '0'){ // verificando consistência do arquivo
        MensagemErro(); fclose(arquivoDadosBIN); fclose(arquivoIndexBIN);
        return;
    }

    //  - LAZY LOADING - 
    // Indice carregado em memoria primaria apenas quando necessario
    // posto que ele pode fazer N buscas e nenhuma usar o 'codEstacao'
    // portanto não ser plausível de otimização com base no índice
    IndexRegistro *registrosIndex = NULL;
    int totalRegsIndex = 0;
    int indiceCarregado = 0; // flag indicadora de se já trouxemos ou não o índice pra RAM (0 = NÃO TROUXE | 1 = TROUXEMOS PARA A RAM)

    // Loop de processamento das buscas
    for(int buscaAtual = 0; buscaAtual < nroBuscas; buscaAtual++){
        int m_nroCriterios;
        if(scanf("%d", &m_nroCriterios) != 1) break;

        // Guarda os filtros exigidos para a busca atual
        CriterioBusca criterios[m_nroCriterios];
        LerCriteriosBusca(criterios, m_nroCriterios);

        // Verificação se a chave (codEstacao) é um dos filtros exigidos
        int usarIndice = 0;
        int valorCodEstacao = -1;
        for(int criterioAtual = 0; criterioAtual < m_nroCriterios; criterioAtual++){ // verificando se o id codEstacao está nos critérios
            if(strcmp(criterios[criterioAtual].nomeDoCampo, "codEstacao") == 0 ){
                usarIndice = 1; // Sinaliza que codEstacao é um dos filtros exigidos -> podemos usar o índice para otimizar a busca
                valorCodEstacao = atoi(criterios[criterioAtual].valorBuscado); // Guardamos o ID que vamos buscar
                break;
            }
        }

        int registroEncontrado = 0; // flag para sucesso ou não da busca (0 = NÃO ENCONTRADO | 1 = ENCONTRAMOS)

        if(usarIndice == 1){ // Iremos usar o índice para realizar a busca
            if(indiceCarregado == 0 ){// caso o índice ainda não esteja em RAM
                CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegsIndex, &cabecalhoDados);
                indiceCarregado = 1;
            }

            // Busca binária no vetor para extrair a posição dele
            int posVetor = BuscarRegistroIndex(registrosIndex, totalRegsIndex, valorCodEstacao);
            if(posVetor != -1){ // Caso a busca binária encontrar o codEstacao (ID) nos índices
                // Pegamos o RRN correspondente
                int RRN = registrosIndex[posVetor].RRN;
                
                // Calcula em qual byte o registro está no disco
                long byteoffset = TAM_CABECALHO + ( (long) RRN * TAM_REGISTRO );
                fseek(arquivoDadosBIN, byteoffset, SEEK_SET);
                // Lemos apenas o registo especifíco, sem ler todo o arquivo
                Registro regDados;
                regDados.nomeEstacao = NULL;
                regDados.nomeLinha = NULL;
                LerRegistroBIN(arquivoDadosBIN, &regDados);

                // Checamos se não é um registro logicamente removido
                if(regDados.removido != '1'){
                    // Verificamos os demais critérios além do codEstacao, pois pode ter sido passado múltiplos critérios
                    int atendeTodosCriterios = 1;
                    for(int criterioAtual = 0; criterioAtual < m_nroCriterios; criterioAtual++){
                        // Se falhar em qualquer outro critério da busca, ignoramos o registro
                        if(VerificaCriterioBusca(&regDados, criterios[criterioAtual].nomeDoCampo, criterios[criterioAtual].valorBuscado) != 1){
                            atendeTodosCriterios = 0;
                            break;
                        }
                    }
                    if(atendeTodosCriterios){ // Caso atenda todos critérios
                        ImprimirRegistro(&regDados);
                        registroEncontrado = 1;
                    }
                }
                LiberarStringRegistro(&regDados);
            }
        }
        else{ // Caso nao possua codEstacao (id) como critério de busca, realizaremos a BuscaSequencial pelo arquivo
            registroEncontrado = BuscaSequencial(arquivoDadosBIN, cabecalhoDados.proxRRN, criterios, m_nroCriterios);
        }

        // Se após ambas possibilidades de busca a flag não alterou significa que o registro não existe
        if(registroEncontrado == 0) MensagemRegistroNaoEncontrado();
        
        if(buscaAtual < nroBuscas - 1){ // formatação
            printf("\n");
        }
    }
    // Caso o índice foi alocado ao final desalocamos a memória primária utilizada
    if(indiceCarregado) free(registrosIndex);

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);
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
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoBIN, &cabecalhoDados);

    // Verificação se o arquivo está consistente
    if(cabecalhoDados.status == '0'){
        MensagemErro();
        fclose(arquivoBIN);
        return;
    }

    // Se o RRN for inválido (negativo ou além dos registros existentes), aborta
    if(RRN < 0 || RRN >= cabecalhoDados.proxRRN){
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