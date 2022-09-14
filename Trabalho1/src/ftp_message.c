#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>

#include "ftp.h"

/* starts off with some impossible value */
static unsigned NACK_prev_seq = 1 << 5;

void
ftp_message_init(struct ftp_message *msg)
{
    memset(msg, 0, sizeof *msg);
    msg->header = FTP_MESSAGE_HEADER_VALUE;
    msg->seq = 0xE;
    NACK_prev_seq = 1 << 5;
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

static unsigned char
_ftp_get_parity(struct ftp_message *msg)
{
    unsigned char parity = 0;
    for (size_t i = 0; i < msg->size; ++i)
        parity ^= msg->data[i];
    return parity;
}

static void
_ftp_set_parity(struct ftp_message *msg)
{
    msg->data[msg->size] = _ftp_get_parity(msg);
}

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
    case FTP_TYPES_DATA_MASKED:
        return "DATA MASKED\n";
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
    fprintf(out, "%.*s", msg->size, msg->data);
    fflush(out);
    FTP_DEBUG_PRINT(msg, sizeof *msg, stderr, "%u: %s\n", msg->seq,
                    _ftp_get_type_name(msg));
}

bool
ftp_message_update(struct ftp_message *msg,
                   enum ftp_message_types type,
                   const unsigned char data[FTP_MESSAGE_DATA_SIZE - 1],
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
    memcpy(msg->data, data, size);
    /* atualiza tipo */
    if (type != FTP_TYPES_DATA)
        msg->type = type;
    else {
        bool should_mask = false;
        for (size_t i = 0; i < size; ++i) {
            if (data[i] == FTP_ERROR_SEQ_1 || data[i] == FTP_ERROR_SEQ_2
                || data[i] == FTP_ERROR_SEQ_3)
            {
                should_mask = true;
                break;
            }
        }
        msg->type = should_mask ? FTP_TYPES_DATA_MASKED : FTP_TYPES_DATA;
    }
    /* set parity byte */
    _ftp_set_parity(msg);

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

    if (msg->type == FTP_TYPES_DATA_MASKED) {
        for (size_t i = 0; i < msg->size; ++i)
            msg->data[i] ^= FTP_ERROR_SEQ_FILLER;
    }
    else if (msg->type == FTP_TYPES_NACK)
        NACK_prev_seq = msg->seq;

    if ((retval = send(sockfd, msg, sizeof *msg, 0)) >= 0)
        fprintf(stderr, "SEND (%zu bytes):\t", sizeof *msg);
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

    if (errno == EWOULDBLOCK) return -2;

    /* ignores garbage */
    if (recv_msg.header != FTP_MESSAGE_HEADER_VALUE) return 0;

    /* ignores message not belonging to the next in sequence (unless NACK) */
    if (recv_msg.type == FTP_TYPES_NACK) {
        if (recv_msg.seq == NACK_prev_seq) return 0;
        NACK_prev_seq = recv_msg.seq;
    }
    else if (recv_msg.seq != next_seq)
        return 0;
#if 0
    /** XXX: trigger NACK, remove later */
    if (_ftp_get_parity(&recv_msg) != recv_msg.data[recv_msg.size]) return -1;
#endif
    /* remove mask from masked messages */
    if (recv_msg.type == FTP_TYPES_DATA_MASKED) {
        for (size_t i = 0; i < recv_msg.size; ++i)
            recv_msg.data[i] ^= FTP_ERROR_SEQ_FILLER;
    }
    /* errors message with wrong parity */
    if (_ftp_get_parity(&recv_msg) != recv_msg.data[recv_msg.size]) return -1;

    *msg = recv_msg;
    fprintf(stderr, "RECV (%d bytes):\t", retval);

    if (retval < 0) perror("recv()");
    return retval;
}

void
ftp_message_send_batch(int sockfd,
                       struct ftp_message *msg,
                       struct ftp_file *fout)
{
    unsigned char buf[FTP_MESSAGE_DATA_SIZE - 1];
    struct ftp_message last_msg = { 0 };
    size_t len = 0;
    int retval;

    while (1) {
        if (msg->type == FTP_TYPES_NACK)
            memcpy(buf, last_msg.data, len = last_msg.size);
        else if (!(len = fread(buf, 1, sizeof(buf), fout->stream)))
            break;

        ftp_message_update(msg, FTP_TYPES_DATA, buf, len);
        ftp_message_send(sockfd, msg);
        ftp_message_print(msg, stdout);
        last_msg = *msg;

        /* consume sent data */
        while ((retval = ftp_message_recv(sockfd, msg)) == 0)
            continue;
        ftp_message_print(msg, stdout);
    }

    ftp_message_update(msg, FTP_TYPES_END, NULL, 0);
    ftp_message_send(sockfd, msg);
    ftp_message_print(msg, stdout);
}

void
ftp_message_recv_batch(int sockfd, struct ftp_message *msg)
{
    int retval;

    while (1) {
        while (1) {
            if ((retval = ftp_message_recv(sockfd, msg)) == 0) continue;

            if (retval < 0) {
                ftp_message_update(msg, FTP_TYPES_NACK, NULL, 0);
                ftp_message_send(sockfd, msg);
                ftp_message_print(msg, stdout);
                return;
            }
            ftp_message_print(msg, stdout);

            if ((msg->type == FTP_TYPES_DATA
                 || msg->type == FTP_TYPES_DATA_MASKED)) {
                break;
            }
            if (msg->type == FTP_TYPES_END) return;
        }

        ftp_message_update(msg, FTP_TYPES_ACK, NULL, 0);
        ftp_message_send(sockfd, msg);
        ftp_message_print(msg, stdout);
    }
}

void
ftp_file_close(struct ftp_file *file)
{
    if (((struct _ftp_file_wrapped *)file)->is_pipe && file->stream)
        pclose(file->stream);
    else
        fclose(file->stream);
    free(file);
}
