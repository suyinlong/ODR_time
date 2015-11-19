/*
* @File: utils.c
* @Date: 2015-11-10 22:56:21
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-17 19:55:42
* @Description:
*     Util function library, some miscellaneous helper functions
*     + int util_ip_to_hostname(const char *ipaddr, char *hostname)
*         [Convert IP address to hostname]
*     + int util_hostname_to_ip(const char *hostname, char *ipaddr)
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
 *  @return : int           [ -1 if failed ]
 *
 *  Convert IP address to hostname
 * --------------------------------------------------------------------------
 */
int util_ip_to_hostname(const char *ipaddr, char *hostname) {
    struct sockaddr_in  sockaddr;
    struct hostent      *he;

    bzero(&sockaddr, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;

    inet_pton(AF_INET, ipaddr, &sockaddr.sin_addr);
    he = gethostbyaddr(&sockaddr.sin_addr, sizeof(sockaddr.sin_addr), AF_INET);
    if (he == NULL) {
        printf("util_ip_to_hostname error: gethostbyaddr error for %s\n", ipaddr);
        return -1;
    }
    strncpy(hostname, he->h_name, HOSTNAME_BUFFSIZE);

    return 0;
}

/* --------------------------------------------------------------------------
 *  util_hostname_to_ip
 *
 *  Util function
 *
 *  @param  : const char    *hostname   [Hostname]
 *            char          *ipaddr     [IP address]
 *  @return : int           [ -1 if failed ]
 *
 *  Convert hostname to IP address
 * --------------------------------------------------------------------------
 */
int util_hostname_to_ip(const char *hostname, char *ipaddr) {
    struct hostent      *he;
    he = gethostbyname(hostname);
    if (he == NULL) {
        printf("util_hostname_to_ip error: gethostbyname error for %s\n", hostname);
        return -1;
    }
    strncpy(ipaddr, inet_ntoa(*(struct in_addr*)he->h_addr), IPADDR_BUFFSIZE);
    return 0;
}
