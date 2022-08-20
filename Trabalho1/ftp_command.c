#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "ftp.h"

/** @todo comandos devem ser executados em outro processo
 *      (para evitar espera infinita) */
void
ftp_command_dispatch(struct ftp_message *msg)
{
    const enum ftp_message_types type = ftp_message_get_type(msg);
    const unsigned size = ftp_message_get_data_size(msg);
    char cmd[1024] = { 0 };
    const char *aux = "";
    FILE *fp;

    switch (type) {
    case FTP_TYPES_CD: /**< @todo print getcwd() output */
        memcpy(cmd, ftp_message_get_data(msg), size);
        chdir(cmd);
        return;
    case FTP_TYPES_MKDIR:
        aux = "mkdir";
        break;
    case FTP_TYPES_LS:
        aux = "ls";
        break;
    default:
        break;
    }

    snprintf(cmd, sizeof(cmd), "%s %.*s", aux, size,
             ftp_message_get_data(msg));
    fp = popen(cmd, "r");
    for (int c; (c = fgetc(fp)) != EOF;)
        putchar(c);
    pclose(fp);
}
