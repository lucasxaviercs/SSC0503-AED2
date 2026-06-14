#include "funcionalidades.h"


// Função auxiliar que varre o arquivo para descobrir se ainda existe outra estação ativa com este nome
static int VerificaNomeEstacaoExiste(FILE *arquivoDadosBIN, int proxRRN, const char *nomeBuscado, int rrnIgnorado) {
    // Se a estação não tem nome, não tem o que buscar
    if (nomeBuscado == NULL) return 0; 

    int existe = 0;
    // Varredura sequencial em todos possíveis RRN do disco
    for (int rrn = 0; rrn < proxRRN; rrn++) {
        // Se este é o registro que estamos a modificando/apagando, ignoramos
        if (rrn == rrnIgnorado) continue; 

        // Calcula em qual byte o registro começa e salta até ele
        long byteOffset = TAM_CABECALHO + (rrn * TAM_REGISTRO);
        fseek(arquivoDadosBIN, byteOffset, SEEK_SET);

        Registro reg;
        reg.nomeEstacao = NULL; reg.nomeLinha = NULL;
        LerRegistroBIN(arquivoDadosBIN, &reg); // Traz o registro para memória RAM

        // VERIFICAÇÃO
        // O registro não pode estar logicamente removido,
        // precisa ter um nome alocado e
        // o texto deve ser exatamente igual ao buscado
        if (reg.removido == '0' && reg.nomeEstacao != NULL && strcmp(reg.nomeEstacao, nomeBuscado) == 0) {
            existe = 1; // O nome ainda existirá em outro registro 
            LiberarStringRegistro(&reg);
            break;
        }
        LiberarStringRegistro(&reg);
    }

    return existe;
}

