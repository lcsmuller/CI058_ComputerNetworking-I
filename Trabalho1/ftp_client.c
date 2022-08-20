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
    /** endereço em que o servidor que o cliente está conectado reside */
    struct sockaddr_in6 server_addr;
};

struct ftp_client *
ftp_client_init(void)
{
    static const int ON = 1;
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
    // realiza conexão ao servidor
    client->server_addr = ftp_server_get_addr();
    if (connect(client->sockfd, (struct sockaddr *)&client->server_addr,
                sizeof(client->server_addr))
        < 0)
    {
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
    return sendto(client->sockfd, msg, sizeof *msg, MSG_NOSIGNAL,
                  (struct sockaddr *)&client->server_addr,
                  sizeof(client->server_addr));
}

int
ftp_client_recv(struct ftp_client *client, struct ftp_message *msg)
{
    socklen_t size = sizeof(client->server_addr);
    return recvfrom(client->sockfd, msg, sizeof *msg, MSG_NOSIGNAL,
                    (struct sockaddr *)&client->server_addr, &size);
}
