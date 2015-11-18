/*
* @File: odr_handler.c
* @Date: 2015-11-14 19:51:16
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-17 18:08:51
* @Description:
*     ODR frame and queued packet handler
*     - void send_rreq(odr_object *obj, char *dst, char *src, uint hopcnt, uint bcast_id, int frdflag, int resflag)
*         [RREQ send function]
*     - void send_rrep
*         []
*     + void queue_handler(odr_object *obj)
*         [Queue handler]
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

    // send the frame via all interfaces
    for (itable = obj->itable; itable != NULL; itable = itable->hwa_next) {
        bzero(&frame, sizeof(frame));
        build_bcast_frame(&frame, itable->if_haddr, ODR_FRAME_RREQ, &rreq);
        send_frame(obj->p_sockfd, itable->if_index, &frame, PACKET_BROADCAST);
    }
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
 *  Send RREP via routing interface
 * --------------------------------------------------------------------------
 */
void send_rrep(odr_object *obj, char *dst, char *src, uint hopcnt, int frdflag) {
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
    // send the frame via the interface
    bzero(&frame, sizeof(frame));
    build_bcast_frame(&frame, itable->if_haddr, ODR_FRAME_RREP, &rrep);
    send_frame(obj->p_sockfd, rtable->index, &frame, PACKET_OTHERHOST);

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
    int freeflag = 0;
    odr_rtable *route;
    odr_itable *interface;
    odr_rpacket *rpacket;
    odr_apacket *apacket;
    odr_frame frame;

    // return if the queue is empty
    if (obj->queue.head == NULL)
        return;

    if (obj->queue.head->type == ODR_FRAME_APPMSG) {
        apacket = (odr_apacket *)obj->queue.head->data;
        route = get_item_rtable(apacket->dst, obj);

        if (route == NULL || apacket->frd == 1) {
            // destination is currently unreachable, send RREQ
            // or forced discovery, send rreq with flag.frd = 1
            send_rreq(obj, apacket->dst, obj->ipaddr, 0, ++obj->bcast_id, apacket->frd, 0);
        } else {
            // found entry in rtable, send apacket via interface
            interface = get_item_itable(route->index, obj);

            bzero(&frame, sizeof(frame));
            build_frame(&frame, route->nexthop, interface->if_haddr, ODR_FRAME_APPMSG, apacket);
            send_frame(obj->p_sockfd, route->index, &frame, PACKET_OTHERHOST);

            freeflag = 1;
        }
    } else if (obj->queue.head->type == ODR_FRAME_RREP) {
        rpacket = (odr_rpacket *)obj->queue.head->data;
        route = get_item_rtable(rpacket->dst, obj); // TODO: Is it dst or src?

        if (route == NULL) {
            send_rreq(obj, rpacket->dst, obj->ipaddr, 0, ++obj->bcast_id, 0, 0);
        } else {
            interface = get_item_itable(route->index, obj);

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

void frame_rreq_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {
    uchar       resflag = 0;            // reply already sent flag
    int         bcast_rreq_flag = 0;    // continue broadcast RREQ flag
    odr_rpacket *rpacket;
    odr_rtable  *src_ritem, *dst_ritem;

    // get rpacket in frame
    rpacket = (odr_rpacket *)frame->data;
    // find routing items in rtable
    src_ritem = get_item_rtable(rpacket->src, obj);
    dst_ritem = get_item_rtable(rpacket->dst, obj);

    if (src_ritem == NULL) {
        // insert a new routing path (reverse route back)
        InsertOrUpdateRoutingTable(obj, src_ritem, rpacket->src, frame->h_source, from->sll_ifindex, rpacket->hopcnt + 1, rpacket->bcast_id);

        /*src_ritem = (odr_rtable *)Calloc(1, sizeof(odr_rtable));
        strcpy(src_ritem->dst, rpacket->src);
        strcpy(src_ritem->nexthop, frame->h_source);
        src_ritem->index = from->sll_ifindex;
        src_ritem->hopcnt = rpacket->hopcnt;
        src_ritem->bcast_id = rpacket->bcast_id;
        src_ritem->timestamp = time(NULL);
        src_ritem->next = obj->rtable;
        obj->rtable = src_ritem;*/

        bcast_rreq_flag = 1;
    }
    else if (rpacket->flag.frd == 1                                 // forced discovery = true
        || rpacket->hopcnt + 1 < src_ritem->hopcnt                  // shorter path
        || (rpacket->hopcnt + 1 == src_ritem->hopcnt
            && strcmp(src_ritem->nexthop, frame->h_source) != 0)) {  // same hopcnt but different path
        // update the routing path (reverse route back)
        InsertOrUpdateRoutingTable(obj, src_ritem, rpacket->src, frame->h_source, from->sll_ifindex, rpacket->hopcnt + 1, rpacket->bcast_id);

        /*strcpy(src_ritem->nexthop, frame->h_source);
        src_ritem->index = from->sll_ifindex;
        src_ritem->hopcnt = rpacket->hopcnt;
        src_ritem->bcast_id = rpacket->bcast_id;
        src_ritem->timestamp = time(NULL);*/

        bcast_rreq_flag = 1;
    }


    if (strcmp(rpacket->dst, obj->ipaddr) == 0) {
        // destination, send RREP back
        send_rrep(obj, rpacket->dst, rpacket->src, 0, rpacket->flag.frd);
        resflag = 1;
    }
    if (dst_ritem != NULL                                       // have routing path to destionation
        && rpacket->flag.frd == 0                           // forced discovery = false
        && rpacket->flag.res == 0                           // reply already sent = false
        && strcmp(dst_ritem->nexthop, frame->h_source) != 0) {  // split horizon
        // intermediate node, send RREP back
        send_rrep(obj, rpacket->dst, rpacket->src, dst_ritem->hopcnt, rpacket->flag.frd);
        resflag = 1;
    }
    if (bcast_rreq_flag) {
        // send rreq out
        send_rreq(obj, rpacket->dst, rpacket->src, rpacket->hopcnt + 1, rpacket->bcast_id, rpacket->flag.frd, resflag);
    }
}

void frame_rrep_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {
    HandleRREP(obj, frame, from);
}

void frame_appmsg_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {
    HandleAppMsg(obj, frame, from);
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
 *  debug_data_handler
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
void debug_data_handler(odr_frame *frame, struct sockaddr_ll *from, socklen_t fromlen) {
    int i;

    printf("Interface index: %d, MAC address: ", from->sll_ifindex);
    for (i = 0; i < 6; i++)
        printf("%.2x%s", from->sll_addr[i] & 0xff, ((i == 5) ? " " : ":"));
    printf("Data: %s\n", frame->data);
}