/* Realiza a remoção lógica de um registro, ou seja, é apenas marcado como removido e permite
que aquele espaço possa ser reutilizado por um outro novo registro. O RRN desse registro removido 
passa ser o novo topo da pilha no cabeçalho, que será utilizado em outras funcionalidades*/
void Delete(char *arquivoDados, char *arquivoIndex, int nroRemocoes) {
    // Abertura e verificação se ocorreu tudo certo no arquivo
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb+");
    FILE *arquivoIndexBIN = fopen(arquivoIndex, "rb+");
    if (arquivoDadosBIN == NULL || arquivoIndexBIN == NULL) {
        MensagemErro();
        if (arquivoDadosBIN) fclose(arquivoDadosBIN);
        if (arquivoIndexBIN) fclose(arquivoIndexBIN);
        return;
    }

    // Realiza a leitura do cabeçalho 
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0') {
        MensagemErro();
        fclose(arquivoDadosBIN); fclose(arquivoIndexBIN);
        return;
    }

    // Marca o arquivo como inconsistente durante a operação da funcionalidade
    cabecalhoDados.status = '0';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    // Carrega o índice para memória primária
    IndexRegistro *registrosIndex = NULL;
    int totalRegs = 0;
    CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegs, &cabecalhoDados);

    // Loop de remoções
    for (int i = 0; i < nroRemocoes; i++) {
        // Lê a quantidade de critérios da remoção
        int nroCriterios;
        scanf("%d", &nroCriterios);

        // Lê os pares de critério
        CriterioBusca criterios[nroCriterios];
        LerCriteriosBusca(criterios, nroCriterios);

        // Verifica se codEstacao é um dos filtros, para poder otimizar a busca
        int idBuscado = -1;
        for (int j = 0; j < nroCriterios; j++) {
            if (strcmp(criterios[j].nomeDoCampo, "codEstacao") == 0) {
                idBuscado = atoi(criterios[j].valorBuscado);
                break;
            }
        }

        if (idBuscado != -1) { // BUSCA COM ÍNDICE
            // Posição do índice no vetor de registros de índice em RAM
            int pos = BuscarRegistroIndex(registrosIndex, totalRegs, idBuscado);
            if (pos != -1) {
                // Descobre o RRN correspondente
                int rrnAtual = registrosIndex[pos].RRN;
                long offset = TAM_CABECALHO + (rrnAtual * TAM_REGISTRO);
                fseek(arquivoDadosBIN, offset, SEEK_SET); // Salta para o endereço físico do registro

                Registro reg;
                reg.nomeEstacao = NULL; reg.nomeLinha = NULL;
                LerRegistroBIN(arquivoDadosBIN, &reg); // Lê e traz o registro para a memória primária

                // Caso esse registro já não esteja apagado
                if (reg.removido == '0') {
                    // Validação se os demais critérios de busca batem
                    int match = 1;
                    for (int j = 0; j < nroCriterios; j++) {
                        if (!VerificaCriterioBusca(&reg, criterios[j].nomeDoCampo, criterios[j].valorBuscado)) {
                            match = 0; break;
                        }
                    }
                    if (match) { // Passou nos demais critérios de busca
                        // Verifica a unicidade daquele nome de estação (OU SEJA, SE É A ÚLTIMA ESTAÇÃO COM ESSE NOME NO ARQ. INTEIRO)
                        // Se for a última com esse nome -> Atualiza os contadores de estações 
                        int nomeExiste = VerificaNomeEstacaoExiste(arquivoDadosBIN, cabecalhoDados.proxRRN, reg.nomeEstacao, rrnAtual);
                        if (!nomeExiste && cabecalhoDados.nroEstacoes > 0) cabecalhoDados.nroEstacoes--;

                        // Se essa estação ligava com outra estação, também diminuímos o número de pares de estação
                        if (reg.codProxEstacao != -1 && cabecalhoDados.nroParesEstacao > 0) cabecalhoDados.nroParesEstacao--;

                        // Anotamos que está apagado e utilizamos a lógica da pilha de buraco
                        // então passamos o rrnAtual para o topo da pilha para ele saber
                        // onde há espaço disponível para um registro
                        char removido = '1';
                        int proximo = cabecalhoDados.topo;
                        cabecalhoDados.topo = rrnAtual;

                        // Sobrescrevemos esses bytes da alteração no cabeçalho no disco
                        fseek(arquivoDadosBIN, offset, SEEK_SET);
                        fwrite(&removido, sizeof(char), 1, arquivoDadosBIN);
                        fwrite(&proximo, sizeof(int), 1, arquivoDadosBIN);

                        // Removemos a chave desse índice
                        RemoverRegistroIndex(&registrosIndex, &totalRegs, reg.codEstacao);
                    }
                }
                LiberarStringRegistro(&reg); // Desalocamos as strings do registro
            }
        } 
        
        else { // BUSCA SEQUENCIAL
            // REPETE A LÓGICA ANTERIOR DE REMOÇÃO, CONTUDO SEM OTIMIZAÇÃO DA BUSCA BINÁRIA PELO ÍNDICE
            for (int rrnAtual = 0; rrnAtual < cabecalhoDados.proxRRN; rrnAtual++) {
                long offset = TAM_CABECALHO + (rrnAtual * TAM_REGISTRO);
                fseek(arquivoDadosBIN, offset, SEEK_SET);

                Registro reg;
                reg.nomeEstacao = NULL; reg.nomeLinha = NULL;
                LerRegistroBIN(arquivoDadosBIN, &reg);

                if (reg.removido == '0') {
                    int match = 1;
                    for (int j = 0; j < nroCriterios; j++) {
                        if (!VerificaCriterioBusca(&reg, criterios[j].nomeDoCampo, criterios[j].valorBuscado)) {
                            match = 0; break;
                        }
                    }
                    if (match) {
                        
                        int nomeExiste = VerificaNomeEstacaoExiste(arquivoDadosBIN, cabecalhoDados.proxRRN, reg.nomeEstacao, rrnAtual);
                        if (!nomeExiste && cabecalhoDados.nroEstacoes > 0) cabecalhoDados.nroEstacoes--;
                        if (reg.codProxEstacao != -1 && cabecalhoDados.nroParesEstacao > 0) cabecalhoDados.nroParesEstacao--;

                        char removido = '1';
                        int proximo = cabecalhoDados.topo;
                        cabecalhoDados.topo = rrnAtual;

                        fseek(arquivoDadosBIN, offset, SEEK_SET);
                        fwrite(&removido, sizeof(char), 1, arquivoDadosBIN);
                        fwrite(&proximo, sizeof(int), 1, arquivoDadosBIN);

                        RemoverRegistroIndex(&registrosIndex, &totalRegs, reg.codEstacao);
                    }
                }
                LiberarStringRegistro(&reg);
            }
        }
    }

    // Reescrevemos a nossa listas de índices que está em RAM após as deleções e salvamos novamente em disco
    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegs, arquivoIndex);
    free(registrosIndex); // Descartamos a lista de índice em memória primária

    // "Avisamos" que a operação terminou e retorna o seu sinal de consistente ao cabeçalho
    cabecalhoDados.status = '1';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoDados);
    BinarioNaTela(arquivoIndex);
}

