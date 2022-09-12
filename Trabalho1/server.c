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
main(void)
{
    char buf[FTP_MESSAGE_DATA_SIZE - 1];
    int sockfd = ftp_raw_socket("lo");
    struct ftp_message msg;
    struct ftp_file *fout;

    if (sockfd < 0) return EXIT_FAILURE;

    ftp_message_init(&msg);
    puts("Starting server's session ...");
    while (1) {
        /* awaits client payload */
        if (ftp_message_recv(sockfd, &msg) <= 0) {
            continue;
        }

        /* interpret client payload */
        ftp_message_print(&msg, stdout);

        /* perform received message action, and send back the response */
        if ((fout = ftp_message_unpack(&msg))) {
            switch (msg.type) {
            case FTP_TYPES_CD:
            case FTP_TYPES_LS:
            case FTP_TYPES_MKDIR:
                ftp_message_update(&msg, FTP_TYPES_OK, NULL, 0);
                ftp_message_send(sockfd, &msg);
                break;
            default:
                break;
            }

            bool end = false;
            for (size_t len; (len = fread(buf, 1, sizeof(buf), fout->stream));)
            {

                ftp_message_update(&msg, FTP_TYPES_DATA, buf, len);
                if (ftp_message_send(sockfd, &msg) < 0) {
                    end = true;
                    break;
                }
                ftp_message_print(&msg, stdout);
            }
            ftp_message_update(&msg, FTP_TYPES_END, NULL, 0);
            if (ftp_message_send(sockfd, &msg) < 0) {
                end = true;
            }
            ftp_file_close(fout);
            if (end) break;
        }
    }
    puts("Finishing server's session ...");

    close(sockfd);

    return EXIT_SUCCESS;
}
