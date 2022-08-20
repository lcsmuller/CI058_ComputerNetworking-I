#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ftp.h"

#define BUFFER_LEN 1024

struct command {
    char *args;
    unsigned argslen;
    enum ftp_message_types type;
};

void
command_init(struct command *cmd, char buf[BUFFER_LEN])
{
    const unsigned cmdlen = strcspn(buf, " ");

    cmd->args = buf + cmdlen + 1;
    cmd->argslen = strcspn(cmd->args, "\n");

    switch (cmdlen) {
    case 2:
        if (0 == strncmp(buf, "ls", cmdlen))
            cmd->type = FTP_TYPES_LS;
        else if (0 == strncmp(buf, "cd", cmdlen))
            cmd->type = FTP_TYPES_CD;
        break;
    case 5:
        if (0 == strncmp(buf, "mkdir", cmdlen)) cmd->type = FTP_TYPES_MKDIR;
        break;
    default:
        cmd->type = FTP_TYPES_ERROR;
        break;
    }
}

int
main(void)
{
    struct ftp_client *client = ftp_client_init();
    struct ftp_message msg;
    char buf[BUFFER_LEN];
    struct command cmd;

    if (!client) exit(EXIT_FAILURE);

    while (puts("Insira comando:\t"), 1) { // loop do cliente
        ftp_message_init(&msg);

        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), stdin);
        command_init(&cmd, buf);

        ftp_message_update(&msg, cmd.type, cmd.args, cmd.argslen);

        puts("Sending to server:");
        ftp_message_print(&msg, stdout);
        putchar('\n');

        // envia mensagem ao servidor
        if (ftp_client_send(client, &msg) < 0) {
            perror("Não foi possível enviar pacote");
            break;
        }
    }

    ftp_client_cleanup(client);

    return EXIT_FAILURE;
}
