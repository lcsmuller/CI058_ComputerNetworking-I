#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include "ftp.h"

struct command_args {
    enum ftp_message_types type;
    bool is_remote;
    char *contents;
    unsigned len;
};

struct command_args
command_args_fetch(FILE *stream)
{
    struct command_args args = { 0 };
    char buf[1024];
    unsigned len;

    memset(buf, 0, sizeof(buf));
    fgets(buf, sizeof(buf), stream);

    len = strcspn(buf, " \n");
    args.contents = buf + len + 1;
    args.len = strcspn(args.contents, "\n");

    switch (len) {
    case 2:
        if (0 == strncmp(buf, "ls", len))
            args.type = FTP_TYPES_LS;
        else if (0 == strncmp(buf, "cd", len))
            args.type = FTP_TYPES_CD;
        break;
    case 3:
        if (0 == strncmp(buf, "rls", len)) {
            args.type = FTP_TYPES_LS;
            args.is_remote = true;
        }
        else if (0 == strncmp(buf, "rcd", len)) {
            args.type = FTP_TYPES_CD;
            args.is_remote = true;
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
            args.is_remote = true;
        }
        break;
    default:
        args.type = FTP_TYPES_ERROR;
        break;
    }

    return args;
}

int
main(void)
{
    int sockfd = ftp_raw_socket("lo");
    struct command_args args;
    struct ftp_message msg;
    int retval;

    if (sockfd < 0) return EXIT_FAILURE;

    ftp_message_init(&msg);
    puts("Starting client's session ...");
    while (1) {
        printf("Input:\t");
        args = command_args_fetch(stdin);
        ftp_message_update(&msg, args.type, args.contents, args.len);

        if (!args.is_remote) { /* perform command locally */
            struct ftp_file *file = ftp_message_unpack(&msg);
            if (file) { /* print command output to screen */
                char buf[sizeof(msg.data)];
                size_t len;

                while (1) {
                    len = fread(buf, sizeof(char), sizeof(buf), file->stream);
                    if (!len) break;
                    ftp_message_update(&msg, FTP_TYPES_DATA, buf, len);
                    ftp_message_print(&msg, stdout);
                }
                ftp_file_close(file);
            }
        }
        else { /* perform command remotely */
            if (ftp_message_send(sockfd, &msg) < 0) {
                perror("ftp_message_send()");
                continue;
            }
            printf("SEND (%u bytes):\t", msg.size);
            ftp_message_print(&msg, stdout);
            /* awaits server's response */
            while (1) {
                if ((retval = ftp_message_recv(sockfd, &msg)) < 0) {
                    perror("ftp_message_recv()");
                    break;
                }
                else if (retval > 0) {
                    printf("RECV (%d bytes):\t", retval);
                    ftp_message_print(&msg, stdout);
                    /** FIXME: temporary, shouldn't break before ACK */
                    break;
                }
            }
        }
    }
    puts("Finishing client's session ...");

    close(sockfd);

    return EXIT_SUCCESS;
}
