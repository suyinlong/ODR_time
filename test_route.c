/*
* @Author: Yinlong Su
* @Date:   2015-11-19 10:53:06
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-19 16:28:16
*/

#include "np.h"

int main() {
    char ipaddr[IPADDR_BUFFSIZE];
    bzero(ipaddr, IPADDR_BUFFSIZE);

    char data[ODR_FRAME_PAYLOAD];
    bzero(data, ODR_FRAME_PAYLOAD);

    odr_itable *itable = Get_hw_addrs(ipaddr);
    odr_itable *item;

    int sockfd = Socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_ID));

    odr_frame frame;
    bzero(&frame, sizeof(frame));
    for (item = itable; item != NULL; item = item->hwa_next) {
        build_bcast_frame(&frame, item->if_haddr, ODR_FRAME_INTERFACE, data);
        send_frame(sockfd, item->if_index, &frame, PACKET_BROADCAST);

        build_bcast_frame(&frame, item->if_haddr, ODR_FRAME_ROUTE, data);
        send_frame(sockfd, item->if_index, &frame, PACKET_BROADCAST);
    }

    free_hwa_info(itable);
    return 0;
}
