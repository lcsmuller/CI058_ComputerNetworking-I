#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ftp.h"

int
main(void)
{
    struct ftp_client *client = ftp_client_init();
    char buf[1024];

    if (!client) exit(EXIT_FAILURE);

    while (puts("Insira comando:\t"), 1) { // loop do cliente
        struct ftp_message msg;

        ftp_message_init(&msg);

        memset(buf, 0, sizeof(buf));
        fgets(buf, sizeof(buf), stdin);
        ftp_message_update(&msg, FTP_TYPES_LS, buf, sizeof(buf));

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
