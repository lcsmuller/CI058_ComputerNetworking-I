#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/poll.h>

#include "ftp.h"

struct ftp_server {
    struct sockaddr_in6 *addr;
    int sockfd;
    int timeout_ms;
    struct pollfd *fds;
    int nfds;
};

static void
_ftp_fd_close(struct pollfd *pollfd)
{
    close(pollfd->fd);
    pollfd->fd = -1;
}

int
main(void)
{
    struct ftp_server *server = ftp_server_init();

    /* inicia servidor que ouve na porta `FTP_SERVER_PORT` */
    if (!server) exit(EXIT_FAILURE);
    /* loop de eventos do servidor */
    for (bool end = false; !end;) {
        bool should_compress_array = false;
        int retval;

        /* poll() eventos */
        puts("Aguardando em poll()...");
        if ((retval = poll(server->fds, server->nfds, server->timeout_ms)) < 0)
        {
            perror("poll() ");
            break;
        }
        else if (retval == 0) { /* timeout atingido sem receber eventos */
            fputs("Tempo de espera de poll() se esgotou\n", stderr);
            break;
        }

        /* evento(s) recebido(s), decodificar */
        for (int i = 0, n = server->nfds; i < n; ++i) {
            if (server->fds[i].revents == 0) continue;

            if (server->fds[i].revents != POLLIN) {
                fprintf(stderr, "Error revents = %hu\n",
                        server->fds[i].revents);
                end = true;
                break;
            }

            if (server->fds[i].fd == server->sockfd) { /* novo cliente */
                for (int new_sockfd = 0; new_sockfd != -1;) {
                    if ((new_sockfd = accept(server->sockfd, NULL, NULL)) < 0)
                    {
                        if (errno != EWOULDBLOCK) {
                            perror("accept() ");
                            end = true;
                        }
                        break;
                    }
                    /* adiciona nova conexão à estrutura pollfd */
                    server->fds[server->nfds].fd = new_sockfd;
                    server->fds[server->nfds].events = POLLIN;
                    ++server->nfds;
                }
            }
            else { /* mensagem recebida de algum cliente existente */
                /** XXX: loop permite receber mensagem em 'chunks' */
                while (1) {
                    struct ftp_message msg;

                    ftp_message_init(&msg);

                    /* recebe payload do cliente */
                    if ((retval =
                             recv(server->fds[i].fd, &msg, sizeof(msg), 0))
                        < 0) {
                        if (errno != EWOULDBLOCK) {
                            perror("accept() ");
                            _ftp_fd_close(&server->fds[i]);
                            should_compress_array = true;
                        }
                        break;
                    }
                    else if (retval == 0) {
                        fputs("Conexão encerrada pelo cliente\n", stderr);
                        _ftp_fd_close(&server->fds[i]);
                        should_compress_array = true;
                        break;
                    }

                    /** TODO: decodificar mensagem do usuário e exec comando */
                    puts("Received from client:");
                    ftp_message_print(&msg, stdout);
                    putchar('\n');

                    ftp_command_dispatch(&msg);
#if 0
                    /** echo de volta ao cliente TODO: manda resposta do
                     *      comando decodificado */
                    if (send(server->fds[i].fd, buf, retval, 0) < 0) {
                        perror("send() ");
                        _ftp_fd_close(&server->fds[i]);
                        should_compress_array = true;
                        break;
                    }
#endif
                }
            }
        }

        /* em caso de alguma conexão finalizada o cliente inativo é removido */
        if (should_compress_array)
            for (int i = 0; i < server->nfds; ++i) {
                if (server->fds[i].fd != -1) continue;

                memmove(server->fds + i, server->fds + (i + 1),
                        server->nfds - i);
                --i;
                --server->nfds;
            }
    }

    ftp_server_cleanup(server);

    return EXIT_FAILURE;
}
