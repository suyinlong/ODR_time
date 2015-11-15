/*
* @File: odr_handler.c
* @Date: 2015-11-14 19:51:16
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-14 23:46:03
* @Description:
*     ODR frame and queued packet handler
*     - void send_rreq(odr_object *obj, char *dst, int frdflag, int resflag)
*         [RREQ send function]
*     - void send_rrep
*         []
*     + void queue_handler(odr_object *obj)
*         [Queue handler]
*/

#include "np.h"

/* --------------------------------------------------------------------------
 *  send_rreq
 *
 *  RREQ send function
 *
 *  @param  : odr_object    *obj    [odr object]
 *            char          *dst    [Destionation IP address]
 *            int           frdflag [Forced discovery flag]
 *            int           resflag [Replay already sent flag]
 *  @return : void
 *
 *  Send RREQ via all interfaces
 * --------------------------------------------------------------------------
 */
void send_rreq(odr_object *obj, char *dst, int frdflag, int resflag) {
    odr_frame   frame;
    odr_rpacket rreq;
    odr_itable  *itable;
    bzero(&rreq, sizeof(rreq));

    // fill the RREQ information
    strcpy(rreq.dst, dst);
    strcpy(rreq.src, obj->ipaddr);
    rreq.flag.req = 1;
    rreq.flag.frd = frdflag;
    rreq.flag.res = resflag;

    rreq.hopcnt = 0;
    rreq.bcast_id = ++obj->bcast_id;

    // send the frame via all interfaces
    for (itable = obj->itable; itable != NULL; itable = itable->hwa_next) {
        bzero(&frame, sizeof(frame));
        build_bcast_frame(&frame, itable->if_haddr, ODR_FRAME_RREQ, &rreq);
        send_frame(obj->p_sockfd, itable->if_index, &frame, PACKET_BROADCAST);
    }
}


void send_rrep() {

}

/* --------------------------------------------------------------------------
 *  queue_handler
 *
 *  Queue handler
 *
 *  @param  : odr_object    *obj    [odr object]
 *  @return : void
 *
 *  For the apacket in queue, find the destination routing path in rtable
 *  - If the destination is currently unreachable, send RREQ
 *  - If the forced discovery flag is set, send RREQ with flag.frd
 *  - Otherwise, send the frame via routing interface
 * --------------------------------------------------------------------------
 */
void queue_handler(odr_object *obj) {
    odr_rtable *route;
    odr_itable *interface;
    odr_apacket *apacket;
    odr_frame frame;

    // return if the queue is empty
    if (obj->queue.head == NULL)
        return;

    apacket = &(obj->queue.head->apacket);
    route = get_item_rtable(apacket->dst, obj);

    if (route == NULL || obj->queue.head->flag == 1) {
        // destination is currently unreachable, send RREQ
        // or forced discovery, send rreq with flag.frd = 1
        send_rreq(obj, apacket->dst, obj->queue.head->flag, 0);
    } else {
        // found entry in rtable, send apacket via interface
        interface = get_item_itable(route->index, obj);

        bzero(&frame, sizeof(frame));
        build_frame(&frame, route->nexthop, interface->if_haddr, ODR_FRAME_APPMSG, apacket);
        send_frame(obj->p_sockfd, route->index, &frame, PACKET_OTHERHOST);

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

}

void frame_rrep_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {

}

void frame_appmsg_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {

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


