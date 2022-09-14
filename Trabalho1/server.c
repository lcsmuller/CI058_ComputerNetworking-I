#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include "ftp.h"

int
main(const int argc, const char *argv[])
{
    struct ftp_message msg;
    struct ftp_file *fout;
    int retval;
    int sockfd;

    if (argc <= 1) {
        fprintf(stderr, "Usage: ./%s < %s\n", argv[0], argv[1]);
        return EXIT_FAILURE;
    }

    sockfd = ftp_raw_socket(argv[1]);
    if (sockfd < 0) return EXIT_FAILURE;

    ftp_message_init(&msg);
    fputs("Starting server's session ...\n", stderr);
    while (1) {
        /* awaits client payload */
        if ((retval = ftp_message_recv(sockfd, &msg)) == 0)
            continue;
        else if (retval < 0)
            fputs("Parity error!\n", stderr);
        ftp_message_print(&msg, stdout);

        /* perform received message action, and send back the response */
        if ((fout = ftp_message_unpack(&msg))) {
            const enum ftp_message_types type = msg.type;

            ftp_message_update(&msg, FTP_TYPES_OK, NULL, 0);
            ftp_message_send(sockfd, &msg);
            ftp_message_print(&msg, stdout);
            if (type == FTP_TYPES_PUT) /* receive packets from client */
                ftp_message_recv_batch(sockfd, &msg);
            else /* send packets to client */
                ftp_message_send_batch(sockfd, &msg, fout);

            ftp_file_close(fout);
        }
    }
    fputs("Finishing server's session ...\n", stderr);

    close(sockfd);

    return EXIT_SUCCESS;
}