/* Insere novos registros gerenciando uma pilha de registros removidos e consultando
espaços possíveis para realizar as inserções*/
void InsertInto(char *arquivoDados, char *arquivoIndex, int nroInsercoes) {
    // Abertura dos arquivos e verificação se ocorreu tudo certo no processo de abertura
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb+");
    FILE *arquivoIndexBIN = fopen(arquivoIndex, "rb+");
    if (arquivoDadosBIN == NULL || arquivoIndexBIN == NULL) {
        MensagemErro();
        if (arquivoDadosBIN) fclose(arquivoDadosBIN);
        if (arquivoIndexBIN) fclose(arquivoIndexBIN);
        return;
    }

    // Verificação da consistência do arquivo
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0') {
        MensagemErro();
        fclose(arquivoDadosBIN); fclose(arquivoIndexBIN);
        return;
    }

    // Marcamos como inconsistente enquanto realizamos as operações
    cabecalhoDados.status = '0';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    // Colocamos os índices em memória primária para agilizar as buscas e/ou verificações de registros
    IndexRegistro *registrosIndex = NULL;
    int totalRegs = 0;
    CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegs, &cabecalhoDados);

    // Loop de inserções
    for (int i = 0; i < nroInsercoes; i++) {
        Registro novo;
        novo.removido = '0';
        novo.proximo = -1;

        // Leitura da chave
        scanf("%d", &novo.codEstacao);

        char buffer[256];

        // Leitura e tratamento das string
        // ScanQuoteString lida com as aspas ou
        // transforma NULO em string vazia ""
        ScanQuoteString(buffer);
        novo.tamNomeEstacao = strlen(buffer);
        novo.nomeEstacao = novo.tamNomeEstacao > 0 ? strdup(buffer) : NULL;

        //Leitura e tratamento dos inteiros
        //Se for "NULO", o valor torna -1 na struct
        scanf("%s", buffer);
        novo.codLinha = (strcmp(buffer, "NULO") == 0) ? -1 : atoi(buffer);

        ScanQuoteString(buffer);
        novo.tamNomeLinha = strlen(buffer);
        novo.nomeLinha = novo.tamNomeLinha > 0 ? strdup(buffer) : NULL;

        scanf("%s", buffer);
        novo.codProxEstacao = (strcmp(buffer, "NULO") == 0) ? -1 : atoi(buffer);

        scanf("%s", buffer);
        novo.distProxEstacao = (strcmp(buffer, "NULO") == 0) ? -1 : atoi(buffer);

        scanf("%s", buffer);
        novo.codLinhaIntegra = (strcmp(buffer, "NULO") == 0) ? -1 : atoi(buffer);

        scanf("%s", buffer);
        novo.codEstIntegra = (strcmp(buffer, "NULO") == 0) ? -1 : atoi(buffer);

        // Usamos a busca binária no índice para garantir que o ID não existe
        // Se retornar diferente de -1, o ID já existe e abortamos a inserção atual
        if (BuscarRegistroIndex(registrosIndex, totalRegs, novo.codEstacao) != -1) {
            LiberarStringRegistro(&novo);
            continue;
        }

        // Checa se o nome é inédito no arquivo antes de inserir, pois o total
        // de estações únicas deve aumentar se inserido
        int nomeExiste = VerificaNomeEstacaoExiste(arquivoDadosBIN, cabecalhoDados.proxRRN, novo.nomeEstacao, -1);
        
        int rrnNovoRegistro;
        if (cabecalhoDados.topo != -1) { // checa se tem espaço livre para reusar no apontamento do topo
            // Reuso do espaço livre apontado pelo topo
            rrnNovoRegistro = cabecalhoDados.topo;

            long offset = TAM_CABECALHO + (rrnNovoRegistro * TAM_REGISTRO);
            // Pula o 'removido' (char) para ler apenas o 'proximo' (int)
            fseek(arquivoDadosBIN, offset + 1, SEEK_SET); 
            int proximoDaPilha;
            fread(&proximoDaPilha, sizeof(int), 1, arquivoDadosBIN);

            // O topo passa a apontar para o próximo buraco
            cabecalhoDados.topo = proximoDaPilha;

        } else { // Não há espaço (buraco) para inserir o registro
            // Insere depois do último registro presente no arquivo
            rrnNovoRegistro = cabecalhoDados.proxRRN;
            cabecalhoDados.proxRRN++;
        }

        // Movemos o cursor para a posição física em que iremos gravar no disco
        long offset = TAM_CABECALHO + (rrnNovoRegistro * TAM_REGISTRO);
        fseek(arquivoDadosBIN, offset, SEEK_SET);
        EscreverRegistroBIN(arquivoDadosBIN, &novo); // Escreve no disco o registro

        // Atualiza o sumário de índices
        InserirRegistroIndex(&registrosIndex, novo.codEstacao, rrnNovoRegistro, &totalRegs);

        // Atualizações
        if (!nomeExiste) cabecalhoDados.nroEstacoes++; // ESTAÇÃO QUE É NOVIDADE -> INCREMENTA
        if (novo.codProxEstacao != -1) cabecalhoDados.nroParesEstacao++; // TEM UM PRÓXIMO -> NOVO PAR -> INCREMENTA

        LiberarStringRegistro(&novo);
    }

    // Reescreve o arquivo de índice utilizando a memória primária atualizada dos índices
    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegs, arquivoIndex);
    free(registrosIndex);

    // Salvando o cabeçalho final e marca como consistente o arquivo
    cabecalhoDados.status = '1';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoDados);
    BinarioNaTela(arquivoIndex);
}

