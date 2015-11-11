/*
* @Author: Yinlong Su
* @Date:   2015-11-08 20:56:07
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-10 23:02:36
*/

#include "np.h"

void parseHost(odr_object *obj) {
    struct hwa_info *hwa;
    struct sockaddr *sa;
    int i;

    for (hwa = obj->hwa; hwa != NULL; hwa = hwa->hwa_next) {
        if (strcmp(hwa->if_name, "eth0") == 0) {
            sa = hwa->ip_addr;
            strcpy(obj->ipaddr, Sock_ntop_host(sa, sizeof(*sa)));
            break;
        }
    }

    util_ip_to_hostname(obj->ipaddr, obj->hostname);
}

int main(int argc, char **argv) {
    odr_object obj;
    bzero(&obj, sizeof(odr_object));

    // command argument
    if (argc != 2)
        err_quit("usage: ODR_yinlsu <staleness time in seconds>");
    obj.staleness = atol(argv[1]);

    // Get interface information and canonical IP address / hostname
    obj.hwa = Get_hw_addrs();
    parseHost(&obj);

    printf("Node IP address: %s, hostname: %s\n", obj.ipaddr, obj.hostname);

    int sockid;

    sockid = Socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_ID));

    int s = send_frame(sockid);
    printf("Sent %d.\n", s);
    recv_frame(sockid);
    printf("Received.\n");
    free_hwa_info(obj.hwa);
    exit(0);
}
