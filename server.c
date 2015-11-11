/*
* @File: server.c
* @Date: 2015-11-08 20:56:53
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-11 10:31:37
*/

#include "np.h"

int main(int argc, char **argv) {
    struct hwa_info *hwa, *hwahead;
    struct sockaddr *sa;
    char   *ptr;
    int    i, prflag;
    char ipaddr[IPADDR_BUFFSIZE];

    printf("char* size=%ld, void* size=%ld, int size=%ld\n", sizeof(char*), sizeof(void*), sizeof(int));

    for (hwahead = hwa = Get_hw_addrs(ipaddr); hwa != NULL; hwa = hwa->hwa_next) {

        printf("%s :%s", hwa->if_name, ((hwa->ip_alias) == IP_ALIAS) ? " (alias)\n" : "\n");

        if ( (sa = hwa->ip_addr) != NULL)
            printf("         IP addr = %s\n", Sock_ntop_host(sa, sizeof(*sa)));

        prflag = 0;
        i = 0;
        do {
            if (hwa->if_haddr[i] != '\0') {
                prflag = 1;
                break;
            }
        } while (++i < IF_HADDR);

        if (prflag) {
            printf("         HW addr = ");
            ptr = hwa->if_haddr;
            i = IF_HADDR;
            do {
                printf("%.2x%s", *ptr++ & 0xff, (i == 1) ? " " : ":");
            } while (--i > 0);
        }

        printf("\n         interface index = %d\n\n", hwa->if_index);
    }

    free_hwa_info(hwahead);
    exit(0);
}
