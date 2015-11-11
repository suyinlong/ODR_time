/*
* @Author: Yinlong Su
* @Date:   2015-11-10 22:45:45
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-10 23:15:07
*/

#include "np.h"

int send_frame(int sockid) {
    struct sockaddr_ll socket_address;
    void *buffer = (void*)Malloc(ETH_FRAME_LEN);
    unsigned char* etherhead = buffer;
    unsigned char* data = buffer+14;
    struct ethhdr *eh = (struct ethhdr*)etherhead;

    int send_result = 0;

    unsigned char src_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    unsigned char dest_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    socket_address.sll_family   = PF_PACKET;
    socket_address.sll_protocol = htons(PROTOCOL_ID);
    socket_address.sll_ifindex  = 2;
    socket_address.sll_hatype   = ARPHRD_ETHER;
    socket_address.sll_pkttype  = PACKET_BROADCAST;
    socket_address.sll_halen    = ETH_ALEN;

    socket_address.sll_addr[0]  = 0xff;
    socket_address.sll_addr[1]  = 0xff;
    socket_address.sll_addr[2]  = 0xff;
    socket_address.sll_addr[3]  = 0xff;
    socket_address.sll_addr[4]  = 0xff;
    socket_address.sll_addr[5]  = 0xff;

    socket_address.sll_addr[6]  = 0x00;
    socket_address.sll_addr[7]  = 0x00;

    memcpy((void*)buffer, (void*)dest_mac, ETH_ALEN);
    memcpy((void*)(buffer+ETH_ALEN), (void*)src_mac, ETH_ALEN);
    eh->h_proto = htons(PROTOCOL_ID);

    data[0] = 'T';
    data[1] = 'E';
    data[2] = 'S';
    data[3] = 'T';
    data[4] = 0;

    return sendto(sockid, buffer, ETH_FRAME_LEN, 0,
          (struct sockaddr*)&socket_address, sizeof(socket_address));

}

int recv_frame(int sockid) {
    struct sockaddr_ll recv_address;
    bzero(&recv_address, sizeof(struct sockaddr_ll));
    socklen_t addrlen;
    void *buffer = (void*)Malloc(ETH_FRAME_LEN);
    int length = 0;
    length = recvfrom(sockid, buffer, ETH_FRAME_LEN, 0, (struct sockaddr*)&recv_address, &addrlen);
    int i;
    unsigned char *data = buffer+14;
    printf("%s\n", data);
    printf("Interface index: %d, MAC address: ", recv_address.sll_ifindex);

    for (i = 0; i < 6; i++)
        printf("%.2x ", recv_address.sll_addr[i] & 0xff);
    return length;
}
