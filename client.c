#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// socket libraries:
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <pthread.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

void usage(int argc, char **argv)
{
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize)
{
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

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage)
{
    if (addrstr == NULL || portstr == NULL)
    {
        // returns error case port is not found
    }
    // parse:
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    // atoi = string to integer
    if (port == 0)
    {
        return -1;
        // wrong information transmisted (there is no port 0)
    }
    port = htons(port); // host to network short

    struct in_addr inaddr4; // IP address
    if (inet_pton(AF_INET, addrstr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // IPv6 address
    if (inet_pton(AF_INET6, addrstr, &inaddr6))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

#define BUFSZ 500

int main(int argc, char **argv)
{
    // Receber quantidade certa de argumentos
    if (argc < 3)
    {
        usage(argc, argv);
    }

    // Exibir uso correto
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    // Cria socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        error("socket");
    }

    // Conecta
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage)))
    {
        error("connect");
    }

    // Define conexão
    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    printf("connected to %s\n", addrstr);

    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);

    size_t count;
    unsigned total = 0;

    while (1)
    {
        printf(">");
        fgets(buf, BUFSZ - 1, stdin);

        count = send(s, buf, strlen(buf), 0); // não manda o caracter '\0'
        if (count != strlen(buf))
        {
            error("send");
        }

        memset(buf, 0, BUFSZ); // esvazia o buffer
        total = 0;
        count = recv(s, buf + total, BUFSZ - total, 0); // recebendo dados do servidor
        puts(buf);
        if (count == 0)
        {
            break;
        }
        total += count;
        memset(buf, 0, BUFSZ);
    }
    close(s); // fecha socket

    printf("received %u bytes\n", total);

    exit(EXIT_SUCCESS);
}