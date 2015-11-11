/*
* @Author: Yinlong Su
* @Date:   2015-11-10 22:56:21
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-10 23:02:14
*/

#include "np.h"

void util_ip_to_hostname(const char *ipaddr, char *hostname) {
    if (strcmp(ipaddr, "130.245.156.19") == 0)
        strcpy(hostname, "minix");
    else if (strcmp(ipaddr, "130.245.156.20") == 0)
        strcpy(hostname, "vm10");
    else if (strcmp(ipaddr, "130.245.156.21") == 0)
        strcpy(hostname, "vm1");
    else if (strcmp(ipaddr, "130.245.156.22") == 0)
        strcpy(hostname, "vm2");
    else if (strcmp(ipaddr, "130.245.156.23") == 0)
        strcpy(hostname, "vm3");
    else if (strcmp(ipaddr, "130.245.156.24") == 0)
        strcpy(hostname, "vm4");
    else if (strcmp(ipaddr, "130.245.156.25") == 0)
        strcpy(hostname, "vm5");
    else if (strcmp(ipaddr, "130.245.156.26") == 0)
        strcpy(hostname, "vm6");
    else if (strcmp(ipaddr, "130.245.156.27") == 0)
        strcpy(hostname, "vm7");
    else if (strcmp(ipaddr, "130.245.156.28") == 0)
        strcpy(hostname, "vm8");
    else if (strcmp(ipaddr, "130.245.156.29") == 0)
        strcpy(hostname, "vm9");
    else
        strcpy(hostname, "unknown");
}
