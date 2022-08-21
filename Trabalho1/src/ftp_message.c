#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>

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
    return msg->payload[1] >> (8 - FTP_MESSAGE_SIZE_BMAX);
}

static void
_ftp_message_set_data_size(struct ftp_message *msg,
                           unsigned size,
                           bool *is_truncated)
{
    if (size <= (FTP_MESSAGE_DATA_BMAX / 8) - 1)
        *is_truncated = false;
    else {
        size = (FTP_MESSAGE_DATA_BMAX / 8) - 1;
        *is_truncated = true;
    }
    msg->payload[1] =
        (msg->payload[1] << FTP_MESSAGE_SIZE_BMAX) >> FTP_MESSAGE_SIZE_BMAX;
    msg->payload[1] |= (unsigned char)(size << (8 - FTP_MESSAGE_SIZE_BMAX));
}

enum ftp_message_types
ftp_message_get_type(const struct ftp_message *msg)
{
    return (msg->payload[2] << (8 - FTP_MESSAGE_TYPE_BMAX))
           >> (8 - FTP_MESSAGE_TYPE_BMAX);
}

static void
_ftp_message_set_type(struct ftp_message *msg, enum ftp_message_types type)
{
    msg->payload[2] = (msg->payload[2] >> FTP_MESSAGE_TYPE_BMAX)
                      << FTP_MESSAGE_TYPE_BMAX;
    msg->payload[2] |= (unsigned char)type;
}

unsigned char *
ftp_message_get_data(const struct ftp_message *msg)
{
    return (unsigned char *)(msg->payload + 3);
}

static void
_ftp_message_set_data(struct ftp_message *msg, const char data[])
{
    memcpy(msg->payload + 3, data, FTP_MESSAGE_DATA_BMAX / 8);
}

#ifdef FTP_DEBUG
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

    if (desc) printf("%s", desc);

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
#endif

static const char *
_ftp_get_type_name(const struct ftp_message *msg)
{
    switch (ftp_message_get_type(msg)) {
    case FTP_TYPES_ACK:
        return "ACK\n";
    case FTP_TYPES_CD:
        return "cd ";
    case FTP_TYPES_DATA:
        return "DATA\n";
    case FTP_TYPES_END:
        return "END\n";
    case FTP_TYPES_ERROR:
        return "ERROR\n";
    case FTP_TYPES_FDESC:
        return "FDESC\n";
    case FTP_TYPES_GET:
        return "GET\n";
    case FTP_TYPES_LS:
        return "ls ";
    case FTP_TYPES_MKDIR:
        return "mkdir ";
    case FTP_TYPES_NACK:
        return "NACK\n";
    case FTP_TYPES_OK:
        return "OK\n";
    case FTP_TYPES_PUT:
        return "PUT\n";
    }
    return "UNKNOWN\n";
}

int
ftp_message_print(const struct ftp_message *msg, FILE *out)
{
    char desc[1024] = "";
    int ret =
        snprintf(desc, sizeof(desc), "%s%.*s\n", _ftp_get_type_name(msg),
                 ftp_message_get_data_size(msg), ftp_message_get_data(msg));
#ifdef FTP_DEBUG
    _hexdump(desc, msg, sizeof *msg, 16, out);
#else
    fputs(desc, out);
#endif
    return ret;
}

bool
ftp_message_update(struct ftp_message *msg,
                   enum ftp_message_types type,
                   const char data[],
                   unsigned size)
{
    bool is_truncated;

    _ftp_message_set_data_size(msg, size, &is_truncated);
    _ftp_message_set_type(msg, type);
    _ftp_message_set_data(msg, data);
#if 0
    msg->sequence += 1;
#endif
    return is_truncated;
}

/** @brief Aligns to @ref ftp_file and maintains a private is_pipe field */
struct _ftp_file_wrapped {
    /** pointer to stream */
    FILE *stream;
    /** whether stream is a pipe */
    bool is_pipe;
};

struct ftp_file *
ftp_message_unpack(struct ftp_message *msg)
{
    const enum ftp_message_types type = ftp_message_get_type(msg);
    unsigned size = ftp_message_get_data_size(msg);
    const char *name = NULL;
    char cmd[1024] = { 0 };
    bool should_pipe = true;

    switch (type) {
    case FTP_TYPES_MKDIR:
        name = "mkdir";
        break;
    case FTP_TYPES_LS:
        name = "ls";
        break;
    case FTP_TYPES_CD: /**< @todo print getcwd() output */
        memcpy(cmd, ftp_message_get_data(msg), size);
        chdir(cmd);
        name = "pwd";
        size = 0;
        break;
    default:
        return NULL;
    }

    struct _ftp_file_wrapped *file = malloc(sizeof *file);
    if (!file) return NULL;

    snprintf(cmd, sizeof(cmd), "%s %.*s", name, size,
             ftp_message_get_data(msg));
    *file = (struct _ftp_file_wrapped){
        .stream = should_pipe ? popen(cmd, "r") : fopen(cmd, "r"),
        .is_pipe = should_pipe,
    };

    return (struct ftp_file *)file;
}

void
ftp_file_close(struct ftp_file *file)
{
    if (((struct _ftp_file_wrapped *)file)->is_pipe)
        pclose(file->stream);
    else
        fclose(file->stream);
}
