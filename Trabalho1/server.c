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
    int sockfd = ftp_raw_socket("lo");
    struct ftp_message msg;
    struct ftp_file *file;
    int retval;

    if (sockfd < 0) return EXIT_FAILURE;

    ftp_message_init(&msg);
    puts("Starting server's session ...");
    while (1) {
        /* awaits client payload */
        if ((retval = ftp_message_recv(sockfd, &msg)) < 0) {
            perror("ftp_message_recv()");
            continue;
        }
        else if (retval > 0) {
            printf("RECV (%d bytes):\t", retval);
            ftp_message_print(&msg, stdout);

            if ((file = ftp_message_unpack(&msg))) {
                char buf[sizeof(msg.data)];
                bool end = false;
                size_t len;

                while (1) {
                    len = fread(buf, sizeof(char), sizeof(buf), file->stream);
                    if (!len) break;

                    ftp_message_update(&msg, FTP_TYPES_DATA, buf, len);
                    if (ftp_message_send(sockfd, &msg) < 0) {
                        perror("ftp_message_send()");
                        end = true;
                        break;
                    }
                    printf("SEND (%u bytes):\t", msg.size);
                    ftp_message_print(&msg, stdout);
                }
                ftp_file_close(file);
                if (end) break;
            }
        }
    }
    puts("Finishing server's session ...");

    close(sockfd);

    return EXIT_SUCCESS;
}
