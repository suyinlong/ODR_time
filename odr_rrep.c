/**
* @file         :  odr_rrep.c
* @author       :  Jiewen Zheng
* @date         :  2015-11-15
* @brief        :  RREP implementation
* @changelog    :
**/

#include <stdbool.h>
#include "odr_rrep.h"

int VerifyITable(const odr_itable *table, const char *addr)
{
    const odr_itable *it = table;
    for (; it != NULL; it = it->hwa_next)
    {
        if (strstr(it->if_haddr, addr))
            return 0;
    }
    return -1;
}

/*odr_rtable *GetRouteItem(const odr_rtable *table, const char *dstIpAddr)
{
    odr_rtable *it = (odr_rtable *)table;
    for (; it != NULL; it = it->next)
    {
        if (strstr(it->dst, dstIpAddr))
            return it;
    }
    return NULL;
}

int GetRouteIndex(const odr_rtable *table, const char *srcIpAddr)
{
    const odr_rtable *it = GetRouteItem(table, srcIpAddr);
    if (it == NULL)
        return -1;
    return it->index;
}

int InsertRoutingTable(odr_rtable *table, const odr_frame *frame, const char *fromAddr)
{
    int idx = 0;
    odr_rpacket *rpkt = (odr_rpacket *)frame->data;
    // get the interface index from 'reverse' routing table
    idx = GetRouteIndex(table, rpkt->src);
    if (idx == -1)
    {
        printf("[ODR-RREP]: Get interface index error, src=%s\n", rpkt->src);
        return -1;
    }

    // create odr routing table
    odr_rtable *route = malloc(sizeof(odr_rtable));
    memset(route, 0, sizeof(odr_rtable));
    strcpy(route->dst, rpkt->src);
    strcpy(route->nexthop, fromAddr);
    route->index = idx;
    route->hopcnt = rpkt->hopcnt;
    route->bcast_id = rpkt->bcast_id;
    route->timestamp = time(NULL);

    if (table)
        table->next = route;
    else
        table = route;

    return idx;
}

void UpdateRoutingTable(odr_rtable *item, const odr_frame *frame, const char *fromAddr)
{
    odr_rpacket *rpkt = (odr_rpacket *)frame->data;
    strcpy(item->nexthop, fromAddr);
    item->hopcnt = rpkt->hopcnt;
    item->timestamp = time(NULL);
}
*/

