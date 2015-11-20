/*
* @File: odr_handler.c
* @Date: 2015-11-14 19:51:16
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-19 19:44:32
* @Description:
*     ODR frame and queued packet handler
*     - void send_rreq(odr_object *obj, char *dst, char *src, uint hopcnt, uint bcast_id, int frdflag, int resflag)
*         [RREQ send function]
*     - void send_rrep(odr_object *obj, char *dst, char *src, uint hopcnt, int frdflag)
*         [RREP send function]
*     - int cmp_hwaddrs(char *addr1, char *addr2)
*         [MAC address compare function]
*     - void send_dgram(odr_object *obj, odr_apacket *appmsg)
*         [Dgram APPMSG send function]
*     + void queue_handler(odr_object *obj)
*         [Queue handler]
*     + void frame_rreq_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from)
*         [Frame RREQ handler]
*     + void frame_rrep_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from)
*         [Frame RREP handler]
*     + void frame_appmsg_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from)
*         [Frame APPMSG handler]
*/

#include "np.h"
#include "odr_rrep.h"

/* --------------------------------------------------------------------------
 *  send_rreq
 *
 *  RREQ send function
 *
 *  @param  : odr_object    *obj        [odr object]
 *            char          *dst        [Destionation IP address]
 *            char          *src        [Source IP address]
 *            uint          hopcnt      [Hop count]
 *            uint          bcast_id    [Broadcast ID]
 *            int           frdflag     [Forced discovery flag]
 *            int           resflag     [Replay already sent flag]
 *  @return : void
 *
 *  Send RREQ via all interfaces
 * --------------------------------------------------------------------------
 */
void send_rreq(odr_object *obj, char *dst, char *src, uint hopcnt, uint bcast_id, int frdflag, int resflag) {
    odr_frame   frame;
    odr_rpacket rreq;
    odr_itable  *itable;
    bzero(&rreq, sizeof(rreq));

    // fill the RREQ information
    strcpy(rreq.dst, dst);
    strcpy(rreq.src, src);
    rreq.flag.req = 1;
    rreq.flag.rep = 0;
    rreq.flag.frd = frdflag;
    rreq.flag.res = resflag;

    rreq.hopcnt = hopcnt;
    rreq.bcast_id = bcast_id;

    printf("[send_rreq] RREQ (dst: %s src: %s frd: %d res: %d hopcnt: %d bcast_id: %d)\n", rreq.dst, rreq.src, rreq.flag.frd, rreq.flag.res, rreq.hopcnt, rreq.bcast_id);
    printf("            broadcast via interface: ");
    // send the frame via all interfaces
    for (itable = obj->itable; itable != NULL; itable = itable->hwa_next) {
        bzero(&frame, sizeof(frame));
        build_bcast_frame(&frame, itable->if_haddr, ODR_FRAME_RREQ, &rreq);
        send_frame(obj->p_sockfd, itable->if_index, &frame, PACKET_BROADCAST);
        printf("%d ", itable->if_index);
    }
    printf("\n");
}

/* --------------------------------------------------------------------------
 *  send_rrep
 *
 *  RREP send function
 *
 *  @param  : odr_object    *obj        [odr object]
 *            char          *dst        [Destionation IP address]
 *            char          *src        [Source IP address]
 *            uint          hopcnt      [Hop count]
 *            int           frdflag     [Forced discovery flag]
 *  @return : void
 *
 *  Send RREP via route interface
 * --------------------------------------------------------------------------
 */
