#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

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
main(void)
{
    char buf[FTP_MESSAGE_DATA_SIZE - 1];
    int sockfd = ftp_raw_socket("lo");
    struct command_args args;
    struct ftp_message msg;
    struct ftp_file *fout;

    if (sockfd < 0) return EXIT_FAILURE;

    ftp_message_init(&msg);
    puts("Starting client's session ...");
    while (1) {
        printf(">> ");
        args = command_args_fetch(stdin);

        ftp_message_update(&msg, args.type, args.contents, args.len);
        if (!args.expect) { /* PERFORM COMMAND LOCALLY */
            if ((fout = ftp_message_unpack(&msg))) {
                for (size_t len;
                     (len = fread(buf, 1, sizeof(buf), fout->stream));) {

                    ftp_message_update(&msg, msg.type, buf, len);
                    ftp_message_print(&msg, stdout);
                }
                ftp_file_close(fout);
            }
            continue;
        }

        /* PERFORM COMMAND REMOTELY */

        if (args.type == FTP_TYPES_PUT) { /* send packets to server */
            if (!(fout = ftp_message_unpack(&msg))) continue;

            while (1) {
                for (size_t len;
                     (len = fread(buf, 1, sizeof(buf), fout->stream));) {

                    ftp_message_update(&msg, FTP_TYPES_DATA, buf, len);
                    if (ftp_message_send(sockfd, &msg) < 0) {
                        break;
                    }
                    ftp_message_print(&msg, stdout);
                }
                ftp_message_update(&msg, FTP_TYPES_END, NULL, 0);
                ftp_message_send(sockfd, &msg);
            }
            ftp_file_close(fout);
        }
        else { /* receive packets from server */
            if (ftp_message_send(sockfd, &msg) < 0) {
                continue;
            }
            ftp_message_print(&msg, stdout);

            /* awaits server's response */
            for (int retval; msg.type != args.expect;) {
                if ((retval = ftp_message_recv(sockfd, &msg)) < 0) {
                    break;
                }
                if (retval > 0) ftp_message_print(&msg, stdout);
            }
        }
    }
    puts("Finishing client's session ...");

    close(sockfd);

    return EXIT_SUCCESS;
}
