/**
* @file         :  odr_rrep.c
* @author       :  Jiewen Zheng
* @date         :  2015-11-15
* @brief        :  RREP implementation
* @changelog    :
**/

#include <stdbool.h>
#include "odr_rrep.h"

void InsertOrUpdateRoutingTable(odr_object *obj, odr_rtable *item, char *dst, char *nexthop, int index, uint hopcnt) {
    int i;
    if (item == NULL)
    {
        // insert a new route
        item = (odr_rtable *)Calloc(1, sizeof(odr_rtable));
        item->next = obj->rtable;
        obj->rtable = item;
    }

    // modify the route
    memcpy(item->dst, dst, IPADDR_BUFFSIZE);
    memcpy(item->nexthop, nexthop, HWADDR_BUFFSIZE);
    item->index = index;
    item->hopcnt = hopcnt;
    item->timestamp = time(NULL);

    printf("[Route Table] dst: %s, nexthop: ", item->dst);
    for (i = 0; i < 6; i++)
        printf("%.2x%s", item->nexthop[i] & 0xff, (i == 5 ? ", ": ":"));
    printf("index: %d, hopcnt: %d\n", item->index, item->hopcnt);
}


int HandleRREP(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from)
{
    int idx = 0, i;
    bool needReply = false;

    odr_rpacket *rrep = (odr_rpacket *)frame->data;
    printf("[rrep_handler] Received RREP (dst: %s src: %s hopcnt: %d)\n", rrep->dst, rrep->src, rrep->hopcnt);
    printf("               from interface %d mac: ", from->sll_ifindex);
    for (i = 0; i < 6; i++)
        printf("%.2x%s", frame->h_source[i] & 0xff, (i < 5) ? ":" : "\n");
    // get the 'forward' route from routing table
    odr_rtable *dst_ritem = get_item_rtable(rrep->dst, obj);

    if (dst_ritem == NULL || dst_ritem->hopcnt > rrep->hopcnt + 1) {
        InsertOrUpdateRoutingTable(obj, dst_ritem, rrep->dst, from->sll_addr, from->sll_ifindex, rrep->hopcnt + 1);
        if (strcmp(obj->ipaddr, rrep->src) != 0)
            needReply = true;
        else
            printf("[rrep_handler] RREP reached source node (%s)\n", rrep->src);
    }

    if (needReply)
    {
        // build apacket item
        odr_queue_item  *item = (odr_queue_item *)Calloc(1, sizeof(odr_queue_item));
        rrep->hopcnt ++;

        memcpy(item->data, rrep, ODR_FRAME_PAYLOAD);

        item->type = ODR_FRAME_RREP;
        item->timestamp = time(NULL);
        item->next = NULL;

        // insert into queue
        if (obj->queue.head == NULL) {
            obj->queue.head = item;
            obj->queue.tail = item;
        } else {
            obj->queue.tail->next = item;
            obj->queue.tail = item;
        }
        printf("Queued up rrep_packet [DST: %s SRC: %s HOPCNT: %d FRD:%d]\n", rrep->dst, rrep->src, rrep->hopcnt, rrep->flag.frd);

    }

    queue_handler(obj);

    return 0;
}

