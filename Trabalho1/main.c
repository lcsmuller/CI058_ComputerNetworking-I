#include <stdio.h>
#include <stdlib.h>

#include "ftp.h"

int
main(void)
{
    struct ftp_message msg;

    ftp_message_init(&msg);
    ftp_message_update(&msg, FTP_TYPES_LS, ".", sizeof("."));
#if 0
    ftp_message_print(&msg, stdout);
#endif
    ftp_command_dispatch(&msg);
    return EXIT_SUCCESS;
}