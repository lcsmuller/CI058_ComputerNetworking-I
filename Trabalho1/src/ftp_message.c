#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <unistd.h>
#include <netinet/in.h>

#include "ftp.h"

void
ftp_message_init(struct ftp_message *msg)
{
    memset(msg, 0, sizeof *msg);
    msg->header = FTP_MESSAGE_HEADER_VALUE;
    msg->seq = 0xE;
}

#ifndef FTP_DEBUG
#define FTP_DEBUG_PRINT(addr, len, stream, ...) fprintf(stream, __VA_ARGS__)
#else
#define FTP_DEBUG_PRINT(addr, len, stream, ...)                               \
    do {                                                                      \
        fprintf(stream, __VA_ARGS__);                                         \
        _hexdump(addr, len, stream);                                          \
    } while (0)

/**
 * @brief Dump hexadecimal representation of data
 *
 * @param addr the address to start dumping from
 * @param len the number of bytes to dump
 * @param stream stream to dump to
 */
static void
_hexdump(const void *addr, const size_t len, FILE *stream)
{
    unsigned char buf[16 + 1];
    const int per_line = sizeof(buf) - 1;
    const unsigned char *pc = addr;
    size_t i;

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
    switch (msg->type) {
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
    default:
        break;
    }
    return NULL;
}

void
ftp_message_print(const struct ftp_message *msg, FILE *out)
{
    FTP_DEBUG_PRINT(msg, sizeof *msg, out, "%u: %s%.*s\n", msg->seq,
                    _ftp_get_type_name(msg), msg->size, msg->data);
}

bool
ftp_message_update(struct ftp_message *msg,
                   enum ftp_message_types type,
                   const char data[FTP_MESSAGE_DATA_SIZE - 1],
                   size_t size)
{
    bool is_truncated;

    /* atualiza tamanho */
    if (size < FTP_MESSAGE_DATA_SIZE - 1)
        is_truncated = false;
    else {
        size = FTP_MESSAGE_DATA_SIZE - 1;
        is_truncated = true;
    }
    msg->size = size;
    /* atualiza tipo */
    msg->type = type;
    /* atualiza dados */
    memcpy(msg->data, data, size);

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
    unsigned size = msg->size;
    bool should_pipe = true;
    char cmd[1024] = { 0 };
    const char *name = "";

    switch (msg->type) {
    case FTP_TYPES_MKDIR:
        name = "mkdir ";
        break;
    case FTP_TYPES_LS:
        name = "ls ";
        break;
    case FTP_TYPES_CD:
        memcpy(cmd, msg->data, size);
        chdir(cmd);
        name = "pwd ";
        size = 0;
        break;
    case FTP_TYPES_GET:
    case FTP_TYPES_PUT:
        should_pipe = false;
        break;
    default:
        return NULL;
    }

    struct _ftp_file_wrapped *file = malloc(sizeof *file);
    if (!file) return NULL;

    snprintf(cmd, sizeof(cmd), "%s%.*s", name, size, msg->data);
    *file = (struct _ftp_file_wrapped){
        .stream = should_pipe ? popen(cmd, "r") : fopen(cmd, "r"),
        .is_pipe = should_pipe,
    };

    return (struct ftp_file *)file;
}

int
ftp_message_send(int sockfd, struct ftp_message *msg)
{
    int retval;
    /* updates sequence */
    msg->seq = (msg->seq + 1) % 0xF;
    if ((retval = send(sockfd, msg, sizeof *msg, 0)) >= 0)
        printf("SEND (%u bytes):\t", msg->size);
    else
        perror("send()");
    return retval;
}

int
ftp_message_recv(int sockfd, struct ftp_message *msg)
{
    const unsigned next_seq = (msg->seq + 1) % 0xF;
    struct ftp_message recv_msg = { 0 };
    int retval = recv(sockfd, &recv_msg, sizeof(recv_msg), 0);

    /* ignores garbage */
    if (recv_msg.header != FTP_MESSAGE_HEADER_VALUE) return 0;
    /* ignores message not belonging to the next in sequence */
    if (recv_msg.seq != next_seq) return 0;

    *msg = recv_msg;
    printf("RECV (%d bytes):\t", retval);

    if (retval < 0) perror("recv()");
    return retval;
}

void
ftp_file_close(struct ftp_file *file)
{
    if (((struct _ftp_file_wrapped *)file)->is_pipe)
        pclose(file->stream);
    else
        fclose(file->stream);
    free(file);
}
