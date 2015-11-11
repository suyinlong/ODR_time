/*
* @File: client.c
* @Date: 2015-11-08 20:57:18
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-11 12:01:27
*/

#include "np.h"

int main(int argc, char **argv) {
    char ipaddr[IPADDR_BUFFSIZE];
    int i;

    printf("client\n");

    odr_itable *itable, *item;
    int sockid, s;

    itable = Get_hw_addrs(ipaddr);

    printf("IP address: %s\n", ipaddr);

    sockid = Socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_ID));

    while (1) {
        for (item = itable; item != NULL; item = item->hwa_next) {
            s = send_bcast_frame(sockid, item->if_index, item->if_haddr, ipaddr, IPADDR_BUFFSIZE);
            printf("Send frame size %d to interface [%d] %s, MAC address = ", s, item->if_index, item->if_name);
            for (i = 0; i < 6; i++)
                printf("%.2x ", item->if_haddr[i] & 0xff);
            printf("\n");
        }
        sleep(1);
    }
    return 0;
}
