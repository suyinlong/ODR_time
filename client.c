/*
* @File: client.c
* @Date: 2015-11-08 20:57:18
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-18 22:33:43
* @Description:
*     + int main(int argc, char **argv)
*         [Client entry function]
*/

#include "np.h"

/* --------------------------------------------------------------------------
 *  main
 *
 *  Client Entry function
 *
 *  @param  : int   argc
 *            char  **argv
 *  @return : int
 *
 *  1. Get node IP address and hostname
 *  2. Create UNIX domain Socket and bind to a temporary path
 *  3. - Work cycle -
 *     a. Prompt the user to choose the server node
 *     b. Use ODR API to send request to the server node
 *     c. If receive a response, print out and start the cycle again;
 *        Else if first timeout, go to step b and try again;
 *        Otherwise, the request is failed, start the cycle again.
 * --------------------------------------------------------------------------
 */
int main(int argc, char **argv) {
    int     i, sockfd, fd, resend = 0, port = 0;
    char    data[ODR_DGRAM_DATALEN];
    char    cli_ipaddr[IPADDR_BUFFSIZE], cli_hostname[HOSTNAME_BUFFSIZE];
    char    srv_ipaddr[IPADDR_BUFFSIZE], srv_hostname[HOSTNAME_BUFFSIZE];
    struct sockaddr_un cliaddr;

    // get IP address of current node
    free_hwa_info(Get_hw_addrs(cli_ipaddr));
    util_ip_to_hostname(cli_ipaddr, cli_hostname);

    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, TIMECLIE_PATH);

    fd = mkstemp(cliaddr.sun_path);
    // Call unlink so that whenever the file is closed or the program exits
    // the temporary file is deleted
    unlink(cliaddr.sun_path);

    // Create UNIX Domain Socket
    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    Bind(sockfd, (SA *)&cliaddr, sizeof(cliaddr));

    printf("client at node %s open socket %d on path %s\n", cli_hostname, sockfd, cliaddr.sun_path);

    // work cycle
    while (1) {
        resend = 0;
        bzero(srv_ipaddr, IPADDR_BUFFSIZE);
        bzero(srv_hostname, HOSTNAME_BUFFSIZE);
        bzero(data, ODR_DGRAM_DATALEN);

        printf("\nPlease type server node [vm1 - vm10] or 'exit': ");
        Fgets(srv_hostname, HOSTNAME_BUFFSIZE, stdin);
        i = 0;
        // remove 'enter' characters
        while (i < HOSTNAME_BUFFSIZE) {
            if (srv_hostname[i] < 32)
                srv_hostname[i] = 0;
            i++;
        }
        if (strcmp(srv_hostname, "exit") == 0)
            break;
        util_hostname_to_ip(srv_hostname, srv_ipaddr);

        if (strlen(srv_ipaddr) == 0) {
            printf("Node '%s' does not exist!\n", srv_hostname);
            continue;
        }

sendagain:
        strcpy(data, "R");
        printf("client at node %s: send request to server at %s %s\n", cli_hostname, srv_hostname, (resend ? "(forced discovery)" : ""));
        msg_send(sockfd, srv_ipaddr, TIMESERV_PORT, data, resend);

        bzero(data, ODR_DGRAM_DATALEN);
        i = msg_recv(sockfd, data, srv_ipaddr, &port);
        if (i > 0) {
            printf("client at node %s: received from %s <%s>\n", cli_hostname, srv_hostname, data);
        } else if (i == 0 && resend == 0) {
            resend = 1;
            printf("client at node %s: timeout on response from %s\n", cli_hostname, srv_hostname);
            goto sendagain;
        } else {
            printf("client at node %s: failed on communication to %s\n", cli_hostname, srv_hostname);
        }
    }
    unlink(cliaddr.sun_path);
    return 0;
}
