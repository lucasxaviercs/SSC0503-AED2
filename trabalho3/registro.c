#include "registro.h"


#define SEPARADOR_CAMPOS ","

/*Lê um campo de tamanho fixo separado por vírgula no CSV.
Retornando o valor numérico correspondente ao campo para o registro
ou -1 para valores vazios ou "NULO"*/ 
static int LerCampoFixo(char **linha){
    char *campo = strsep(linha, SEPARADOR_CAMPOS);

    // Os valores de campos nulos devem ser representados pelo valor -1
    if(campo == NULL || campo[0] == '\0' || strcmp(campo, "NULO") == 0) return -1;

    return (int)strtol(campo, NULL, 10);
}

/*Lê um campo de tamanho variável que é separado por vírgula no CSV,
alocando a memória necessária. Retornando um ponteiro para a string
ou NULL para quando aquele campo estiver vazio ou NULO*/
static char *LerCampoVariavel(char **linha){
    char *campo = strsep(linha, SEPARADOR_CAMPOS);

    if(campo == NULL || campo[0] == '\0' || strcmp(campo, "NULO") == 0) return NULL;

    char *nome = malloc( (strlen(campo) + 1) * sizeof(char) );
    strcpy(nome, campo);
    return nome;
}

/*Registra o indicador de tamanho e a string em sequência no arquivo binário.
Retornando a quantidade total de bytes ocupados, para facilitar na função de preencher
com lixo o arquivo binário*/
static int EscreverStringVariavelBIN(FILE *arquivoBIN, int tamanho, const char *string){
    int espacoUtilizado = 0;
        
    // Escreve o indicador de tamanho (4 bytes) primeiro
    fwrite(&tamanho, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
        
    // Se o tamanho for maior que 0, escreve a string (sem o '\0')
    if (tamanho > 0 && string != NULL) {
        fwrite(string, sizeof(char), tamanho, arquivoBIN);
        espacoUtilizado += tamanho;
    }
        
    return espacoUtilizado;
}

/*Caso ainda há bytes restantes do TAM_REGISTRO (80 bytes), preenche com lixo '$'
diretamente no arquivo para completar os 80 bytes.*/
static void PreencherComLixoBIN(FILE *arquivoBIN, int espacoUtilizado, int tamanhoTotal){
    char lixo = '$';
    int bytesRestantes = tamanhoTotal - espacoUtilizado;
        
    for (int i = 0; i < bytesRestantes; i++){
        fwrite(&lixo, sizeof(char), 1, arquivoBIN);
    }
}


/*Lê sequencialmente os 80 bytes de um registro do arquivo e aloca suas strings por meio do fread.
Preenchendo os dados na struct pelo endereço de memória passado como argumento na chamada da função*/
void LerRegistroBIN(FILE *arquivoBIN, Registro *registroDados){
    // Para não ficar lendo LIXO '$'
    int espacoUtilizado = 0;
   
    // Leitura do campo de remoção
    fread(&registroDados->removido, sizeof(char), 1, arquivoBIN);
    espacoUtilizado += sizeof(char);
    // Checagem se o registro está marcado como removido ou não
    if(registroDados->removido == '1'){// se removido
        // Mesmo quando estiver removido, é preciso
        // guardar o número do próximo registro para não haver 
        // quebra da lógica do topo da pilha
        fread(&registroDados->proximo, sizeof(int), 1, arquivoBIN);
        espacoUtilizado += sizeof(int);

        // Pula o que sobrou de bytes
        fseek(arquivoBIN, TAM_REGISTRO - espacoUtilizado, SEEK_CUR); // altera o cursor para o próximo registro
        registroDados->nomeEstacao = NULL; // prevenção de lixo nos ponteiros
        registroDados->nomeLinha = NULL;   // prevenção de lixo nos ponteiros
        return;
    }
    
    // Leitura dos campos de tamanho fixo
    fread(&registroDados->proximo, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
    fread(&registroDados->codEstacao, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
    fread(&registroDados->codLinha, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
    fread(&registroDados->codProxEstacao, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
    fread(&registroDados->distProxEstacao, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
    fread(&registroDados->codLinhaIntegra, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
    fread(&registroDados->codEstIntegra, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);

    // Leitura dos campos de tamanho variável:
    // primeiro lê o int que indica o tamanho, depois aloca e lê a string
    fread(&registroDados->tamNomeEstacao, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
    if(registroDados->tamNomeEstacao > 0){
        registroDados->nomeEstacao = malloc((registroDados->tamNomeEstacao + 1) * sizeof(char));
        fread(registroDados->nomeEstacao, sizeof(char), registroDados->tamNomeEstacao, arquivoBIN);
        registroDados->nomeEstacao[registroDados->tamNomeEstacao] = '\0';
        espacoUtilizado += registroDados->tamNomeEstacao;
    } else {
        registroDados->nomeEstacao = NULL;
    }

    fread(&registroDados->tamNomeLinha, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);
    if(registroDados->tamNomeLinha > 0){
        registroDados->nomeLinha = malloc((registroDados->tamNomeLinha + 1) * sizeof(char));
        fread(registroDados->nomeLinha, sizeof(char), registroDados->tamNomeLinha, arquivoBIN);
        registroDados->nomeLinha[registroDados->tamNomeLinha] = '\0';
        espacoUtilizado += registroDados->tamNomeLinha;
    } else {
        registroDados->nomeLinha = NULL;
    }

    // PULAR OS '$' NA LEITURA DOS 80 BYTES
    int bytesRestantes = TAM_REGISTRO - espacoUtilizado;
    if(bytesRestantes > 0){
        fseek(arquivoBIN, bytesRestantes, SEEK_CUR);
    }
}

/*Consome uma linha do CSV, separando os campos por vírgula, e colocando
esses dados extraídos nos seus respectivos campos da struct*/
void LerRegistroCSV(FILE *arquivoCSV, Registro *registroDados){
    char buffer[512];
    if(fgets(buffer, 512, arquivoCSV) == NULL){
        registroDados->removido = '1';
        return;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';

    // PREVENÇÃO
    if(strlen(buffer) == 0){
        registroDados->removido = '1';
        return;
    }

    char *linha = buffer;

    registroDados->removido = '0';
    registroDados->proximo = -1;

    registroDados->codEstacao = LerCampoFixo(&linha);
    registroDados->nomeEstacao = LerCampoVariavel(&linha);
    registroDados->tamNomeEstacao = (registroDados->nomeEstacao == NULL) ? 0 : strlen(registroDados->nomeEstacao);

    registroDados->codLinha = LerCampoFixo(&linha);
    registroDados->nomeLinha = LerCampoVariavel(&linha);
    registroDados->tamNomeLinha = (registroDados->nomeLinha == NULL) ? 0 : strlen(registroDados->nomeLinha);

    registroDados->codProxEstacao = LerCampoFixo(&linha);
    registroDados->distProxEstacao = LerCampoFixo(&linha);
    registroDados->codLinhaIntegra = LerCampoFixo(&linha);
    registroDados->codEstIntegra = LerCampoFixo(&linha);
}

/*Os campos que foram salvos no registro (memória primária), agora serão
escritos nos arquivos (memória secundária) e em caso de sobrar bytes nos 80 sequenciais,
completa eles preenchendo com lixo '$'*/
void EscreverRegistroBIN(FILE *arquivoBIN, const Registro *registroDados){
    int espacoUtilizado = 0;

    // ESCREVE SEGUINDO A ORDEM DOS CAMPOS NO REGISTRO
    // Escreve os campos de tamanho fixo
    fwrite(&registroDados->removido, sizeof(char), 1, arquivoBIN);
    espacoUtilizado += sizeof(char);

    fwrite(&registroDados->proximo, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);

    fwrite(&registroDados->codEstacao, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);

    fwrite(&registroDados->codLinha, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);

    fwrite(&registroDados->codProxEstacao, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);

    fwrite(&registroDados->distProxEstacao, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);

    fwrite(&registroDados->codLinhaIntegra, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);

    fwrite(&registroDados->codEstIntegra, sizeof(int), 1, arquivoBIN);
    espacoUtilizado += sizeof(int);

    // Escrever os campos de tamanho variável escrevendo antes em um campo de 4 bytes o tamanho do campo.
    espacoUtilizado += EscreverStringVariavelBIN(arquivoBIN, registroDados->tamNomeEstacao, registroDados->nomeEstacao);
    espacoUtilizado += EscreverStringVariavelBIN(arquivoBIN, registroDados->tamNomeLinha, registroDados->nomeLinha);

    // Preenche o bytes não ocupados com lixo ('$')
    PreencherComLixoBIN(arquivoBIN, espacoUtilizado, TAM_REGISTRO);
}