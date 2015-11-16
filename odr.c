/*
* @File: odr.c
* @Date: 2015-11-08 20:56:07
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-15 21:57:07
* @Description:
*     ODR main program, provides maintenance features of odr_object
*     + odr_itable *get_item_itable(int index, odr_object *obj)
*         [ODR itable index finder]
*     + odr_rtable *get_item_rtable(const char *ipaddr, odr_object *obj)
*         [ODR rtable routing path finder]
*     - int get_port_ptable(const char *path, odr_object *obj)
*         [ODR ptable path-port finder]
*     - void purge_tables(odr_object *obj)
*         [ODR service tables purge function]
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
 *  get_item_itable
 *
 *  Itable index finder
 *
 *  @param  : int                   index   [Interface index]
 *            odr_object            *obj    [odr object]
 *  @return : odr_itable *          [interface entry]
 *
 *  Find the interface information of given index
 * --------------------------------------------------------------------------
 */
odr_itable *get_item_itable(int index, odr_object *obj) {
    odr_itable *item = obj->itable;

    // find the item in itable
    while (item) {
        if (item->if_index == index)
            break;
        item = item->hwa_next;
    }

    return item;
}

/* --------------------------------------------------------------------------
 *  get_item_rtable
 *
 *  Rtable routing path finder
 *
 *  @param  : const char            *ipaddr [Destination IP address]
 *            odr_object            *obj    [odr object]
 *  @return : odr_rtable *          [routing path entry]
 *
 *  Find the routing path entry of the destination IP address
 *  return NULL if destination is currently unreachable
 * --------------------------------------------------------------------------
 */
odr_rtable *get_item_rtable(const char *ipaddr, odr_object *obj) {
    odr_rtable *item = obj->rtable;

    // find the item in rtable
    while (item) {
        if (strcmp(item->dst, ipaddr) == 0)
            break;
        item = item->next;
    }

    return item;
}

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
 *  purge_tables
 *
 *  ODR service tables purge function
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  Purge the entries in rtable that have gone stale
 *  Purge the entries in ptable that have no communication longer than
 *  ODR_TIMETOLIVE
 * --------------------------------------------------------------------------
 */
void purge_tables(odr_object *obj) {
    odr_rtable *rtable, *rp;
    odr_ptable *ptable, *pp;
    ulong t = time(NULL);

    // remove the route that has been stale
    rp = obj->rtable;
    rtable = obj->rtable;
    while (rtable) {
        if (rtable->timestamp + obj->staleness > t) {
            // remove the routing path
            if (rtable == obj->rtable) {
                // remove head
                obj->rtable = rtable->next;
                free(rtable);
                rtable = obj->rtable;
                rp = obj->rtable;
            } else {
                // remove not head
                rp->next = rtable->next;
                free(rtable);
                rtable = rp->next;
            }
        } else {
            rp = rtable;
            rtable = rtable->next;
        }
    }

    // remove the path-port that has been timeout
    pp = obj->ptable;
    ptable = obj->ptable;
    while (ptable) {
        if (ptable->timestamp != 0 && ptable->timestamp + ODR_TIMETOLIVE > t) {
            // remove not head
            pp->next = ptable->next;
            free(ptable);
            ptable = pp->next;
        } else {
            pp = ptable;
            ptable = ptable->next;
        }
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
    item->apacket.hopcnt = 0;
    strcpy(item->apacket.data, dgram.data);

    item->flag = dgram.flag;
    item->next = NULL;

    // insert into queue
    if (obj->queue.head == NULL) {
        obj->queue.head = item;
        obj->queue.tail = item;
    } else {
        obj->queue.tail->next = item;
        obj->queue.tail = item;
    }
    printf("Queued up app_packet [DST: %s:%d SRC: %s:%d HOPCNT: %d DATA(%d): %s]\n", item->apacket.dst, item->apacket.dst_port, item->apacket.src, item->apacket.src_port, item->apacket.hopcnt, item->apacket.length, item->apacket.data);

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

        purge_tables(obj);

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
