#include "funcionalidades.h"


// Função auxiliar que varre o arquivo para descobrir se ainda existe outra estação ativa com este nome
static int VerificaNomeEstacaoExiste(FILE *arquivoDadosBIN, int proxRRN, const char *nomeBuscado, int rrnIgnorado) {
    if (nomeBuscado == NULL) return 0; 

    int existe = 0;

    for (int rrn = 0; rrn < proxRRN; rrn++) {
        if (rrn == rrnIgnorado) continue;

        long byteOffset = TAM_CABECALHO + (rrn * TAM_REGISTRO);
        fseek(arquivoDadosBIN, byteOffset, SEEK_SET);

        Registro reg;
        reg.nomeEstacao = NULL; reg.nomeLinha = NULL;
        LerRegistroBIN(arquivoDadosBIN, &reg);

        if (reg.removido == '0' && reg.nomeEstacao != NULL && strcmp(reg.nomeEstacao, nomeBuscado) == 0) {
            existe = 1; 
            LiberarStringRegistro(&reg);
            break;
        }
        LiberarStringRegistro(&reg);
    }

    return existe;
}

void Delete(char *arquivoDados, char *arquivoIndex, int nroRemocoes) {
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb+");
    FILE *arquivoIndexBIN = fopen(arquivoIndex, "rb+");
    if (arquivoDadosBIN == NULL || arquivoIndexBIN == NULL) {
        MensagemErro();
        if (arquivoDadosBIN) fclose(arquivoDadosBIN);
        if (arquivoIndexBIN) fclose(arquivoIndexBIN);
        return;
    }

    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0') {
        MensagemErro();
        fclose(arquivoDadosBIN); fclose(arquivoIndexBIN);
        return;
    }

    cabecalhoDados.status = '0';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    IndexRegistro *registrosIndex = NULL;
    int totalRegs = 0;
    CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegs, &cabecalhoDados);

    for (int i = 0; i < nroRemocoes; i++) {
        int nroCriterios;
        scanf("%d", &nroCriterios);

        CriterioBusca criterios[nroCriterios];
        LerCriteriosBusca(criterios, nroCriterios);

        int idBuscado = -1;
        for (int j = 0; j < nroCriterios; j++) {
            if (strcmp(criterios[j].nomeDoCampo, "codEstacao") == 0) {
                idBuscado = atoi(criterios[j].valorBuscado);
                break;
            }
        }

        if (idBuscado != -1) { // BUSCA COM ÍNDICE
            int pos = BuscarRegistroIndex(registrosIndex, totalRegs, idBuscado);
            if (pos != -1) {
                int rrnAtual = registrosIndex[pos].RRN;
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
                        // A LÓGICA 100%: Checa se a estação vai sumir de vez
                        int nomeExiste = VerificaNomeEstacaoExiste(arquivoDadosBIN, cabecalhoDados.proxRRN, reg.nomeEstacao, rrnAtual);
                        if (!nomeExiste && cabecalhoDados.nroEstacoes > 0) cabecalhoDados.nroEstacoes--;
                        if (reg.codProxEstacao != -1 && cabecalhoDados.nroParesEstacao > 0) cabecalhoDados.nroParesEstacao--;

                        // Remoção Física
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
        
        else { // BUSCA SEQUENCIAL
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
                        // A LÓGICA 100%
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

    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegs, arquivoIndex);
    free(registrosIndex);

    // Salvando os contadores diretamente
    cabecalhoDados.status = '1';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoDados);
    BinarioNaTela(arquivoIndex);
}

void InsertInto(char *arquivoDados, char *arquivoIndex, int nroInsercoes) {
    FILE *arquivoDadosBIN = fopen(arquivoDados, "rb+");
    FILE *arquivoIndexBIN = fopen(arquivoIndex, "rb+");
    if (arquivoDadosBIN == NULL || arquivoIndexBIN == NULL) {
        MensagemErro();
        if (arquivoDadosBIN) fclose(arquivoDadosBIN);
        if (arquivoIndexBIN) fclose(arquivoIndexBIN);
        return;
    }

    Header cabecalhoDados;
    LerCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);
    if (cabecalhoDados.status == '0') {
        MensagemErro();
        fclose(arquivoDadosBIN); fclose(arquivoIndexBIN);
        return;
    }

    cabecalhoDados.status = '0';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    IndexRegistro *registrosIndex = NULL;
    int totalRegs = 0;
    CarregarIndex(arquivoIndexBIN, &registrosIndex, &totalRegs, &cabecalhoDados);

    for (int i = 0; i < nroInsercoes; i++) {
        Registro novo;
        novo.removido = '0';
        novo.proximo = -1;

        scanf("%d", &novo.codEstacao);

        char buffer[256];

        ScanQuoteString(buffer);
        novo.tamNomeEstacao = strlen(buffer);
        novo.nomeEstacao = novo.tamNomeEstacao > 0 ? strdup(buffer) : NULL;

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

        if (BuscarRegistroIndex(registrosIndex, totalRegs, novo.codEstacao) != -1) {
            LiberarStringRegistro(&novo);
            continue;
        }

        // Checa se o nome é inédito no arquivo antes de inserir
        int nomeExiste = VerificaNomeEstacaoExiste(arquivoDadosBIN, cabecalhoDados.proxRRN, novo.nomeEstacao, -1);
        
        int rrnNovoRegistro;
        if (cabecalhoDados.topo != -1) {
            rrnNovoRegistro = cabecalhoDados.topo;
            long offset = TAM_CABECALHO + (rrnNovoRegistro * TAM_REGISTRO);
            fseek(arquivoDadosBIN, offset + 1, SEEK_SET); 
            int proximoDaPilha;
            fread(&proximoDaPilha, sizeof(int), 1, arquivoDadosBIN);
            cabecalhoDados.topo = proximoDaPilha;
        } else {
            rrnNovoRegistro = cabecalhoDados.proxRRN;
            cabecalhoDados.proxRRN++;
        }

        long offset = TAM_CABECALHO + (rrnNovoRegistro * TAM_REGISTRO);
        fseek(arquivoDadosBIN, offset, SEEK_SET);
        EscreverRegistroBIN(arquivoDadosBIN, &novo);

        InserirRegistroIndex(&registrosIndex, novo.codEstacao, rrnNovoRegistro, &totalRegs);

        // Atualização em Tempo Real 
        if (!nomeExiste) cabecalhoDados.nroEstacoes++;
        if (novo.codProxEstacao != -1) cabecalhoDados.nroParesEstacao++;

        LiberarStringRegistro(&novo);
    }

    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegs, arquivoIndex);
    free(registrosIndex);

    // Salvando os contadores diretamente
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
        return;
    }

    // Lemos os dois cabeçalhos primeiro
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

        if (usarIndice == 1) { // busca binária no índice, caso tenha
            int posVetor = BuscarRegistroIndex(registrosIndex, totalRegsIndex, valorCodEstacao);
            if (posVetor != -1) {
                int RRN_Atual = registrosIndex[posVetor].RRN;
                long byteoffset = TAM_CABECALHO + (long) (RRN_Atual * TAM_REGISTRO);
                fseek(arquivoDadosBIN, byteoffset, SEEK_SET);

                Registro regDados;
                regDados.nomeEstacao = NULL;
                regDados.nomeLinha = NULL;
                LerRegistroBIN(arquivoDadosBIN, &regDados);

                if(regDados.removido != '1') {
                    int atendeTodos = 1; 
                    for(int c = 0; c < m_nroCriterios; c++){
                        if(VerificaCriterioBusca(&regDados, criteriosBusca[c].nomeDoCampo, criteriosBusca[c].valorBuscado) != 1){
                            atendeTodos = 0;
                            break;
                        }
                    }

                    if(atendeTodos == 1){
                        int codEstacaoAntiga = regDados.codEstacao; 

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
                }
                LiberarStringRegistro(&regDados);
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
                if(regDados.removido == '1'){
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
    

    //Reescrevemos o índice atualizado e alteramos o arquivo para consistente
    ReescritaIndex(arquivoIndexBIN, registrosIndex, totalRegsIndex, arquivoIndex);
    free(registrosIndex);
    registrosIndex = NULL;

    // Recalculamos nroEstacoes e nroParesEstacao do zero, pois registros podem ter sido alterados
    RecalcularContadoresCabecalho(arquivoDadosBIN, &cabecalhoDados);

    cabecalhoDados.status = '1';
    EscreverCabecalhoBIN(arquivoDadosBIN, &cabecalhoDados);

    fclose(arquivoDadosBIN);
    fclose(arquivoIndexBIN);

    BinarioNaTela(arquivoDados);
    BinarioNaTela(arquivoIndex);
}