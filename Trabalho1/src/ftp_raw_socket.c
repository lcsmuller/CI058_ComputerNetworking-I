#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if.h>

int
ftp_raw_socket(const char device[])
{
    const uint16_t protocol = htons(ETH_P_ALL);
    struct sockaddr_ll addr;
    struct packet_mreq mr;
    struct ifreq ir;
    int sockfd;

    /* cria socket */
    if ((sockfd = socket(AF_PACKET, SOCK_RAW, protocol)) < 0) {
        perror("socket()");
        return -1;
    }
    /* dispositivo eth0 */
    memset(&ir, 0, sizeof(ir));
    snprintf(ir.ifr_name, sizeof(ir.ifr_name), "%s", device);
    if (ioctl(sockfd, SIOCGIFINDEX, &ir) < 0) {
        perror("ioctl()");
        close(sockfd);
        return -2;
    }
    /* IP do dispositivo */
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = protocol;
    addr.sll_ifindex = ir.ifr_ifindex;
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind()");
        close(sockfd);
        return -3;
    }
    /* modo promíscuo (escuta a rede) */
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ir.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(sockfd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr))
        < 0) {
        perror("setsockopt()");
        close(sockfd);
        return -4;
    }
    return sockfd;
}