void send_rrep(odr_object *obj, char *dst, char *src, uint hopcnt, int frdflag) {
    int i;
    odr_frame   frame;
    odr_rpacket rrep;
    odr_itable  *itable;
    odr_rtable  *rtable;
    bzero(&rrep, sizeof(rrep));

    // fill the RREP information
    strcpy(rrep.dst, dst);
    strcpy(rrep.src, src);
    rrep.flag.req = 0;
    rrep.flag.rep = 1;
    rrep.flag.frd = frdflag;
    rrep.flag.res = 1;

    rrep.hopcnt = hopcnt;
    rrep.bcast_id = 0;

    rtable = get_item_rtable(src, obj);
    itable = get_item_itable(rtable->index, obj);

    printf("[send_rrep] RREP (dst: %s src: %s frd: %d res: %d hopcnt: %d)\n", rrep.dst, rrep.src, rrep.flag.frd, rrep.flag.res, rrep.hopcnt);
    // send the frame via the interface
    bzero(&frame, sizeof(frame));
    build_frame(&frame, rtable->nexthop, itable->if_haddr, ODR_FRAME_RREP, &rrep);
    send_frame(obj->p_sockfd, rtable->index, &frame, PACKET_OTHERHOST);
    printf("            unicast via interface %d to ", rtable->index);
    for (i = 0; i < 6; i++)
        printf("%.2x%s", rtable->nexthop[i] & 0xff, (i < 5) ? ":" : "\n");

}

/* --------------------------------------------------------------------------
 *  cmp_hwaddrs
 *
 *  MAC address compare function
 *
 *  @param  : char          *addr1      [MAC address 1]
 *            char          *addr2      [MAC address 2]
 *  @return : int           [0 if not equal, 1 if equal]
 *
 *  MAC address compare function
 * --------------------------------------------------------------------------
 */
int cmp_hwaddrs(char *addr1, char *addr2) {
    int i;
    for (i = 0; i < 6; i++)
        if ((addr1[i] & 0xff) != (addr2[i] & 0xff))
            return 0;
    return 1;
}

/* --------------------------------------------------------------------------
 *  send_dgram
 *
 *  Dgram APPMSG send function
 *
 *  @param  : odr_object    *obj        [odr object]
 *            odr_apacket   *appmsg     [APPMSG]
 *  @return : void
 *
 *  Send APPMSG to local domain path
 * --------------------------------------------------------------------------
 */
