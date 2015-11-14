/*
* @File: server.c
* @Date: 2015-11-08 20:56:53
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-13 20:49:36
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
    int sockfd;
    char buff[255];
    struct sockaddr_un servaddr;

    sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, TIMESERV_PATH);

    Bind(sockfd, (SA *)&servaddr, sizeof(servaddr));

    printf("%s\n", servaddr.sun_path);

    strcpy(buff, "Test.");
    //Sendto(sockfd, buff, 255, 0, (SA *)&odraddr, sizeof(odraddr));
    msg_send(sockfd, NULL, 0, buff, 0);


    unlink(servaddr.sun_path);


    exit(0);
}
