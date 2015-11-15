/*
* @File: odr.c
* @Date: 2015-11-08 20:56:07
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-14 20:24:50
* @Description:
*     ODR main program, provides maintenance features of odr_object
*     - int get_port_ptable(const char *path, odr_object *obj)
*         [ODR ptable path-port finder]
*     - void process_frame(odr_object *obj)
*         [ODR PF_PACKET socket frame processor]
*     - void process_domain_dgram(odr_object *obj)
*         [ODR Domain socket datagram processor]
*     - odr_ptable *create_ptable()
*         [odr_ptable constructor]
*     - void create_sockets(odr_object *obj)
*         [PF_PACKET socket and domain socket constructor]
*     - void process_sockets(odr_object *obj)
*         [ODR Sockets processor]
*     - void free_odr_object(odr_object *obj)
*         [odr_object destructor]
*     + int main(int argc, char **argv)
*         [ODR service entry function]
*/

#include "np.h"

/* --------------------------------------------------------------------------
 *  get_port_ptable
 *
 *  Ptable path-port finder
 *
 *  @param  : const char            *path   [pathname]
 *            odr_object            *obj    [odr object]
 *  @return : int                   [port number]
 *
 *  Find the port number to the path in path-port table
 *  If the path is in table, update the timestamp
 *  Otherwise, add the path-port entry to the table
 * --------------------------------------------------------------------------
 */
int get_port_ptable(const char *path, odr_object *obj) {
    odr_ptable *item = obj->ptable, *newitem;

    // find the path in table
    while (item) {
        if (strcmp(item->path, path) == 0)
            break;
        item = item->next;
    }

    if (item) {
        // if found, update timestamp and return port number
        if (item->timestamp > 0)
            item->timestamp = time(NULL);
        printf("[ptable] Path: %s, Port: %d, Timestamp: %ld\n", item->path, item->port, item->timestamp);
        return item->port;
    } else {
        // otherwise, create new one then plug in
        newitem = (odr_ptable *)Calloc(1, sizeof(odr_ptable));

        if (obj->free_port == 0xffff)
            obj->free_port = TIMESERV_PORT;

        newitem->port = ++obj->free_port;
        strcpy(newitem->path, path);
        newitem->timestamp = time(NULL);
        newitem->next = obj->ptable->next;

        obj->ptable->next = newitem;
        printf("[ptable] New Path: %s, Port: %d, Timestamp: %ld\n", newitem->path, newitem->port, newitem->timestamp);
        return newitem->port;
    }
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
        frame_rreq_handler(obj, &frame, &from);
        break;
    case ODR_FRAME_RREP:
        frame_rrep_handler(obj, &frame, &from);
        break;
    case ODR_FRAME_APPMSG:
        frame_appmsg_handler(obj, &frame, &from);
        break;
    case ODR_FRAME_ROUTE:
        debug_route_handler(obj);
        break;
    case ODR_FRAME_DATA:
        debug_data_handler(&frame, &from, fromlen);
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
void process_domain_dgram(odr_object *obj) {
    int n, port;
    odr_dgram dgram;
    struct sockaddr_un from;
    socklen_t addrlen = sizeof(from);

    bzero(&dgram, sizeof(dgram));
    n = Recvfrom(obj->d_sockfd, &dgram, sizeof(dgram), 0, (SA *)&from, &addrlen);
    printf("Received from [%s]: %s\n", from.sun_path, dgram.data);

    port = get_port_ptable(from.sun_path, obj);

    // build apacket item
    odr_queue_item *item = (odr_queue_item *)Calloc(1, sizeof(odr_queue_item));

    strcpy(item->apacket.dst, dgram.ipaddr);
    item->apacket.dst_port = dgram.port;
    strcpy(item->apacket.src, obj->ipaddr);
    item->apacket.src_port = port;
    item->apacket.length = strlen(dgram.data);
    strcpy(item->apacket.data, dgram.data);

    item->next = NULL;

    // insert into queue
    if (obj->queue.head == NULL) {
        obj->queue.head = item;
        obj->queue.tail = item;
    } else {
        obj->queue.tail->next = item;
        obj->queue.tail = item;
    }
    printf("Queued up app_packet [DST: %s:%d SRC: %s:%d DATA(%d): %s]\n", item->apacket.dst, item->apacket.dst_port, item->apacket.src, item->apacket.src_port, item->apacket.length, item->apacket.data);

    // queue_handler(obj);

    // for testing, send back
    dgram.data[0] = 'T';
    sendto(obj->d_sockfd, &dgram, sizeof(dgram), 0, (SA *)&from, addrlen);
    printf("Send back.\n");
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
            process_domain_dgram(obj);
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
    odr_queue_item *q, *qnext;

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

    q = obj->queue.head;
    while (q) {
        qnext = q->next;
        free(q);
        q = qnext;
    }
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
    obj.free_port = TIMESERV_PORT;

    // Get interface information and canonical IP address / hostname
    obj.itable = Get_hw_addrs(obj.ipaddr);
    obj.rtable = NULL;
    obj.ptable = create_ptable();
    util_ip_to_hostname(obj.ipaddr, obj.hostname);

    obj.queue.head = NULL;
    obj.queue.tail = NULL;

    create_sockets(&obj);

    printf("[ODR] Node IP address: %s, hostname: %s, path: %s\n", obj.ipaddr, obj.hostname, ODR_PATH);

    process_sockets(&obj);

    free_odr_object(&obj);
    exit(0);
}