void send_dgram(odr_object *obj, odr_apacket *appmsg) {
    // APPMSG reach destination, send to domain socket
    int port = appmsg->dst_port;
    struct sockaddr_un addr;
    odr_dgram   dgram;
    odr_ptable  *pitem = get_item_ptable(port, obj);

    if (pitem == NULL) {
        printf("[send_dgram] Error: Port number not available.\n");
        return;
    }
    bzero(&addr, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strcpy(addr.sun_path, pitem->path);

    bzero(&dgram, sizeof(dgram));
    strcpy(dgram.ipaddr, appmsg->src);
    dgram.port = appmsg->src_port;
    dgram.flag = appmsg->frd;
    strcpy(dgram.data, appmsg->data);

    sendto(obj->d_sockfd, &dgram, sizeof(dgram), 0, (SA *)&addr, sizeof(addr));
}

/* --------------------------------------------------------------------------
 *  queue_handler
 *
 *  Queue handler
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  For the APPMSG/RREP in queue, find the destination routing path in rtable
 *  - If the destination is currently unreachable, send RREQ
 *  - If the forced discovery flag is set, send RREQ with flag.frd
 *  - Otherwise, send the frame via routing interface
 * --------------------------------------------------------------------------
 */
void queue_handler(odr_object *obj) {
    int i, freeflag = 0;
    odr_rtable *route;
    odr_itable *interface;
    odr_rpacket *rpacket;
    odr_apacket *apacket;
    odr_frame frame;

    // return if the queue is empty
    if (obj->queue.head == NULL)
        return;

    if (obj->queue.head->timestamp + QUEUE_TIMEOUT <= time(NULL)) {
        // queue timeout, fail and remove
        freeflag = 1;
    } else if (obj->queue.head->type == ODR_FRAME_APPMSG) {
        apacket = (odr_apacket *)obj->queue.head->data;
        printf("[queue_handler] Processing APPMSG (dst: %s:%d src: %s:%d hopcnt: %d frd: %d data[%d]: %s)\n", apacket->dst, apacket->dst_port, apacket->src, apacket->src_port, apacket->hopcnt, apacket->frd, apacket->length, apacket->data);
        route = get_item_rtable(apacket->dst, obj);

        if (strcmp(obj->ipaddr, apacket->dst) == 0) {
            // APPMSG reach destination
            printf("[queue_handler] APPMSG reach destination, send to domain socket.\n");
            send_dgram(obj, apacket);
            freeflag = 1;
        } else if (route == NULL || apacket->frd == 1) {
            // destination is currently unreachable, send RREQ
            // or forced discovery, send rreq with flag.frd = 1
            printf("[queue_handler] Destination is currently unreachable, send RREQ.\n");
            send_rreq(obj, apacket->dst, obj->ipaddr, 0, ++obj->bcast_id, apacket->frd, 0);
        } else {
            // found entry in rtable, send apacket via interface
            interface = get_item_itable(route->index, obj);

            printf("[queue_handler] Send APPMSG via interface %d to ", route->index);
            for (i = 0; i < 6; i++)
                printf("%.2x%s", route->nexthop[i] & 0xff, (i < 5) ? ":" : "\n");
            bzero(&frame, sizeof(frame));
            build_frame(&frame, route->nexthop, interface->if_haddr, ODR_FRAME_APPMSG, apacket);
            send_frame(obj->p_sockfd, route->index, &frame, PACKET_OTHERHOST);

            freeflag = 1;
        }
    } else if (obj->queue.head->type == ODR_FRAME_RREP) {
        rpacket = (odr_rpacket *)obj->queue.head->data;
        printf("[queue_handler] Processing RREP (dst: %s src: %s hopcnt: %d)\n", rpacket->dst, rpacket->src, rpacket->hopcnt);
        route = get_item_rtable(rpacket->src, obj);

        if (route == NULL) {
            printf("[queue_handler] Source is currently unreachable, send RREQ.\n");
            send_rreq(obj, rpacket->src, obj->ipaddr, 0, ++obj->bcast_id, 0, 0);
        } else {
            interface = get_item_itable(route->index, obj);

            printf("[queue_handler] Send RREP via interface %d to ", route->index);
            for (i = 0; i < 6; i++)
                printf("%.2x%s", route->nexthop[i] & 0xff, (i < 5) ? ":" : "\n");
            bzero(&frame, sizeof(frame));
            build_frame(&frame, route->nexthop, interface->if_haddr, ODR_FRAME_RREP, rpacket);
            send_frame(obj->p_sockfd, route->index, &frame, PACKET_OTHERHOST);

            freeflag = 1;
        }
    }

    if (freeflag) {
        // free the head of queue
        if (obj->queue.head == obj->queue.tail) {
            free(obj->queue.head);
            obj->queue.head = NULL;
            obj->queue.tail = NULL;
        } else {
            odr_queue_item *next = obj->queue.head->next;
            free(obj->queue.head);
            obj->queue.head = next;
        }

        // if there is at least one apacket still in queue, try to handle it
        if (obj->queue.head)
            queue_handler(obj);
    }

}

/* --------------------------------------------------------------------------
 *  frame_rreq_handler
 *
 *  Frame RREQ handler
 *
 *  @param  : odr_object            *obj    [odr object]
 *            odr_frame             *frame  [received frame]
 *            struct sockaddr_ll    *from   [socket sender address]
 *  @return : void
 *
 *  Handle received RREQ
 *
 * --------------------------------------------------------------------------
 */
void frame_rreq_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {
    uchar       resflag = 0;            // reply already sent flag
    uchar       newsflag = 0;           // new S flag
    uchar       newrreqflag = 0;        // new RREQ flag
    uchar       newhopflag = 0;         // new path having smaller hopcnt flag
    int         s, d, i;                // node number of src and dst
    odr_rpacket *rreq;
    odr_rtable  *src_ritem, *dst_ritem;

    // get rpacket in frame
    rreq = (odr_rpacket *)frame->data;
    printf("[rreq_handler] Received RREQ (dst: %s src: %s frd: %d res: %d hopcnt: %d bcast_id: %d)\n", rreq->dst, rreq->src, rreq->flag.frd, rreq->flag.res, rreq->hopcnt, rreq->bcast_id);
    printf("               from interface %d mac: ", from->sll_ifindex);
    for (i = 0; i < 6; i++)
        printf("%.2x%s", frame->h_source[i] & 0xff, (i < 5) ? ":" : "\n");

    if (strcmp(obj->ipaddr, rreq->src) == 0) {
        printf("[rreq_handler] RREQ was sent by local node, ignored.\n");
        return;
    }
    // find routing items in rtable
    src_ritem = get_item_rtable(rreq->src, obj);
    dst_ritem = get_item_rtable(rreq->dst, obj);
    // get the broadcast id for ss, ds
    s = util_ip_to_index(rreq->src);
    d = util_ip_to_index(rreq->dst);

    if (src_ritem == NULL)
        newsflag = 1;
    if (rreq->bcast_id > obj->b_ids[s][s]) {
        // insert or update a new routing path (reverse route back)
        InsertOrUpdateRoutingTable(obj, src_ritem, rreq->src, frame->h_source, from->sll_ifindex, rreq->hopcnt + 1);
        obj->b_ids[s][s] = rreq->bcast_id;
        newrreqflag = 1;
    } else if (rreq->flag.frd == 1                          // forced discovery = true
        || rreq->hopcnt + 1 < src_ritem->hopcnt             // shorter path
        || (rreq->hopcnt + 1 == src_ritem->hopcnt
            && strncmp(src_ritem->nexthop, frame->h_source, HWADDR_BUFFSIZE) != 0)) {   // same hopcnt but different path
        // update the routing path (reverse route back)
        if (rreq->hopcnt + 1 < src_ritem->hopcnt)
            newhopflag = 1;
        InsertOrUpdateRoutingTable(obj, src_ritem, rreq->src, frame->h_source, from->sll_ifindex, rreq->hopcnt + 1);
        obj->b_ids[s][s] = rreq->bcast_id;
    }

    // TODO: check
    if (strcmp(rreq->dst, obj->ipaddr) == 0 && rreq->bcast_id > obj->b_ids[d][s]) {
        // destination, send RREP back
        obj->b_ids[d][s] = rreq->bcast_id;
        printf("[rreq_handler] RREQ reached destination, send back RREP\n");
        send_rrep(obj, rreq->dst, rreq->src, 0, rreq->flag.frd);
        resflag = 1;
        return;
    } else {
        // intermediate node
        printf("rreq->bcast_id: %d obj->b_ids[d][s]: %d %s\n", rreq->bcast_id, obj->b_ids[d][s], (dst_ritem == NULL) ? "NULL" : "NOT NULL");
        if (rreq->bcast_id > obj->b_ids[d][s]) {
            obj->b_ids[d][s] = rreq->bcast_id;
            if (dst_ritem != NULL                                       // have routing path to destionation
                && rreq->flag.frd == 0                                  // forced discovery = false
                && rreq->flag.res == 0                                  // reply already sent = false
                && cmp_hwaddrs(dst_ritem->nexthop, frame->h_source) == 0) {  // split horizon
                // intermediate node, send RREP back
                printf("[rreq_handler] RREQ reached intermediate, send back RREP\n");
                send_rrep(obj, rreq->dst, rreq->src, dst_ritem->hopcnt, rreq->flag.frd);
                resflag = 1;
            }
        } else if (dst_ritem != NULL
            && newhopflag == 1) {
            printf("[rreq_handler] RREQ reached intermediate, send back RREP\n");
            send_rrep(obj, rreq->dst, rreq->src, dst_ritem->hopcnt, rreq->flag.frd);
            resflag = 1;
        }
    }

    if (dst_ritem == NULL && (newrreqflag == 1 || newhopflag == 1)) {
        // send out rreq
        printf("[rreq_handler] Broadcast RREQ\n");
        send_rreq(obj, rreq->dst, rreq->src, rreq->hopcnt + 1, rreq->bcast_id, rreq->flag.frd, resflag);
    }

    if (dst_ritem != NULL && (newsflag == 1 || newhopflag == 1)) {
        // send out rreq
        printf("[rreq_handler] Broadcast RREQ\n");
        send_rreq(obj, rreq->dst, rreq->src, rreq->hopcnt + 1, rreq->bcast_id, rreq->flag.frd, resflag);
    }
}

/* --------------------------------------------------------------------------
 *  frame_rrep_handler
 *
 *  Frame RREP handler
 *
 *  @param  : odr_object            *obj    [odr object]
 *            odr_frame             *frame  [received frame]
 *            struct sockaddr_ll    *from   [socket sender address]
 *  @return : void
 *
 *  Handle received RREP
 *
 * --------------------------------------------------------------------------
 */
void frame_rrep_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {
    HandleRREP(obj, frame, from);
}

/* --------------------------------------------------------------------------
 *  frame_appmsg_handler
 *
 *  Frame APPMSG handler
 *
 *  @param  : odr_object            *obj    [odr object]
 *            odr_frame             *frame  [received frame]
 *            struct sockaddr_ll    *from   [socket sender address]
 *  @return : void
 *
 *  Handle received APPMSG
 *  1. Insert or update route path if possible
 *  2. If APPMSG reaches destination, send to domain socket
 *  3. Otherwise, put APPMSG into queue
 * --------------------------------------------------------------------------
 */
void frame_appmsg_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {
    printf("[appmsg_handler] Received APPMSG\n");

    odr_apacket *appmsg = (odr_apacket *)frame->data;

    // insert or update route path
    odr_rtable *ritem = get_item_rtable(appmsg->src, obj);
    if (ritem == NULL || ritem->hopcnt > appmsg->hopcnt + 1) {
        printf("[appmsg_handler] APPMSG route path insert/update.\n");
        InsertOrUpdateRoutingTable(obj, ritem, appmsg->src, from->sll_addr, from->sll_ifindex, appmsg->hopcnt + 1);
    }

    if (strcmp(obj->ipaddr, appmsg->dst) == 0) {
        // APPMSG reach destination
        printf("[appmsg_handler] APPMSG reach destination, send to domain socket.\n");
        send_dgram(obj, appmsg);
    } else {
        // APPMSG queue up
        odr_queue_item  *item = (odr_queue_item *)Calloc(1, sizeof(odr_queue_item));
        appmsg->hopcnt ++;

        memcpy(item->data, appmsg, ODR_FRAME_PAYLOAD);

        item->type = ODR_FRAME_APPMSG;
        item->next = NULL;

        // insert into queue
        if (obj->queue.head == NULL) {
            obj->queue.head = item;
            obj->queue.tail = item;
        } else {
            obj->queue.tail->next = item;
            obj->queue.tail = item;
        }
        //printf("Queued up appmsg [DST: %s SRC: %s HOPCNT: %d FRD:%d]\n", rrep->dst, rrep->src, rrep->hopcnt, rrep->flag.frd);
        queue_handler(obj);
    }

}

/* --------------------------------------------------------------------------
 *  debug_route_handler
 *
 *  Debug function to print route table
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  Print all content in route table
 * --------------------------------------------------------------------------
 */
void debug_route_handler(odr_object *obj) {
    // print out all route items
    int i;
    odr_rtable *r = obj->rtable;

    printf("\n");
    printf("+----- IP address -----+---- Next hop -----+- I -+- H -+\n");
    while (r) {
        printf("| %-*s | ", IPADDR_BUFFSIZE, r->dst);
        for (i = 0; i < 6; i++)
            printf("%.2x%s", r->nexthop[i] & 0xff, (i < 5) ? ":" : " | ");
        printf("%3d | ", r->index);
        printf("%3d | ", r->hopcnt);
        //printf("%4d | ", r->bcast_id);
        printf("\n");
        r = r->next;
    }
    printf("+----------------------+-------------------+-----+-----+\n");
}

/* --------------------------------------------------------------------------
 *  debug_interface_handler
 *
 *  Debug function to print interface table
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  Print all content in interface table
 * --------------------------------------------------------------------------
 */
void debug_interface_handler(odr_object *obj) {
    // print all interface items
    int i;
    odr_itable *item = obj->itable;

    printf("\n");
    printf("+- Interface name -+--- MAC address ---+ I +\n");
    while (item) {
        printf("| %-*s | ", IF_NAME, item->if_name);
        for (i = 0; i < 6; i++)
            printf("%.2x%s", item->if_haddr[i] & 0xff, (i < 5) ? ":" : " | ");
        printf("%1d |\n", item->if_index);
        item = item->hwa_next;
    }
    printf("+------------------+-------------------+---+\n");
}


