#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "ftp.h"

#define SERVER_MAX_FDS 16

struct ftp_server {
    struct sockaddr_in6 *addr;
    int sockfd;
    int timeout_ms;
    struct pollfd *fds;
    int nfds;
};

struct sockaddr_in6
ftp_server_get_addr(void)
{
    return (struct sockaddr_in6){
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(FTP_SERVER_PORT),
    };
}

struct ftp_server *
ftp_server_init(void)
{
    static const int ON = 1;
    struct ftp_server *server = calloc(1, sizeof *server);

    /* stream socket para receber conexões */
    if ((server->sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket() ");
        ftp_server_cleanup(server);
        return NULL;
    }
    /* permite a reutilização do socket */
    if (setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &ON, sizeof(ON))
        < 0) {
        perror("setsockopt() ");
        ftp_server_cleanup(server);
        return NULL;
    }
    /* faz com que o socket seja não bloqueante */
    if (ioctl(server->sockfd, FIONBIO, &ON) < 0) {
        perror("ioctl() ");
        ftp_server_cleanup(server);
        return NULL;
    }
    /* bind o socket a um endereço */
    server->addr = calloc(1, sizeof *server->addr);
    if (!server->addr) {
        perror("calloc() ");
        ftp_server_cleanup(server);
        return NULL;
    }
    *server->addr = ftp_server_get_addr();
    if (bind(server->sockfd, (struct sockaddr *)server->addr,
             sizeof *server->addr)
        < 0)
    {
        perror("bind() ");
        ftp_server_cleanup(server);
        return NULL;
    }
    /* seta o backlog do listen (qtd máxima de conexões ao server) */
    if (listen(server->sockfd, SERVER_MAX_FDS - 1) < 0) {
        perror("listen() ");
        ftp_server_cleanup(server);
        return NULL;
    }
    /** inicializa a estrutura pollfd */
    server->fds = calloc(SERVER_MAX_FDS, sizeof(struct pollfd));
    if (!server->fds) {
        perror("calloc() ");
        ftp_server_cleanup(server);
        return NULL;
    }
    server->fds[0] = (struct pollfd){
        .fd = server->sockfd,
        .events = POLLIN,
    };
    server->nfds = 1;
    /* timeout de 3 minutos */
    server->timeout_ms = 3 * 60 * 1000;

    return server;
}

void
ftp_server_cleanup(struct ftp_server *server)
{
    if (server->fds) {
        for (int i = 0; i < server->nfds; ++i)
            if (server->fds[i].fd >= 0) close(server->fds[i].fd);
        free(server->fds);
    }
    if (server->sockfd) close(server->sockfd);
    if (server->addr) free(server->addr);
    free(server);
}
