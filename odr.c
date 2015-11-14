/*
* @File: odr.c
* @Date: 2015-11-08 20:56:07
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-13 19:51:26
*/

#include "np.h"

/* --------------------------------------------------------------------------
 *  debug_route
 *
 *  Debug function to print route table
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  Print all content in route table
 * --------------------------------------------------------------------------
 */
void debug_route(odr_object *obj) {
    // print out all route items
    int i;
    odr_rtable *r = obj->rtable;

    while (r) {
        printf("| %-*s | ", IPADDR_BUFFSIZE, r->dst);
        for (i = 0; i < 5; i++)
            printf("%.2x:", r->nexthop[i] & 0xff);
        printf("%.2x | ", r->nexthop[i] & 0xff);
        printf("%2d | ", r->index);
        printf("%2d | ", r->hopcnt);
        printf("%4d | ", r->bcast_id);
        printf("\n");
        r = r->next;
    }
}

/* --------------------------------------------------------------------------
 *  debug_data
 *
 *  Debug function to print pure text
 *
 *  @param  : odr_frame             *frame  [frame]
 *            struct sockaddr_ll    *from   [sender address structure]
 *            socklen_t             fromlen [sender address structure length]
 *  @return : void
 *
 *  Print pure text
 * --------------------------------------------------------------------------
 */
void debug_data(odr_frame *frame, struct sockaddr_ll *from, socklen_t fromlen) {
    int i;

    printf("Interface index: %d, MAC address: ", from->sll_ifindex);
    for (i = 0; i < 6; i++)
        printf("%.2x%s", from->sll_addr[i] & 0xff, ((i == 5) ? " " : ":"));
    printf("Data: %s\n", frame->data);
}

/* --------------------------------------------------------------------------
 *  process_frame
 *
 *  ODR service frame processor
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  When receives a frame from PF_PACKET socket, use different function
 *  to process the frame
 * --------------------------------------------------------------------------
 */
void process_frame(odr_object *obj) {
    int len;
    struct sockaddr_ll from;
    bzero(&from, sizeof(struct sockaddr_ll));
    socklen_t fromlen = sizeof(struct sockaddr_ll);

    odr_frame frame;
    bzero(&frame, sizeof(frame));

    len = recv_frame(obj->p_sockfd, &frame, (SA *)&from, &fromlen);

    switch (frame.h_type) {
    case ODR_FRAME_RREQ:
        break;
    case ODR_FRAME_RREP:
        break;
    case ODR_FRAME_APPMSG:
        break;
    case ODR_FRAME_ROUTE:
        debug_route(obj);
        break;
    case ODR_FRAME_DATA:
        debug_data(&frame, &from, fromlen);
    }

}

/* --------------------------------------------------------------------------
 *  process_domain_dgram
 *
 *  ODR service domain datagram processor
 *
 *  @param  :
 *  @return : void
 *
 *  When receives a datagram from Domain socket, use different function to
 *  handle server/client request
 * --------------------------------------------------------------------------
 */
void process_domain_dgram(int sockfd) {
    int n;
    char buff[255];
    struct sockaddr from;
    socklen_t addrlen;

    n = Recvfrom(sockfd, buff, 255, 0, &from, &addrlen);
    printf("Received from [%s]: %s\n", ((struct sockaddr_un *)&from)->sun_path, buff);

}

/* --------------------------------------------------------------------------
 *  create_ptable()
 *
 *  Create permanent item of ptable (port-path table)
 *
 *  @param  : void
 *  @return : odr_ptable *
 *
 *  When starting the odr service, insert a permanent item into ptable:
 *      <TIMESERV_PORT, TIMESERV_PATH, 0>
 *      where 0 stands for permanent
 * --------------------------------------------------------------------------
 */
odr_ptable *create_ptable() {
    odr_ptable *phead = (odr_ptable *) Calloc(1, sizeof(odr_ptable));

    phead->port = TIMESERV_PORT;
    strcpy(phead->path, TIMESERV_PATH);
    phead->timestamp = 0;
    phead->next = NULL;

    return phead;
}

/* --------------------------------------------------------------------------
 *  create_sockets
 *
 *  Create sockets for ODR service
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  Create a PF_PACKET socket for frame communication
 *  Create a Domain socket for datagram communication
 * --------------------------------------------------------------------------
 */
void create_sockets(odr_object *obj) {
    struct sockaddr_un odraddr;

    // Create PF_PACKET Socket
    obj->p_sockfd = Socket(PF_PACKET, SOCK_RAW, htons(PROTOCOL_ID));

    bzero(&odraddr, sizeof(odraddr));
    odraddr.sun_family = AF_LOCAL;
    strcpy(odraddr.sun_path, ODR_PATH);

    // Create UNIX Domain Socket
    unlink(ODR_PATH);
    obj->d_sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
    Bind(obj->d_sockfd, (SA *)&odraddr, sizeof(odraddr));
}

/* --------------------------------------------------------------------------
 *  process_sockets
 *
 *  Main loop of ODR service
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *  @see    : function#process_frame
 *            function#process_domain_dgram
 *
 *  Wait for the message from PF_PACKET socket or Domain socket
 *  then process it
 * --------------------------------------------------------------------------
 */
void process_sockets(odr_object *obj) {
    int maxfdp1 = max(obj->p_sockfd, obj->d_sockfd) + 1;
    int r;
    fd_set rset;

    FD_ZERO(&rset);
    while (1) {
        FD_SET(obj->p_sockfd, &rset);
        FD_SET(obj->d_sockfd, &rset);

        r = Select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(obj->p_sockfd, &rset)) {
            // from PF_PACKET Socket
            process_frame(obj);
        }

        if (FD_ISSET(obj->d_sockfd, &rset)) {
            // from Domain Socket
            process_domain_dgram(obj->d_sockfd);
        }
    }
}

/* --------------------------------------------------------------------------
 *  free_odr_object
 *
 *  odr_object destroyer
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  Free the memory space of odr_object
 * --------------------------------------------------------------------------
 */
void free_odr_object(odr_object *obj) {
    odr_rtable *r, *rnext;
    odr_ptable *p, *pnext;

    free_hwa_info(obj->itable);

    r = obj->rtable;
    while (r) {
        rnext = r->next;
        free(r);
        r = rnext;
    }

    p = obj->ptable;
    while (p) {
        pnext = p->next;
        free(p);
        p = pnext;
    }

    // free queue

}

/* --------------------------------------------------------------------------
 *  main
 *
 *  Entry function
 *
 *  @param  : int   argc
 *            char  **argv
 *  @return : int
 *  @see    : function#Get_hw_addrs
 *            function#create_ptable
 *            function#util_ip_to_hostname
 *            function#create_sockets
 *            function#process_sockets
 *            function#free_odr_object
 *
 *  ODR service entry function
 * --------------------------------------------------------------------------
 */
int main(int argc, char **argv) {

    // command argument
    if (argc != 2)
        err_quit("usage: ODR_yinlsu <staleness time in seconds>");

    odr_object obj;
    bzero(&obj, sizeof(odr_object));
    obj.staleness = atol(argv[1]);
    obj.bcast_id = 0;

    // Get interface information and canonical IP address / hostname
    obj.itable = Get_hw_addrs(obj.ipaddr);
    obj.rtable = NULL;
    obj.ptable = create_ptable();
    util_ip_to_hostname(obj.ipaddr, obj.hostname);

    create_sockets(&obj);

    printf("[ODR] Node IP address: %s, hostname: %s, path: %s\n", obj.ipaddr, obj.hostname, ODR_PATH);

    process_sockets(&obj);

    free_odr_object(&obj);
    exit(0);
}