/* Busca até encontrar um ou mais registros que batam com os critérios de busca e
altera campos específicos dele.*/
void Update(char *arquivoDados, char *arquivoIndex, int nroAtualizacoes){
    // Abertura dos arquivos para leitura e escrita
    // Checagem se não ocorreu nenhum erro durante o processo de abertura
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb+");
    if(!arquivoDadosBIN){
        MensagemErro();
        return;
    }

    FILE *arquivoIndexBIN = fopen(arquivoIndex, "rb+");
    if(!arquivoIndexBIN){
        MensagemErro();
        fclose(arquivoDadosBIN);
        return;
    }

    // Lemos os dois cabeçalhos primeiro para checar a consistência dos arquivos ('1')
    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    IndexHeader cabecalhoIndex;
    LerCabecalhoIndex(arquivoIndexBIN, &cabecalhoIndex);

    // Verificamos a consitência de ambos
    if(cabecalhoDados.status == '0' || cabecalhoIndex.status == '0'){
        MensagemErro();
        fclose(arquivoDadosBIN);
        fclose(arquivoIndexBIN);
        return;
    }

    // Carregamos o índice inteiro em memória primária (ENQUANTO ELE AINDA É VALIDO '1')
    IndexRegistro *registrosIndex = NULL;
    int totalRegsIndex = 0;
    CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegsIndex, &cabecalhoDados);

    // Marcamos ambos arquivos como inconsistente no disco enquanto realizamos as operações
    cabecalhoDados.status = '0';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    cabecalhoIndex.status = '0';
    EscreverCabecalhoIndex(arquivoIndexBIN, &cabecalhoIndex);

    // Loop para realizar as atualizações
    for(int op=0; op < nroAtualizacoes; op++){
        // Lê o os critérios de busca
        int m_nroCriterios;
        if(scanf("%d", &m_nroCriterios) != 1) break;
        CriterioBusca criteriosBusca[m_nroCriterios];
        LerCriteriosBusca(criteriosBusca, m_nroCriterios);

        // Lê os campos que serão atualizados
        int p_nroUpdates;
        if(scanf("%d", &p_nroUpdates) != 1) break;
        CriterioBusca criteriosUpdates[p_nroUpdates];
        LerCriteriosBusca(criteriosUpdates, p_nroUpdates);

        // Verifica se codEstacao está entre os filtros para usar o índice
        int usarIndice = 0;
        int valorCodEstacao = -1;
        for(int c = 0; c < m_nroCriterios; c++){
            if(strcmp(criteriosBusca[c].nomeDoCampo, "codEstacao") == 0){
                usarIndice = 1;
                valorCodEstacao = atoi(criteriosBusca[c].valorBuscado);
                break;
            }
        }

        // Caso tenha índice -> busca otimizada para realizar a alteração
        if (usarIndice == 1) { // busca binária no índice, caso tenha
            // Busca da posição do índice no vetor em memória primária
            int posVetor = BuscarRegistroIndex(registrosIndex, totalRegsIndex, valorCodEstacao);
            if (posVetor != -1) {
                // Pega o RRN correspondente
                int RRN_Atual = registrosIndex[posVetor].RRN;

                // Calcula o byteoffset correspondete e "pula" até o registro
                long byteoffset = TAM_CABECALHO + (long) (RRN_Atual * TAM_REGISTRO);
                fseek(arquivoDadosBIN, byteoffset, SEEK_SET);

                Registro regDados;
                regDados.nomeEstacao = NULL;
                regDados.nomeLinha = NULL;
                LerRegistroBIN(arquivoDadosBIN, &regDados); // Realiza a leitura do registro que queremos

                // Confirma se não está removido logicamente
                if(regDados.removido != '1') {
                    // Confirma se atende aos outros critérios de busca exigidos
                    int atendeTodos = 1; 
                    for(int c = 0; c < m_nroCriterios; c++){
                        if(VerificaCriterioBusca(&regDados, criteriosBusca[c].nomeDoCampo, criteriosBusca[c].valorBuscado) != 1){
                            atendeTodos = 0;
                            break;
                        }
                    }

                    if(atendeTodos == 1){ //Atende aos outros critérios de busca
                        // Guarda ID (codEstacao) antigo antes de alterar
                        // para saber se vai precisar atualizar o índice
                        int codEstacaoAntiga = regDados.codEstacao; 

                        //Aplica as atualizações em memória primária
                        AplicarUpdates(&regDados, criteriosUpdates, p_nroUpdates);

                        //Reposiciona o cursor para o início deste registro e sobreescreve
                        fseek(arquivoDadosBIN, byteoffset, SEEK_SET);
                        EscreverRegistroBIN(arquivoDadosBIN, &regDados);

                        //Se houve alteração no ID (codEstacao), ajustamos o índice
                        if(regDados.codEstacao != codEstacaoAntiga){
                            RemoverRegistroIndex(&registrosIndex, &totalRegsIndex, codEstacaoAntiga);
                            InserirRegistroIndex(&registrosIndex, regDados.codEstacao, RRN_Atual, &totalRegsIndex);
                        }
                    }
                }
                LiberarStringRegistro(&regDados); // Desaloca as strings lidas
            }

        } 
        
        else { // busca sequencial caso não tenha o índice
            for(int RRN_Atual = 0; RRN_Atual < cabecalhoDados.proxRRN; RRN_Atual++){
                // Calculamos o byteoffset antes de ler, para caso seja preciso reposicionar para escrever
                long byteoffset = TAM_CABECALHO + (long) (RRN_Atual * TAM_REGISTRO);
                fseek(arquivoDadosBIN, byteoffset, SEEK_SET);

                Registro regDados;
                regDados.nomeEstacao = NULL;
                regDados.nomeLinha = NULL;
                LerRegistroBIN(arquivoDadosBIN, &regDados);
                if(regDados.removido == '1'){ // Se o registro estiver logicamente removido, pulamos para o próximo
                    LiberarStringRegistro(&regDados);
                    continue;
                }

                int atendeTodos = 1; //flag de controle de atender os critérios de busca
                // Loop para verificação se o registro atual atende a todos os critérios de busca
                for(int c = 0; c < m_nroCriterios; c++){
                    if(VerificaCriterioBusca(&regDados, criteriosBusca[c].nomeDoCampo, criteriosBusca[c].valorBuscado) != 1){
                        atendeTodos = 0;
                        break;
                    }
                }
                // A lógica anterior da ATUALIZAÇÃO SE REPETE AQUI
                if(atendeTodos == 1){
                    int codEstacaoAntiga = regDados.codEstacao; //guardamos antes de alterar

                    //Aplicamos as atualizações em memória primária
                    AplicarUpdates(&regDados, criteriosUpdates, p_nroUpdates);

                    //Reposicionamos o cursor p/ o início deste registro e sobreescrevemos
                    fseek(arquivoDadosBIN, byteoffset, SEEK_SET);
                    EscreverRegistroBIN(arquivoDadosBIN, &regDados);

                    //Se houve alteração no ID (codEstacao), ajustamos o índice
                    if(regDados.codEstacao != codEstacaoAntiga){
                        RemoverRegistroIndex(&registrosIndex, &totalRegsIndex, codEstacaoAntiga);
                        InserirRegistroIndex(&registrosIndex, regDados.codEstacao, RRN_Atual, &totalRegsIndex);
                    }
                }
                LiberarStringRegistro(&regDados);
            }
        }
    }
    

    //Reescrevemos o índice atualizado em disco e alteramos o arquivo para consistente
    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegsIndex, arquivoIndex);
    free(registrosIndex);
    registrosIndex = NULL;

    // Recalculamos nroEstacoes e nroParesEstacao do zero, pois registros podem ter sido alterados com nomes novos, ou
    // mudados pares de integração, portanto usamos essa função como SEGURANÇA para
    // recalcular tudo e nos dar certeza
    RecalcularContadoresCabecalho(arquivoDadosBIN, &cabecalhoDados);

    // Salvamos o cabeçalho e o alteramos para consistente
    cabecalhoDados.status = '1';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoDados);
    BinarioNaTela(arquivoIndex);
}