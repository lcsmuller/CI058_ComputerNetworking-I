#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ftp.h"

struct ftp_message
ftp_message_init(void)
{
    return (struct ftp_message){ FTP_HEADER_START, 0, 0, 0, { 0 } };
}

size_t
ftp_message_print(const struct ftp_message *msg, FILE *out)
{
    const unsigned size = (sizeof *msg - FTP_DATA_MAX) + msg->size;
    return fwrite(msg, sizeof(char), size, out);
}

void
ftp_message_update(struct ftp_message *msg,
                   enum ftp_message_types type,
                   const char data[],
                   size_t size)
{
    msg->type = type;
    msg->size = (size > FTP_DATA_MAX) ? FTP_DATA_MAX : size;
    msg->sequence += 1;
    memcpy(msg->data, data, msg->size);
}
