#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ftp.h"

#define BUFFER_LEN 1024

struct command_args {
    enum ftp_message_types type;
    char *contents;
    unsigned len;
};

struct command_args
command_args_fetch(FILE *stream)
{
    char buf[BUFFER_LEN];
    struct command_args args;
    unsigned len;

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), stream);

    len = strcspn(buf, " ");
    args.contents = buf + len + 1;
    args.len = strcspn(args.contents, "\n");

    switch (len) {
    case 2:
        if (0 == strncmp(buf, "ls", len))
            args.type = FTP_TYPES_LS;
        else if (0 == strncmp(buf, "cd", len))
            args.type = FTP_TYPES_CD;
        break;
    case 5:
        if (0 == strncmp(buf, "mkdir", len)) args.type = FTP_TYPES_MKDIR;
        break;
    default:
        args.type = FTP_TYPES_ERROR;
        break;
    }

    return args;
}

int
main(void)
{
    struct ftp_client *client = ftp_client_init();
    struct ftp_message msg;

    if (!client) exit(EXIT_FAILURE);

    while (printf("Insira comando:\t"), 1) { // loop do cliente
        struct command_args args = command_args_fetch(stdin);

        ftp_message_init(&msg);
        ftp_message_update(&msg, args.type, args.contents, args.len);

        printf("SEND:\t");
        ftp_message_print(&msg, stdout);

        // envia mensagem ao servidor
        if (ftp_client_send(client, &msg) < 0) {
            perror("Não foi possível enviar pacote");
            break;
        }

        while (ftp_client_recv(client, &msg) > 0) {
            printf("RECV:\t");
            ftp_message_print(&msg, stdout);
        }
    }

    ftp_client_cleanup(client);

    return EXIT_FAILURE;
}
