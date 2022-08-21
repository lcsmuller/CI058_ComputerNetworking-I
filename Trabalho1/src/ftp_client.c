#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftp.h"

/** @brief Estrutura do cliente */
struct ftp_client {
    /** socket que o cliente reside */
    int sockfd;
};

struct ftp_client *
ftp_client_init(void)
{
    static const int ON = 1;
    static const struct timeval tv = {
        .tv_usec = 250000, // 250 ms
    };
    struct ftp_client *client = calloc(1, sizeof *client);

    // cria file descriptor socket
    if ((client->sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket() ");
        ftp_client_cleanup(client);
        return NULL;
    }
    // configura socket para reutilizar endereço e porta
    if (setsockopt(client->sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                   &ON, sizeof(ON)))
    {
        perror("setsockopt() ");
        ftp_client_cleanup(client);
        return NULL;
    }
    // configura timeout para recebimento de pacotes
    if (setsockopt(client->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
        perror("setsockopt() ");
        ftp_client_cleanup(client);
        return NULL;
    }
    // realiza conexão ao servidor
    struct sockaddr_in6 addr = ftp_server_get_addr();
    if (connect(client->sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect() ");
        ftp_client_cleanup(client);
        return NULL;
    }

    return client;
}

void
ftp_client_cleanup(struct ftp_client *client)
{
    if (client->sockfd) close(client->sockfd);
    free(client);
}

int
ftp_client_send(struct ftp_client *client, struct ftp_message *msg)
{
    return send(client->sockfd, msg, sizeof *msg, MSG_NOSIGNAL);
}

int
ftp_client_recv(struct ftp_client *client, struct ftp_message *msg)
{
    return recv(client->sockfd, msg, sizeof *msg, MSG_NOSIGNAL);
}
