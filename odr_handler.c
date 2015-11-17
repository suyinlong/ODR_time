/*
* @File: odr_handler.c
* @Date: 2015-11-14 19:51:16
* @Last Modified by:   Yinlong Su
* @Last Modified time: 2015-11-14 20:33:11
* @Description:
*     ODR frame and queued packet handler

*/

#include "np.h"
#include "odr_rrep.h"

void queue_handler(odr_object *obj) {

}

void frame_rreq_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {

}

void frame_rrep_handler(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from) {
    HandleRREP(obj, frame, from);
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