void InsertOrUpdateRoutingTable(odr_object *obj, odr_rtable *item, char *dst, char *nexthop, int index, uint hopcnt, uint bcast_id) {
    int i;
    if (item == NULL) {
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
    item->bcast_id = bcast_id;
    item->timestamp = time(NULL);

    printf("[InsertOrUpdateRoutingTable] dst: %s, nexthop: ", item->dst);
    for (i = 0; i < 6; i++)
        printf("%.2x%s", item->nexthop[i] & 0xff, (i == 5 ? ", ": ":"));
    printf("index: %d, hopcnt: %d, bcast_id: %d, timestamp: %ld\n", item->index, item->hopcnt, item->bcast_id, item->timestamp);
}


int HandleRREP(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from)
{
    int idx = 0;
    bool needReply = false;
     // verify whether a node is source from hardware interface table
    // VerifyITable(obj->itable, frame->h_dest);

    // get sender hardware address
    /*char fromHwAddr[HWADDR_BUFFSIZE];
    memcpy(&fromHwAddr, from->sll_addr, sizeof(fromHwAddr));*/

    odr_rpacket *rrep = (odr_rpacket *)frame->data;
    printf("[frame_rrep_handler] Received RREP (dst:%s src:%s bcast_id:%d hop:%d)\n", rrep->dst, rrep->src, rrep->bcast_id, rrep->hopcnt);
    
    // get the 'forward' route from routing table
    odr_rtable *dst_ritem = get_item_rtable(rrep->dst, obj);

    if (dst_ritem == NULL || dst_ritem->hopcnt > rrep->hopcnt) {
        InsertOrUpdateRoutingTable(obj, dst_ritem, rrep->dst, from->sll_addr, from->sll_ifindex, rrep->hopcnt + 1, 0);
        if (strcmp(obj->ipaddr, rrep->src) != 0)
            needReply = true;
        else
            printf("[frame_rrep_handler] RREP reached the source node(%s)\n", rrep->src);
    }

    /*
    if (forwardRoute == NULL)
    {
        // insert 'forward' route into routing table
        idx = InsertRoutingTable(obj->rtable, frame, fromHwAddr);
        if (idx < 0)
        {
            printf("[ODR-RREP]: Get interface index error\n");
            return -1;
        }
        needReply = true;

        printf("[ODR-RREP]: Insert a 'forward' route [dst=%s next=%s hop=%d]\n", 
            rpkt->src, fromHwAddr, rpkt->hopcnt);
    }
    else
    {
        if (forwardRoute->hopcnt > rpkt->hopcnt)
        {
            // update 'forward' routing table with the shortest-hop
            UpdateRoutingTable(forwardRoute, frame, fromHwAddr);
            idx = forwardRoute->index;
            needReply = true;

            printf("[ODR-RREP]: Update a 'forward' route [dst=%s next=%s hop=%d]\n",
                rpkt->src, fromHwAddr, rpkt->hopcnt);
        }
    }*/

    if (needReply)
    {
        /*// get the 'reverse' route from routing table
        odr_rtable *reverseRoute = GetRouteItem(obj->rtable, rpkt->dst);
        if (reverseRoute == NULL)
        {
            printf("[ODR-RREP]: Get reverse route, dst=%s\n", rpkt->dst);
            return -1;
        }

        odr_frame rrep;
        memcpy(&rrep, frame, sizeof(rrep));
        ((odr_rpacket *)rrep.data)->hopcnt++;
        strcpy(rrep.h_dest, reverseRoute->nexthop);

        // send RREP to next node
        if (send_frame(obj->p_sockfd, idx, &rrep, PACKET_OTHERHOST) < 0)
        {
            printf("[ODR-RREP]: Send RREP error, %s\n", strerror(errno));
            return -1;
        }

        printf("[ODR-RREP]: Propagate the RREP <src=%s dstIp=%s hop=%d>\n",
            rrep.h_source, rpkt->dst, rpkt->hopcnt);*/
        
        // build apacket item
        odr_queue_item  *item = (odr_queue_item *)Calloc(1, sizeof(odr_queue_item));
        rrep->hopcnt ++;

        memcpy(item->data, rrep, ODR_FRAME_PAYLOAD);

        item->type = ODR_FRAME_RREP;
        item->next = NULL;

        // insert into queue
        if (obj->queue.head == NULL) {
            obj->queue.head = item;
            obj->queue.tail = item;
        } else {
            obj->queue.tail->next = item;
            obj->queue.tail = item;
        }
        
        printf("Queued up rrep_packet [DST: %s SRC: %s HOPCNT: %d FRD: %d]\n", rrep->dst, rrep->src, rrep->hopcnt, rrep->flag.frd);
        queue_handler(obj);
    }

    return 0;
}

int HandleAppMsg(odr_object *obj, odr_frame *frame, struct sockaddr_ll *from)
{
    odr_rpacket *rpkt = (odr_rpacket *)frame->data;

    // get the 'forward' route from routing table
    odr_rtable *route = get_item_rtable(rpkt->dst, obj);
    if (route == NULL)
    {
        printf("[ODR-APP]: No route [%s --> %s]\n", rpkt->src, rpkt->dst);
        return -1;
    }

    // get next node address
    strcpy(frame->h_dest, route->nexthop);
    
    odr_queue_item  *item = (odr_queue_item *)Calloc(1, sizeof(odr_queue_item));
    rpkt->hopcnt++;

    memcpy(item->data, rpkt, ODR_FRAME_PAYLOAD);

    item->type = ODR_FRAME_APPMSG;
    item->next = NULL;

    // insert into queue
    if (obj->queue.head == NULL) {
        obj->queue.head = item;
        obj->queue.tail = item;
    }
    else {
        obj->queue.tail->next = item;
        obj->queue.tail = item;
    }

    queue_handler(obj);

    printf("Queued up appmsg_packet [DST: %s SRC: %s HOPCNT: %d FRD:%d]\n", rpkt->dst, rpkt->src, rpkt->hopcnt, rpkt->flag.frd);


    return 0;
}

