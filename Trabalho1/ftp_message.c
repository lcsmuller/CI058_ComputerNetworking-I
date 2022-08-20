#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ftp.h"

void
ftp_message_init(struct ftp_message *msg)
{
    memset(msg, 0, sizeof *msg);
    msg->payload[0] = FTP_HEADER_START;
}

unsigned
ftp_message_get_data_size(const struct ftp_message *msg)
{
    return msg->payload[1] >> 4;
}

static void
_ftp_message_set_data_size(struct ftp_message *msg, unsigned size)
{
    msg->payload[1] = (msg->payload[2] << 4) >> 4;
    msg->payload[1] |= size;
}

enum ftp_message_types
ftp_message_get_type(const struct ftp_message *msg)
{
    return (msg->payload[2] << 4) >> 4;
}

static void
_ftp_message_set_type(struct ftp_message *msg, enum ftp_message_types type)
{
    msg->payload[2] = (msg->payload[2] >> 4) << 4;
    msg->payload[2] |= type;
}

unsigned char *
ftp_message_get_data(const struct ftp_message *msg)
{
    return (unsigned char *)(msg->payload + 3);
}

/**
 * @brief Dump hexadecimal representation of data
 *
 * @param desc if non-NULL, printed as a description before hex dump
 * @param addr the address to start dumping from
 * @param len the number of bytes to dump
 * @param per_line number of bytes on each output line
 */
static void
_hexdump(const char desc[],
         const void *addr,
         const size_t len,
         int per_line,
         FILE *stream)
{
    if (per_line < 4 || per_line > 64) per_line = 16;

    unsigned char buf[per_line + 1];
    const unsigned char *pc = addr;
    size_t i;

    if (desc) printf("%s:\n", desc);

    if (len == 0) {
        fputs("_hexdump(): zero length\n", stderr);
        return;
    }

    // process every byte in the data
    for (i = 0; i < len; ++i) {
        if ((i % per_line) == 0) {
            // only print previous-line ASCII buffer for lines beyond first
            if (i != 0) fprintf(stream, "  %s\n", buf);
            // output the offset of current line
            fprintf(stream, "  %04zx ", i);
        }
        // now the hex code for the specific character
        fprintf(stream, " %02x", pc[i]);
        // and buffer a printable ASCII character for later
        buf[i % per_line] = isprint(pc[i]) ? '.' : '\0';
    }
    // pad out last line if not exactly per_line characters
    while ((i % per_line) != 0) {
        fprintf(stream, "   ");
        i++;
    }
    // and print the final ASCII buffer
    fprintf(stream, "  %s\n", buf);
}

size_t
ftp_message_print(const struct ftp_message *msg, FILE *out)
{
    const unsigned size = ftp_message_get_data_size(msg);
    char desc[1024] = "";

    snprintf(desc, sizeof(desc), "%.*s", size, ftp_message_get_data(msg));

    _hexdump(desc, msg, sizeof *msg, 16, out);
    return size;
}

void
ftp_message_update(struct ftp_message *msg,
                   enum ftp_message_types type,
                   const char data[],
                   unsigned size)
{
    if (size > (FTP_MESSAGE_DATA_BMAX / 8)) size = FTP_MESSAGE_DATA_BMAX / 8;
    _ftp_message_set_data_size(msg, size);
    _ftp_message_set_type(msg, type);
#if 0
    msg->sequence += 1;
#endif
    memcpy(ftp_message_get_data(msg), data, size);
}
