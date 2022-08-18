#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "ftp.h"

#define SERVER_PORT    5010
#define SERVER_MAX_FDS 16

static void
_ftp_server_fds_compress(struct ftp_server *server)
{
    for (int i = 0; i < server->nfds; ++i) {
        if (server->fds[i].fd != -1) continue;

        memmove(server->fds + i, server->fds + (i + 1), server->nfds - i);
        --i;
        --server->nfds;
    }
}

static void
_ftp_fd_close(struct pollfd *pollfd)
{
    close(pollfd->fd);
    pollfd->fd = -1;
}

int
ftp_server_init(struct ftp_server *server)
{
    /* stream socket para receber conexões */
    if ((server->sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket() ");
        return -1;
    }
    /* permite a reutilização do socket */
    static const int ON = 1;
    if (setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &ON, sizeof(ON))
        < 0) {
        perror("setsockopt() ");
        close(server->sockfd);
        return -1;
    }
    /* faz com que o socket seja não bloqueante */
    if (ioctl(server->sockfd, FIONBIO, &ON) < 0) {
        perror("ioctl() ");
        close(server->sockfd);
        return -1;
    }
    /* bind o socket a um endereço */
    server->addr = calloc(1, sizeof *server->addr);
    *server->addr = (struct sockaddr_in6){
        .sin6_family = AF_INET6,
        .sin6_addr = in6addr_any,
        .sin6_port = htons(SERVER_PORT),
    };
    if (bind(server->sockfd, (struct sockaddr *)server->addr,
             sizeof(*server->addr))
        < 0)
    {
        perror("bind() ");
        close(server->sockfd);
        return -1;
    }
    /* seta o backlog do listen (qtd máxima de conexões ao server) */
    if (listen(server->sockfd, SERVER_MAX_FDS - 1) < 0) {
        perror("listen() ");
        close(server->sockfd);
        return -1;
    }
    /** inicializa a estrutura pollfd XXX: talvez seja possível diminuir */
    server->fds = calloc(SERVER_MAX_FDS, sizeof(struct pollfd));
    server->fds[0] = (struct pollfd){
        .fd = server->sockfd,
        .events = POLLIN,
    };
    server->nfds = 1;
    /* timeout de 3 minutos */
    server->timeout_ms = 3 * 60 * 1000;

    return 0;
}

void
ftp_server_cleanup(struct ftp_server *server)
{
    for (int i = 0; i < server->nfds; ++i)
        if (server->fds[i].fd >= 0) close(server->fds[i].fd);
    free(server->fds);
    free(server->addr);
}

int
main(void)
{
    struct ftp_server server;

    /* inicia servidor que ouve a porta `SERVER_PORT` */
    if (ftp_server_init(&server) < 0) exit(EXIT_FAILURE);

    /* loop de eventos do servidor */
    for (bool end = false; !end;) {
        bool should_compress_array = false;
        int retval;

        /* poll() eventos */
        puts("Aguardando em poll()...");
        if ((retval = poll(server.fds, server.nfds, server.timeout_ms)) < 0) {
            perror("poll() ");
            break;
        }
        else if (retval == 0) { /* timeout atingido sem receber eventos */
            fputs("Tempo de espera de poll() se esgotou\n", stderr);
            break;
        }

        /* evento(s) recebido(s), decodificar */
        for (int i = 0, n = server.nfds; i < n; ++i) {
            if (server.fds[i].revents == 0) continue;

            if (server.fds[i].revents != POLLIN) {
                fprintf(stderr, "Error revents = %hu\n",
                        server.fds[i].revents);
                end = true;
                break;
            }

            if (server.fds[i].fd == server.sockfd)
            { /* nova conexão de cliente */
                for (int new_sockfd = 0; new_sockfd != -1;) {
                    if ((new_sockfd = accept(server.sockfd, NULL, NULL)) < 0) {
                        if (errno != EWOULDBLOCK) {
                            perror("accept() ");
                            end = true;
                        }
                        break;
                    }
                    /* adiciona nova conexão à estrutura pollfd */
                    server.fds[server.nfds].fd = new_sockfd;
                    server.fds[server.nfds].events = POLLIN;
                    ++server.nfds;
                }
            }
            else { /* mensagem recebida de algum cliente existente */
                /** XXX: loop permite receber mensagem em 'chunks' */
                while (1) {
                    /* recebe payload do cliente */
                    if ((retval = recv(server.fds[i].fd, server.buf,
                                       sizeof(server.buf), 0))
                        < 0) {
                        if (errno != EWOULDBLOCK) {
                            perror("accept() ");
                            _ftp_fd_close(&server.fds[i]);
                            should_compress_array = true;
                        }
                        break;
                    }
                    else if (retval == 0) {
                        fputs("Conexão encerrada pelo cliente\n", stderr);
                        _ftp_fd_close(&server.fds[i]);
                        should_compress_array = true;
                        break;
                    }

                    /** TODO: decodifica payload do usuário e executa comando
                     */

                    /** echo de volta ao cliente TODO: manda resposta do
                     *      comando decodificado */
                    if (send(server.fds[i].fd, server.buf, retval, 0) < 0) {
                        perror("send() ");
                        _ftp_fd_close(&server.fds[i]);
                        should_compress_array = true;
                        break;
                    }
                }
            }
        }

        /* em casa de alguma conexão finalizada o cliente inativo é removido */
        if (should_compress_array) _ftp_server_fds_compress(&server);
    }

    ftp_server_cleanup(&server);

    return EXIT_FAILURE;
}
