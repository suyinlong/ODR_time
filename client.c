/*
* @File: client.c
* @Date: 2015-11-08 20:57:18
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-12 23:48:32
*/

#include "np.h"

int main(int argc, char **argv) {
    char ipaddr[ODR_FRAME_PAYLOAD];
    int i;

    printf("client\n");

    odr_itable *itable, *item;
    int sockfd, s;

    itable = Get_hw_addrs(ipaddr);

    printf("IP address: %s\n", ipaddr);

    sockfd = Socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_ID));

    while (1) {
        for (item = itable; item != NULL; item = item->hwa_next) {
            odr_frame frame;

            build_bcast_frame(&frame, item->if_haddr, ODR_FRAME_DATA, ipaddr);
            s = send_frame(sockfd, item->if_index, &frame, PACKET_BROADCAST);

            printf("Send frame size %d to interface [%d] %s, MAC address = ", s, item->if_index, item->if_name);
            for (i = 0; i < 6; i++)
                printf("%.2x ", item->if_haddr[i] & 0xff);
            printf("\n");
        }
        sleep(1);
    }
    return 0;
}
