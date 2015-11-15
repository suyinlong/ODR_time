/*
* @File: utils.c
* @Date: 2015-11-10 22:56:21
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-14 20:32:15
* @Description:
*     Util function library, some miscellaneous helper functions
*     + void util_ip_to_hostname(const char *ipaddr, char *hostname)
*         [Convert IP address to hostname]
*     + void util_hostname_to_ip(const char *hostname, char *ipaddr)
*         [Convert hostname to IP address]
*/

#include "np.h"

/* --------------------------------------------------------------------------
 *  util_ip_to_hostname
 *
 *  Util function
 *
 *  @param  : const char    *ipaddr     [IP address]
 *            char          *hostname   [Hostname]
 *  @return : void
 *
 *  Convert IP address to hostname
 * --------------------------------------------------------------------------
 */
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

/* --------------------------------------------------------------------------
 *  util_hostname_to_ip
 *
 *  Util function
 *
 *  @param  : const char    *hostname   [Hostname]
 *            char          *ipaddr     [IP address]
 *  @return : void
 *
 *  Convert hostname to IP address
 * --------------------------------------------------------------------------
 */
void util_hostname_to_ip(const char *hostname, char *ipaddr) {
    if (strcmp(hostname, "vm10") == 0)
        strcpy(ipaddr, "130.245.156.20");
    else if (strcmp(hostname, "vm1") == 0)
        strcpy(ipaddr, "130.245.156.21");
    else if (strcmp(hostname, "vm2") == 0)
        strcpy(ipaddr, "130.245.156.22");
    else if (strcmp(hostname, "vm3") == 0)
        strcpy(ipaddr, "130.245.156.23");
    else if (strcmp(hostname, "vm4") == 0)
        strcpy(ipaddr, "130.245.156.24");
    else if (strcmp(hostname, "vm5") == 0)
        strcpy(ipaddr, "130.245.156.25");
    else if (strcmp(hostname, "vm6") == 0)
        strcpy(ipaddr, "130.245.156.26");
    else if (strcmp(hostname, "vm7") == 0)
        strcpy(ipaddr, "130.245.156.27");
    else if (strcmp(hostname, "vm8") == 0)
        strcpy(ipaddr, "130.245.156.28");
    else if (strcmp(hostname, "vm9") == 0)
        strcpy(ipaddr, "130.245.156.29");
}
