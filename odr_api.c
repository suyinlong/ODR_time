/*
* @File: odr_api.c
* @Date: 2015-11-13 00:15:31
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-13 21:35:20
*/

#include "np.h"

/* --------------------------------------------------------------------------
 *  msg_send
 *
 *  ODR API Message send function
 *
 *  @param  : int   sockfd  [Socket file descriptor]
 *            char  *dst    [Destination IP address]
 *            int   port    [Destination Port number]
 *            char  *data   [Data payload]
 *            int   flag    [Forced rediscovery flag]
 *  @return : int           [The number of sent bytes, -1 if failed]
 *
 *  ODR API function, send message to ODR
 * --------------------------------------------------------------------------
 */
int msg_send(int sockfd, char *dst, int port, char *data, int flag) {
    struct sockaddr_un odraddr;
    odr_dgram dgram;

    bzero(&odraddr, sizeof(odraddr));
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);

    bzero(&dgram, sizeof(dgram));
    strcpy(dgram.ipaddr, dst);
    dgram.port = port;
    dgram.flag = flag;
    strcpy(dgram.data, data);

    return sendto(sockfd, &dgram, sizeof(dgram), 0, (SA *)&odraddr, sizeof(odraddr));
}

/* --------------------------------------------------------------------------
 *  msg_recv
 *
 *  ODR API Message receive function
 *
 *  @param  : int   sockfd  [Socket file descriptor]
 *            char  *data   [Data payload]
 *            char  *src    [Source IP address]
 *            int   port    [Source Port number]
 *  @return : int           [The number of received bytes, -1 if failed]
 *
 *  ODR API function, receive message from ODR
 * --------------------------------------------------------------------------
 */
int msg_recv(int sockfd, char *data, char *src, int *port) {
    odr_dgram dgram;
    bzero(&dgram, sizeof(dgram));

    int r = recvfrom(sockfd, &dgram, sizeof(dgram), 0, NULL, NULL);

    strcpy(data, dgram.data);
    strcpy(src, dgram.ipaddr);
    *port = dgram.port;

    return r;
}
