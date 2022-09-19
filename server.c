#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// socket libraries:
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <pthread.h>

#define BUFSZ 500 //tamanho máximo da mensagem

char primeiroEquipamento[5][3]; //matriz para armazenar cada equipamento sendo (ex: '0', '1', '\n') e mais 4 linhas para os possíveis 4 sensores que podem se associar a este
char segundoEquipamento[5][3];
char terceiroEquipamento[5][3];
char quartoEquipamento[5][3];
int quantidadeSensoresPrimeiroEquipamento = 0; //declara globalmente as variáveis que guardam a qnt de sensores total de cada equipamento
int quantidadeSensoresSegundoEquipamento = 0;
int quantidadeSensoresTerceiroEquipamento = 0;
int quantidadeSensoresQuartoEquipamento = 0;
char mensagemParaCliente[BUFSZ]; //declara a mensagem que sempre retorna para o cliente após fazer alguma operação

void error(char *msg) {
    perror(msg);
    exit(1);
}

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]); //como deve rodar no terminal para dar certo
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
    {
        return -1;
    }
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4"))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    }
    else if (0 == strcmp(proto, "v6"))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    }
    else
    {
        return -1;
    }
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET)
    {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
                       INET6_ADDRSTRLEN + 1))
        {
            error("ntop");
        }
        port = ntohs(addr4->sin_port); // network to host short
    }
    else if (addr->sa_family == AF_INET6)
    {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr,
                       INET6_ADDRSTRLEN + 1))
        {
            error("ntop");
        }
        port = ntohs(addr6->sin6_port); // network to host short
    }
    else
    {
        error("unknown protocol family.");
    }
    if (str)
    {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

void adicionarSensor(char matrizDePalavras[11][7], int tamanhoMensagem) {
    char sensor[4][3]; //tamanho do sensor, da mesma forma que é armazenado no equipamento globalmente, quanto às dimensões
    int numeroDeSensores = tamanhoMensagem - 4; // pega a qnt de sensores diminuindo das palavras padrões que vêm no comando de add
    char equipamento[3];
    strcpy(equipamento, matrizDePalavras[tamanhoMensagem - 1]); // pega o equipamento, usando o ultimo espaço da mensagem mandada no comando add
    int i = 2;
    int j = 0;
    while(i < tamanhoMensagem - 2) {
        strcpy(sensor[j], matrizDePalavras[i]); // lê sensores
        j++;
        i++;
    }

    char mensagem[BUFSZ];
    strcpy(mensagem, "sensor ");

    char auxSensorInvalidoMensagem[BUFSZ];
    strcpy(auxSensorInvalidoMensagem, "");

    int auxSensor = 0;
    while(auxSensor < numeroDeSensores) { // verifica se os sensores inseridos são válidos (está função está em todas as funções que exigem o usuário de inserir sensores)
        if (strncmp(sensor[auxSensor], "01", 2) != 0 &&
            strncmp(sensor[auxSensor], "02", 2) != 0 &&
            strncmp(sensor[auxSensor], "03", 2) != 0 && 
            strncmp(sensor[auxSensor], "04", 2) != 0) {
                strcat(auxSensorInvalidoMensagem, "invalid sensor");
                break;
        }
        auxSensor++;
    }

    char auxLimiteSensoresMensagem[BUFSZ];
    strcpy(auxLimiteSensoresMensagem, "limit exceeded");
    if ( //verifica se a quantidade de sensores já ultrapassou a quantidade máxima que é 15
        ((quantidadeSensoresQuartoEquipamento + 
        quantidadeSensoresTerceiroEquipamento + 
        quantidadeSensoresSegundoEquipamento + 
        quantidadeSensoresPrimeiroEquipamento) + numeroDeSensores) > 15
    ) {
        strcpy(mensagem, auxLimiteSensoresMensagem);
    }
    else {
        int x = 0;
        int y = 1;
        int sensorExiste = 0;

        char stringAuxiliarDosSensoresAdicionados[BUFSZ];
        char stringComEspaco[BUFSZ];

        char auxSensorExisteMensagem[BUFSZ];
        strcpy(auxSensorExisteMensagem, "");

        char auxSensorNaoExisteMensagem[BUFSZ];
        strcpy(auxSensorNaoExisteMensagem, "");
    
        if (strncmp(equipamento, "01", 2) == 0) { // caso o usuário passe o equipamento 01 para operar sobre
        while(x < numeroDeSensores) { //percorre todos os sensores passados
        y = 1;
            while(y < 5) { //percorre todas as 4 possíveis posições de inserção de sensores da matriz
                if (strncmp(primeiroEquipamento[y], sensor[x], 2) == 0) {
                    strcat(auxSensorExisteMensagem, sensor[x]);
                    strcat(auxSensorExisteMensagem, " "); // concatena espaços
                    sensorExiste++;
                    break;
                }
                y++;
            }
            if (!sensorExiste) { //se o sensor não for achado
                strcpy(primeiroEquipamento[quantidadeSensoresPrimeiroEquipamento + 1], sensor[x]);// coloca o sensor passado na matriz na posição
                quantidadeSensoresPrimeiroEquipamento++; //incrementa um sensor globalmente para o equipamento
                strcat(auxSensorNaoExisteMensagem, sensor[x]);
                strcat(auxSensorNaoExisteMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }
        // mesma coisa acima que foi feita para o equipamento 01, faz para o equipamento 02, 03 ou 04, dependendo de qual o usuário esteja referenciando
        if (strncmp(equipamento, "02", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < 5) {
                if (strncmp(segundoEquipamento[y], sensor[x], 2) == 0) {
                    strcat(auxSensorExisteMensagem, sensor[x]);
                    strcat(auxSensorExisteMensagem, " ");
                    sensorExiste++;
                    break;
                }
                y++;
            }
            if (!sensorExiste) {
                strcpy(segundoEquipamento[quantidadeSensoresSegundoEquipamento + 1], sensor[x]);
                quantidadeSensoresSegundoEquipamento++;
                strcat(auxSensorNaoExisteMensagem, sensor[x]);
                strcat(auxSensorNaoExisteMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

        if (strncmp(equipamento, "03", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < 5) {
                if (strncmp(terceiroEquipamento[y], sensor[x], 2) == 0) {
                    strcat(auxSensorExisteMensagem, sensor[x]);
                    strcat(auxSensorExisteMensagem, " ");
                    sensorExiste++;
                    break;
                }
                y++;
            }
            if (!sensorExiste) {
                strcpy(terceiroEquipamento[quantidadeSensoresTerceiroEquipamento + 1], sensor[x]);
                quantidadeSensoresTerceiroEquipamento++;
                strcat(auxSensorNaoExisteMensagem, sensor[x]);
                strcat(auxSensorNaoExisteMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

        if (strncmp(equipamento, "04", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < 5) {
                if (strncmp(quartoEquipamento[y], sensor[x], 2) == 0) {
                    printf("existe\n");
                    sensorExiste++;
                }
                y++;
            }
            if (!sensorExiste) {
                strcpy(primeiroEquipamento[quantidadeSensoresQuartoEquipamento + 1], sensor[x]);
                quantidadeSensoresQuartoEquipamento++;
                strcat(auxSensorNaoExisteMensagem, sensor[x]);
                strcat(auxSensorNaoExisteMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if(strlen(auxSensorInvalidoMensagem) > 0) { //se teve mensagem de erro, manda para o cliente que foi inválido o que ele passou
        strcpy(mensagemParaCliente, auxSensorInvalidoMensagem);
    }
    else {
        if(strlen(auxSensorNaoExisteMensagem) > 0) { //verifica se o auxiliar de sensor nao existe foi ativado, para mudar o retorno da chamada
            strcat(mensagem, auxSensorNaoExisteMensagem);
            strcat(mensagem, "added");
        }
        if(strlen(auxSensorExisteMensagem) > 0) {
            if (strlen(auxSensorNaoExisteMensagem) > 0) { // if interno serve para ver se foi chamado tanto sensor que já existia, como algum que não existia ainda
                strcat(mensagem, " ");
            }   
            strcat(mensagem, auxSensorExisteMensagem);
            strcat(mensagem, "already exists");
        }
        strcat(mensagem, " in ");
        strcat(mensagem, equipamento);
    char* removendoEspacos = strtok(mensagem, "\n"); //removendo \n para imprimir
    strcpy(mensagemParaCliente, removendoEspacos);
    }
    }
}

void listarSensores(char matrizDePalavras[11][7], int tamanhoMensagem) {
    char equipamento[3];
    int i = 1;

    char auxListagemSensoresMensagem[BUFSZ];
    strcpy(auxListagemSensoresMensagem, "");

    char mensagem[BUFSZ];
    strcpy(mensagem, "");

    strcpy(equipamento, matrizDePalavras[tamanhoMensagem - 1]); //pegando equipamento via parametros passados
    if (strncmp(equipamento, "01", 2) == 0) {
        if (quantidadeSensoresPrimeiroEquipamento == 0) { //verifica se tem sensores naquele equipamento para listar
            strcpy(mensagem, "none"); //printa none se nao tiver nada
        }
        else {
            while(i < quantidadeSensoresPrimeiroEquipamento + 1) {
                if (strncmp(primeiroEquipamento[i], "99", 2) == 0) { // verifica se o valor que está naquele momento é o 99, que adotamos como valor apagado, dentro da função removeSensor
                    printf("sensor apagado");
                } 
                else {
                    strcat(auxListagemSensoresMensagem, primeiroEquipamento[i]);
                    strcat(auxListagemSensoresMensagem, " ");
                }
                i++;
            }   
        }
    }

    if (strncmp(equipamento, "02", 2) == 0) {
        if (quantidadeSensoresSegundoEquipamento == 0) {
            strcpy(mensagem, "none");
        }
        else {
            while(i < quantidadeSensoresSegundoEquipamento + 1) {
                if (strncmp(segundoEquipamento[i], "99", 2) == 0) {
                    printf("sensor apagado");
                } 
                else {
                    strcat(auxListagemSensoresMensagem, segundoEquipamento[i]);
                    strcat(auxListagemSensoresMensagem, " ");
                }
                i++;
            }   
        }
    }

    if (strncmp(equipamento, "03", 2) == 0) {
        if (quantidadeSensoresTerceiroEquipamento == 0) {
            strcpy(mensagem, "none");
        }
        else {
            while(i < quantidadeSensoresTerceiroEquipamento + 1) {
                if (strncmp(terceiroEquipamento[i], "99", 2) == 0) {
                    printf("sensor apagado");
                } 
                else {
                    strcat(auxListagemSensoresMensagem, terceiroEquipamento[i]);
                    strcat(auxListagemSensoresMensagem, " ");
                }
                i++;
            }   
        }
    }

    if (strncmp(equipamento, "04", 2) == 0) {
        if (quantidadeSensoresQuartoEquipamento == 0) {
            strcpy(mensagem, "none");
        }
        else {
            while(i < quantidadeSensoresQuartoEquipamento + 1) {
                if (strncmp(quartoEquipamento[i], "99", 2) == 0) {
                    printf("sensor apagado");
                } 
                else {
                    strcat(auxListagemSensoresMensagem, quartoEquipamento[i]);
                    strcat(auxListagemSensoresMensagem, " ");
                }
                i++;
            }   
        }
    }

    if (strlen(auxListagemSensoresMensagem) > 0) {
        strcpy(mensagem, auxListagemSensoresMensagem);
    }

    strcpy(mensagemParaCliente, mensagem);
}

void removerSensor(char matrizDePalavras[11][7], int tamanhoMensagem) {
    char sensor[4][3];
    int numeroDeSensores = tamanhoMensagem - 4; 
    char equipamento[3];

    char auxRemoverSensorExistenteMensagem[BUFSZ];
    strcpy(auxRemoverSensorExistenteMensagem, "");

    char auxRemoverSensorNaoExistenteMensagem[BUFSZ];
    strcpy(auxRemoverSensorNaoExistenteMensagem, "");

    char mensagem[BUFSZ];
    strcpy(mensagem, "sensor ");

    strcpy(equipamento, matrizDePalavras[tamanhoMensagem - 1]);
    int i = 2;
    int j = 0;
    while(i < tamanhoMensagem - 2) {
        strcpy(sensor[j], matrizDePalavras[i]);
        j++;
        i++;
    }

    char auxSensorInvalidoMensagem[BUFSZ];
    strcpy(auxSensorInvalidoMensagem, "");

    int auxSensor = 0;
    while(auxSensor < numeroDeSensores) {
        if (strncmp(sensor[auxSensor], "01", 2) != 0 &&
            strncmp(sensor[auxSensor], "02", 2) != 0 &&
            strncmp(sensor[auxSensor], "03", 2) != 0 && 
            strncmp(sensor[auxSensor], "04", 2) != 0) {
                strcat(auxSensorInvalidoMensagem, "invalid sensor");
                break;
        }
        auxSensor++;
    }

    int x = 0;
    int y = 1;
    int sensorExiste = 0;
    if (strncmp(equipamento, "01", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < 5) {
                if (strncmp(primeiroEquipamento[y], sensor[x], 2) == 0) { //esta função ordena os sensores deixando o apagado por ultimo, porque como faz o equipamentos--, no list este vai ser ignorado ao percorrer o vetor, já que estaria na ultima opção
                    sensorExiste++;
                    strcpy(primeiroEquipamento[y], "99"); //99 valor que nao pode ser lido
                    if(strcmp(primeiroEquipamento[y+1], "99" )!=0 || strcmp(primeiroEquipamento[y+1],"")!=0) {
                        strcpy(primeiroEquipamento[y],primeiroEquipamento[y+1]);
                        strcpy(primeiroEquipamento[y+1], "99");
                        if(strcmp(primeiroEquipamento[y+2], "99") !=0 || strcmp(primeiroEquipamento[y+2], "") !=0) {
                            strcpy(primeiroEquipamento[y+1], primeiroEquipamento[y+2]);
                            strcpy(primeiroEquipamento[y+2], "99");
                                if(strcmp(primeiroEquipamento[y+3], "99") != 0 || strcmp(primeiroEquipamento[y+3], "") != 0) {
                                strcpy(primeiroEquipamento[y+2], primeiroEquipamento[y+3]);
                                strcpy(primeiroEquipamento[y+3], "99");
                                }
                        }
                    }
                    strcat(auxRemoverSensorExistenteMensagem, sensor[x]);
                    strcat(auxRemoverSensorExistenteMensagem, " ");
                    quantidadeSensoresPrimeiroEquipamento--; //decai uma valor para a quantidade de sensores do equipamento que está tratando
                }
                y++;
            }
            if (!sensorExiste) {
                strcat(auxRemoverSensorNaoExistenteMensagem, sensor[x]);
                strcat(auxRemoverSensorNaoExistenteMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if (strncmp(equipamento, "02", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < 5) {
                if (strncmp(segundoEquipamento[y], sensor[x], 2) == 0) {
                    sensorExiste++;
                    strcpy(segundoEquipamento[y], "99"); //99 valor que nao pode ser lido
                    if(strcmp(segundoEquipamento[y+1], "99" )!=0 || strcmp(segundoEquipamento[y+1],"")!=0) {
                        strcpy(segundoEquipamento[y],segundoEquipamento[y+1]);
                        strcpy(segundoEquipamento[y+1], "99");
                        if(strcmp(segundoEquipamento[y+2], "99") !=0 || strcmp(segundoEquipamento[y+2], "") !=0) {
                            strcpy(segundoEquipamento[y+1], segundoEquipamento[y+2]);
                            strcpy(segundoEquipamento[y+2], "99");
                                if(strcmp(segundoEquipamento[y+3], "99") != 0 || strcmp(segundoEquipamento[y+3], "") != 0) {
                                strcpy(segundoEquipamento[y+2], segundoEquipamento[y+3]);
                                strcpy(segundoEquipamento[y+3], "99");
                                }
                        }
                    }
                    strcat(auxRemoverSensorExistenteMensagem, sensor[x]);
                    strcat(auxRemoverSensorExistenteMensagem, " ");
                    quantidadeSensoresSegundoEquipamento--;
                }
                y++;
            }
            if (!sensorExiste) {
                strcat(auxRemoverSensorNaoExistenteMensagem, sensor[x]);
                strcat(auxRemoverSensorNaoExistenteMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if (strncmp(equipamento, "03", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < 5) {
                if (strncmp(terceiroEquipamento[y], sensor[x], 2) == 0) {
                    sensorExiste++;
                    strcpy(terceiroEquipamento[y], "99"); //99 valor que nao pode ser lido
                    if(strcmp(terceiroEquipamento[y+1], "99" )!=0 || strcmp(terceiroEquipamento[y+1],"")!=0) {
                        strcpy(terceiroEquipamento[y],terceiroEquipamento[y+1]);
                        strcpy(terceiroEquipamento[y+1], "99");
                        if(strcmp(terceiroEquipamento[y+2], "99") !=0 || strcmp(terceiroEquipamento[y+2], "") !=0) {
                            strcpy(terceiroEquipamento[y+1], terceiroEquipamento[y+2]);
                            strcpy(terceiroEquipamento[y+2], "99");
                                if(strcmp(terceiroEquipamento[y+3], "99") != 0 || strcmp(terceiroEquipamento[y+3], "") != 0) {
                                strcpy(terceiroEquipamento[y+2], terceiroEquipamento[y+3]);
                                strcpy(terceiroEquipamento[y+3], "99");
                                }
                        }
                    }
                    strcat(auxRemoverSensorExistenteMensagem, sensor[x]);
                    strcat(auxRemoverSensorExistenteMensagem, " ");
                    quantidadeSensoresTerceiroEquipamento--;
                }
                y++;
            }
            if (!sensorExiste) {
                strcat(auxRemoverSensorNaoExistenteMensagem, sensor[x]);
                strcat(auxRemoverSensorNaoExistenteMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if (strncmp(equipamento, "04", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < 5) {
                if (strncmp(quartoEquipamento[y], sensor[x], 2) == 0) {
                    sensorExiste++;
                    strcpy(quartoEquipamento[y], "99"); //99 valor que nao pode ser lido
                    if(strcmp(quartoEquipamento[y+1], "99" )!=0 || strcmp(quartoEquipamento[y+1],"")!=0) {
                        strcpy(quartoEquipamento[y],quartoEquipamento[y+1]);
                        strcpy(quartoEquipamento[y+1], "99");
                        if(strcmp(quartoEquipamento[y+2], "99") !=0 || strcmp(quartoEquipamento[y+2], "") !=0) {
                            strcpy(quartoEquipamento[y+1], quartoEquipamento[y+2]);
                            strcpy(quartoEquipamento[y+2], "99");
                                if(strcmp(quartoEquipamento[y+3], "99") != 0 || strcmp(quartoEquipamento[y+3], "") != 0) {
                                strcpy(quartoEquipamento[y+2], quartoEquipamento[y+3]);
                                strcpy(quartoEquipamento[y+3], "99");
                                }
                        }
                    }
                    strcat(auxRemoverSensorExistenteMensagem, sensor[x]);
                    strcat(auxRemoverSensorExistenteMensagem, " ");
                    quantidadeSensoresQuartoEquipamento--;
                }
                y++;
            }
            if (!sensorExiste) {
                strcat(auxRemoverSensorNaoExistenteMensagem, sensor[x]);
                strcat(auxRemoverSensorNaoExistenteMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if (strlen(auxSensorInvalidoMensagem) > 0) {
        strcpy(mensagemParaCliente, auxSensorInvalidoMensagem); //manda mensagem caso mandou algum sensor invalido
    }
    else {
        if(strlen(auxRemoverSensorExistenteMensagem) > 0) {
            strcat(auxRemoverSensorExistenteMensagem, "removed");
            strcat(mensagem, auxRemoverSensorExistenteMensagem);
        }

        if(strlen(auxRemoverSensorNaoExistenteMensagem) > 0) {
            if (strlen(auxRemoverSensorExistenteMensagem) > 0) { // verifica se mandou um sensor já existente e um que não existia ao mesmo tempo
                strcat(mensagem, " ");
            }   
            strcat(mensagem, auxRemoverSensorNaoExistenteMensagem);
            strcat(mensagem, "does not exist in ");
            strcat(mensagem, equipamento);
        }
        char* removendoEspacos = strtok(mensagem, "\n");
        strcpy(mensagemParaCliente, removendoEspacos);
    }
}

void lerSensor(char matrizDePalavras[11][7], int tamanhoMensagem) {
    char sensor[4][3];
    int numeroDeSensores = tamanhoMensagem - 3;
    char equipamento[3];
    int sensorExiste = 0;

    char mensagem[BUFSZ];
    strcpy(mensagem, "");

    char auxLerMensagem[BUFSZ];
    strcpy(auxLerMensagem, "");

    char auxLerSensorNaoInstaladoMensagem[BUFSZ];
    strcpy(auxLerSensorNaoInstaladoMensagem, "");

    strcpy(equipamento, matrizDePalavras[tamanhoMensagem - 1]);
    int i = 1;
    int j = 0;
    while(i < tamanhoMensagem - 2) {
        strcpy(sensor[j], matrizDePalavras[i]);
        j++;
        i++;
    }

    char auxSensorInvalidoMensagem[BUFSZ];
    strcpy(auxSensorInvalidoMensagem, "");

    int auxSensor = 0;
    while(auxSensor < numeroDeSensores) {
        if (strncmp(sensor[auxSensor], "01", 2) != 0 &&
            strncmp(sensor[auxSensor], "02", 2) != 0 &&
            strncmp(sensor[auxSensor], "03", 2) != 0 && 
            strncmp(sensor[auxSensor], "04", 2) != 0) {
                strcat(auxSensorInvalidoMensagem, "invalid sensor");
                break;
        }
        auxSensor++;
    }


    int x = 0;
    int y = 1;
    if (strncmp(equipamento, "01", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < quantidadeSensoresPrimeiroEquipamento + 1) {
                if (strncmp(primeiroEquipamento[y], sensor[x], 2) == 0) {
                    sensorExiste++;
                    float numeroAleatorio = rand() %10; //gera valor aleatório entre 0 e 10, sendo float com 2 casas de precisão
                    char numeroAleatorioChar[6];
                    sprintf(numeroAleatorioChar, "%.2f", numeroAleatorio);

                    strcat(auxLerMensagem, numeroAleatorioChar);
                    strcat(auxLerMensagem, " ");
                }
                y++;
            }
            if (!sensorExiste) {
                strcat(auxLerSensorNaoInstaladoMensagem, "sensor(s) ");
                strcat(auxLerSensorNaoInstaladoMensagem, sensor[x]);
                strcat(auxLerSensorNaoInstaladoMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if (strncmp(equipamento, "02", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < quantidadeSensoresSegundoEquipamento + 1) {
                if (strncmp(segundoEquipamento[y], sensor[x], 2) == 0) {
                    sensorExiste++;
                    float numeroAleatorio = rand() %10;
                    char numeroAleatorioChar[6];
                    sprintf(numeroAleatorioChar, "%.2f", numeroAleatorio);

                    strcat(auxLerMensagem, numeroAleatorioChar);
                    strcat(auxLerMensagem, " ");
                }
                y++;
            }
            if (!sensorExiste) {
                strcat(auxLerSensorNaoInstaladoMensagem, "sensor(s) ");
                strcat(auxLerSensorNaoInstaladoMensagem, sensor[x]);
                strcat(auxLerSensorNaoInstaladoMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if (strncmp(equipamento, "03", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < quantidadeSensoresTerceiroEquipamento + 1) {
                if (strncmp(terceiroEquipamento[y], sensor[x], 2) == 0) {
                    sensorExiste++;
                    float numeroAleatorio = rand() %10;
                    char numeroAleatorioChar[6];
                    sprintf(numeroAleatorioChar, "%.2f", numeroAleatorio);

                    strcat(auxLerMensagem, numeroAleatorioChar);
                    strcat(auxLerMensagem, " ");
                }
                y++;
            }
            if (!sensorExiste) {
                strcat(auxLerSensorNaoInstaladoMensagem, "sensor(s) ");
                strcat(auxLerSensorNaoInstaladoMensagem, sensor[x]);
                strcat(auxLerSensorNaoInstaladoMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if (strncmp(equipamento, "04", 2) == 0) {
        while(x < numeroDeSensores) {
        y = 1;
            while(y < quantidadeSensoresQuartoEquipamento + 1) {
                if (strncmp(quartoEquipamento[y], sensor[x], 2) == 0) {
                    sensorExiste++;
                    float numeroAleatorio = rand() %10;
                    char numeroAleatorioChar[6];
                    sprintf(numeroAleatorioChar, "%.2f", numeroAleatorio);

                    strcat(auxLerMensagem, numeroAleatorioChar);
                    strcat(auxLerMensagem, " ");
                }
                y++;
            }
            if (!sensorExiste) {
                strcat(auxLerSensorNaoInstaladoMensagem, "sensor(s) ");
                strcat(auxLerSensorNaoInstaladoMensagem, sensor[x]);
                strcat(auxLerSensorNaoInstaladoMensagem, " ");
            }
            x++;
            sensorExiste = 0;
        }
    }

    if (strlen(auxSensorInvalidoMensagem) > 0) {
            strcpy(mensagemParaCliente, auxSensorInvalidoMensagem);
    }
    else {
        if(strlen(auxLerMensagem) > 0) {
            strcat(mensagem, auxLerMensagem);
        }

        if(strlen(auxLerSensorNaoInstaladoMensagem) > 0) {
            if (strlen(auxLerMensagem) > 0) {
                strcat(mensagem, "and ");
                strcat(mensagem, auxLerSensorNaoInstaladoMensagem);
                strcat(mensagem, "not installed");
            }
            else {
                strcat(mensagem, auxLerSensorNaoInstaladoMensagem);
                strcat(mensagem, "not installed");
            }
        }
        strcpy(mensagemParaCliente, mensagem);
    }
}

void direcionaOperacao(char buf[]) {
    char matrizDePalavras[11][7];
    char* comando = strtok(buf, " ");
    int i = 0;
    while (comando != NULL) { //vai lendo todo o comando passado até chegar no final
        strcpy(matrizDePalavras[i], comando);
        comando = strtok(NULL, " ");
        i++;
    }
    char* sensor = strtok(NULL, " ");

    char mensagem[BUFSZ];
    strcpy(mensagem, "sensor ");
    if (strncmp(matrizDePalavras[i - 1], "01", 2) != 0 && //verifica se não passou algum equipamento invalido, pois se passa mandar mensagem de invalid equipament
        strncmp(matrizDePalavras[i - 1], "02", 2) != 0 &&
        strncmp(matrizDePalavras[i - 1], "03", 2) != 0 &&
        strncmp(matrizDePalavras[i - 1], "04", 2) != 0 &&
        strncmp(matrizDePalavras[i - 1], "kill", 4) != 0) //kill aqui também para que quando digite apenas kill nao caia na condicional, e retorne "invalid equipament"
        {
            strcpy(mensagem, "invalid equipment");
            strcpy(mensagemParaCliente, mensagem);
    }
    else { //direciona a operação para cada local a depender do que o usuario colocou no prompt de comando
        if (strncmp(matrizDePalavras[0], "add", 3) == 0) {
            adicionarSensor(matrizDePalavras, i); // passa para cada função a mensagem que o user colocou "quebrada" e o tamanho desta mensagem
        }
        else if (strncmp(matrizDePalavras[0], "list", 4) == 0) {
            listarSensores(matrizDePalavras, i);
        }
        else if (strncmp(matrizDePalavras[0], "remove", 6) == 0) {
            removerSensor(matrizDePalavras, i);
        }
        else if (strncmp(matrizDePalavras[0], "read", 4) == 0) {
            lerSensor(matrizDePalavras, i);
        }
        else if (strncmp(matrizDePalavras[0], "kill", 4) == 0) {
            exit(EXIT_SUCCESS);
        }
        else {
            exit(EXIT_SUCCESS); // se colocar qualquer comando invalido, vai desconectar, como "ad sensor 02 in 01"
        }
    }
}

int main(int argc, char *argv[])
{
    // Receber quantidade certa de argumentos
    if (argc < 3)
    {
        error("erro");
    }

    // Exibir uso correto
    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    // Criando Socket
    int mysock = socket(storage.ss_family, SOCK_STREAM, 0);
    if (mysock == -1)
    {
        error("Socket");
    }

    // Ajudar na reutilização de portas Setsockopt
    int enable = 1;
    if (0 != setsockopt(mysock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        error("Setsockopt");
    }

    // Fazer o bind
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(mysock, addr, sizeof(storage)))
    {
        error("Bind");
    }

    // Fazer  o listen
    if (0 != listen(mysock, 10))
    {
        error("listen");
    }

    // Esperar a conexão
    char addrstr[500];
    addrtostr(addr, addrstr, 500);
    printf("bound to %s, waiting connections\n", addrstr);

    while (1)
    {
        strcpy(primeiroEquipamento[0], "01");
        strcpy(segundoEquipamento[0], "02");
        strcpy(terceiroEquipamento[0], "03");
        strcpy(quartoEquipamento[0], "04");

        int valread;
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(mysock, caddr, &caddrlen);
        if (csock == -1)
        {
            error("accept");
        }

        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);
        printf("[log] connection from %s\n", caddrstr);

        char buf[BUFSZ];
        char *hello = "Hello from server";

        while (1)
        {
            memset(buf, 0, BUFSZ);
            //memset(auxmsg, 0, BUFSZ);
            int valread = read(csock, buf, 1024);
            direcionaOperacao(buf);
            //tokenize(buf);
            //send(csock, auxmsg, strlen(auxmsg), 0);
            send(csock, mensagemParaCliente, strlen(mensagemParaCliente), 0);
        }
        close(csock);
    }

    exit(EXIT_SUCCESS);
}