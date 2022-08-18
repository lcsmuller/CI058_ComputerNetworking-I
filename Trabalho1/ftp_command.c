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
    char cmd[FTP_DATA_MAX + 1] = { 0 };
    const char *aux = "";
    FILE *fp;

    switch (msg->type) {
    case FTP_TYPES_CD: /**< @todo print getcwd() output */
        memcpy(cmd, msg->data, msg->size);
        chdir(cmd);
        return;
    case FTP_TYPES_MKDIR:
        aux = "mkdir";
        break;
    case FTP_TYPES_LS:
        aux = "ls";
        break;
    case FTP_TYPES_DATA:
        aux = "hexdump";
        break;
    default:
        break;
    }

    /** @todo transformar em um função que envia em 'chunks' de
     *      @ref FTP_DATA_MAX bytes ao cliente */
    snprintf(cmd, sizeof(cmd), "%s %.*s", aux, msg->size, msg->data);
    fp = popen(cmd, "r");
    for (int c; (c = fgetc(fp)) != EOF;)
        putchar(c);
    pclose(fp);
}