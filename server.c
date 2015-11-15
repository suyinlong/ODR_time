/*
* @File: server.c
* @Date: 2015-11-08 20:56:53
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-14 20:30:30
* @Description:
*     + int main(int argc, char **argv)
*         [Server entry function]
*/

#include "np.h"

/* --------------------------------------------------------------------------
 *  main
 *
 *  Entry function
 *
 *  @param  : int   argc
 *            char  **argv
 *  @return : int
 *
 *  Server entry function
 * --------------------------------------------------------------------------
 */
int main(int argc, char **argv) {
    int     r, sockfd, port = 0;
    time_t  ticks;
    char    data[ODR_DGRAM_DATALEN];
    char    cli_ipaddr[IPADDR_BUFFSIZE], cli_hostname[HOSTNAME_BUFFSIZE];
    char    srv_ipaddr[IPADDR_BUFFSIZE], srv_hostname[HOSTNAME_BUFFSIZE];
    struct sockaddr_un cliaddr;
    struct sockaddr_un servaddr;

    unlink(servaddr.sun_path);

    // get IP address of current node
    free_hwa_info(Get_hw_addrs(srv_ipaddr));
    util_ip_to_hostname(srv_ipaddr, srv_hostname);

    // create and bind domain socket
    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, TIMESERV_PATH);

    Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));

    // receive request from domain socket
    while (1) {
        r = msg_recv(sockfd, data, cli_ipaddr, &port);
        if (r <= 0)
            continue;
        util_ip_to_hostname(cli_ipaddr, cli_hostname);

        ticks = time(NULL);
        snprintf(data, ODR_DGRAM_DATALEN, "%.24s", ctime(&ticks));

        r = msg_send(sockfd, cli_ipaddr, port, data, 0);
        printf("server at node %s: responding to request from %s\n", srv_hostname, cli_hostname);
    }

    unlink(servaddr.sun_path);


    exit(0);
}
