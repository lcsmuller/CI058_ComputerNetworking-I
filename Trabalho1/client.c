#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

#include "ftp.h"

struct command_args {
    enum ftp_message_types type;
    enum ftp_message_types expect;
    char *contents;
    unsigned len;
};

struct command_args
command_args_fetch(FILE *stream)
{
    struct command_args args = { 0 };
    char buf[1024] = { 0 };
    unsigned len;

    fgets(buf, sizeof(buf), stream);

    len = strcspn(buf, " \n");
    args.contents = buf + len + 1;
    args.len = strcspn(args.contents, "\n");

    switch (len) {
    case 2:
        if (0 == strncmp(buf, "ls", len)) {
            args.type = FTP_TYPES_LS;
        }
        else if (0 == strncmp(buf, "cd", len))
            args.type = FTP_TYPES_CD;
        break;
    case 3:
        if (0 == strncmp(buf, "rls", len)) {
            args.type = FTP_TYPES_LS;
            args.expect = FTP_TYPES_END;
        }
        else if (0 == strncmp(buf, "rcd", len)) {
            args.type = FTP_TYPES_CD;
            args.expect = FTP_TYPES_END;
        }
        else if (0 == strncmp(buf, "get", len)) {
            args.type = FTP_TYPES_GET;
            args.expect = FTP_TYPES_END;
        }
        else if (0 == strncmp(buf, "put", len)) {
            args.type = FTP_TYPES_PUT;
            args.expect = FTP_TYPES_OK;
        }
        break;
    case 5:
        if (0 == strncmp(buf, "mkdir", len)) {
            args.type = FTP_TYPES_MKDIR;
        }
        break;
    case 6:
        if (0 == strncmp(buf, "rmkdir", len)) {
            args.type = FTP_TYPES_MKDIR;
            args.expect = FTP_TYPES_END;
        }
        break;
    default:
        args.type = FTP_TYPES_ERROR;
        break;
    }

    return args;
}

int
main(const int argc, const char *argv[])
{
    unsigned char buf[FTP_MESSAGE_DATA_SIZE - 1];
    struct timeval timeout = { .tv_sec = 2 };
    struct ftp_file *fout = NULL;
    struct command_args args;
    struct ftp_message msg;
    int sockfd;
    int retval;

    if (argc <= 1) {
        fprintf(stderr, "Usage: ./%s < %s\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    sockfd = ftp_raw_socket(argv[1]);
    if (sockfd < 0) return EXIT_FAILURE;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))
        < 0) {
        perror("setsockopt()");
        close(sockfd);
        return EXIT_FAILURE;
    }

    ftp_message_init(&msg);
    fputs("Starting client's session ...\n", stderr);
    while (1) {
        fputs(">>> ", stderr);
        args = command_args_fetch(stdin);
        ftp_message_update(&msg, args.type, (unsigned char *)args.contents,
                           args.len);

        if (fout) {
            ftp_file_close(fout);
            fout = NULL;
        }
        fout = ftp_message_unpack(&msg);

        /* PERFORM COMMAND LOCALLY */
        if (!args.expect) {
            if (fout) {
                size_t len;
                while ((len = fread(buf, 1, sizeof(buf), fout->stream))) {
                    ftp_message_update(&msg, msg.type, buf, len);
                    ftp_message_print(&msg, stdout);
                }
            }
            continue;
        }

        /* PERFORM COMMAND REMOTELY */
        ftp_message_send(sockfd, &msg);
        ftp_message_print(&msg, stdout);

        while ((retval = ftp_message_recv(sockfd, &msg)) == 0)
            continue;
        if (retval == -2) {
            ftp_message_init(&msg);
            continue;
        }
        ftp_message_print(&msg, stdout);

        if (msg.type != FTP_TYPES_OK) {
            fputs("Error: Expect OK!\n", stderr);
            continue;
        }

        if (args.type != FTP_TYPES_PUT) /* receive packets from server */
            ftp_message_recv_batch(sockfd, &msg);
        else { /* send packets to server */
            if (!fout) {
                fputs("Error: Couldn't unpack message (Put)\n", stderr);
                continue;
            }
            ftp_message_send_batch(sockfd, &msg, fout);
        }
    }
    fputs("Finishing client's session ...\n", stderr);

    close(sockfd);

    return EXIT_SUCCESS;
}
