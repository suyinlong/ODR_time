/*
* @File: odr.c
* @Date: 2015-11-08 20:56:07
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-11 10:35:48
*/

#include "np.h"

int main(int argc, char **argv) {
    odr_object obj;
    bzero(&obj, sizeof(odr_object));

    // command argument
    if (argc != 2)
        err_quit("usage: ODR_yinlsu <staleness time in seconds>");
    obj.staleness = atol(argv[1]);

    // Get interface information and canonical IP address / hostname
    obj.itable = Get_hw_addrs(obj.ipaddr);
    util_ip_to_hostname(obj.ipaddr, obj.hostname);

    printf("Node IP address: %s, hostname: %s\n", obj.ipaddr, obj.hostname);

    odr_itable *item;

    for (item = obj.itable; item != NULL; item=item->hwa_next) {
        printf("Interface name: %s\n", item->if_name);
    }

    int sockid;

    sockid = Socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_ID));

    while (1) {
        recv_frame(sockid);
        //printf("Received.\n");
    }
    free_hwa_info(obj.itable);
    exit(0);
}
