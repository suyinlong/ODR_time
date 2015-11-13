/*
* @File: odr_frame.c
* @Date: 2015-11-10 22:45:45
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-12 23:47:59
*/

#include "np.h"

void build_frame_header(odr_frame *frame, uchar *dst_mac, uchar *src_mac, ushort ftype) {

    memcpy(frame->h_dest, dst_mac, ETH_ALEN);
    memcpy(frame->h_source, src_mac, ETH_ALEN);

    frame->h_proto = htons(PROTOCOL_ID);
    frame->h_type = ftype;
}

void build_frame(odr_frame *frame, uchar *dst_mac, uchar *src_mac, ushort ftype, void *data) {
    build_frame_header(frame, dst_mac, src_mac, ftype);
    memcpy(frame->data, data, ODR_FRAME_PAYLOAD);
}

void build_bcast_frame(odr_frame *frame, uchar *src_mac, ushort ftype, void *data) {
    uchar dst_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    build_frame(frame, dst_mac, src_mac, ftype, data);
}

int send_frame(int sockfd, int if_index, odr_frame *frame, uchar pkttype) {
    int i;
    struct sockaddr_ll socket_address;

    socket_address.sll_family   = PF_PACKET;
    socket_address.sll_protocol = htons(PROTOCOL_ID);
    socket_address.sll_ifindex  = if_index;
    socket_address.sll_hatype   = ARPHRD_ETHER;
    socket_address.sll_pkttype  = pkttype;
    socket_address.sll_halen    = ETH_ALEN;

    // Copy destination mac address
    for (i = 0; i < 6; i++)
        socket_address.sll_addr[i] = frame->h_dest[i];
    // unused part
    socket_address.sll_addr[6]  = 0x00;
    socket_address.sll_addr[7]  = 0x00;

    return sendto(sockfd, frame, sizeof(*frame), 0,
          (struct sockaddr*)&socket_address, sizeof(socket_address));

}

int recv_frame(int sockfd, odr_frame *frame, struct sockaddr *from, socklen_t *fromlen) {
    return recvfrom(sockfd, frame, sizeof(odr_frame), 0, from, fromlen);
}
