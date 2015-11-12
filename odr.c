/*
* @File: odr.c
* @Date: 2015-11-08 20:56:07
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-11 23:38:16
*/

#include "np.h"

void process_packet_frame(int sockfd) {
    recv_frame(sockfd);
}

void process_domain_dgram(int sockfd) {
    int n;
    char buff[255];
    struct sockaddr from;
    socklen_t addrlen;

    n = Recvfrom(sockfd, buff, 255, 0, &from, &addrlen);
    printf("Received from [%s]: %s\n", ((struct sockaddr_un *)&from)->sun_path, buff);

}
void create_sockets(odr_object *obj) {
    struct sockaddr_un odraddr;

    // Create PF_PACKET Socket
    obj->p_sockfd = Socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_ID));

    bzero(&odraddr, sizeof(odraddr));
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);

    // Create UNIX Domain Socket
    unlink(ODR_PATH);
    obj->d_sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    Bind(obj->d_sockfd, (SA *)&odraddr, sizeof(odraddr));
}

void process_sockets(odr_object *obj) {
    int maxfdp1 = max(obj->p_sockfd, obj->d_sockfd) + 1;
    int r;
    fd_set rset;

    FD_ZERO(&rset);
    while (1) {
        FD_SET(obj->p_sockfd, &rset);
        FD_SET(obj->d_sockfd, &rset);

        r = Select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(obj->p_sockfd, &rset)) {
            // from PF_PACKET Socket
            process_packet_frame(obj->p_sockfd);
        }

        if (FD_ISSET(obj->d_sockfd, &rset)) {
            // from Domain Socket
            process_domain_dgram(obj->d_sockfd);
        }
    }
}

int main(int argc, char **argv) {

    // command argument
    if (argc != 2)
        err_quit("usage: ODR_yinlsu <staleness time in seconds>");

    odr_object obj;
    bzero(&obj, sizeof(odr_object));
    obj.staleness = atol(argv[1]);

    // Get interface information and canonical IP address / hostname
    obj.itable = Get_hw_addrs(obj.ipaddr);
    util_ip_to_hostname(obj.ipaddr, obj.hostname);

    create_sockets(&obj);

    printf("[ODR] Node IP address: %s, hostname: %s, path: %s\n", obj.ipaddr, obj.hostname, ODR_PATH);

    process_sockets(&obj);

    free_hwa_info(obj.itable);
    exit(0);
}
