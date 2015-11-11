#ifndef __np_h
#define __np_h

#include <stdio.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include "unp.h"
#include "hw_addrs.h"

#define PROTOCOL_ID         61375

#define IPADDR_BUFFSIZE     20
#define HWADDR_BUFFSIZE     6
#define HOSTNAME_BUFFSIZE   10

#define RTABLE_SIZE         20


typedef struct odr_rtable_t {
    char    dst[IPADDR_BUFFSIZE];    //
    char    nexthop[HWADDR_BUFFSIZE];
    uint8_t index;
    uint8_t hops;
    long    timestamp;
} odr_rtable;

typedef struct odr_msg_t {
    uint8_t type;
} odr_msg;

typedef struct odr_queue_t {
    odr_msg             msg;
    struct odr_queue_t  *next;
} odr_queue;

typedef struct odr_object_t {
    unsigned long   staleness;                      /* in seconds */
    struct hwa_info *hwa;                           /* Hardware information*/
    char            ipaddr[IPADDR_BUFFSIZE];        /* IP address */
    char            hostname[HOSTNAME_BUFFSIZE];    /* Host name */
    odr_rtable      rtable[RTABLE_SIZE];            /* routing table */
    odr_queue       queue;                          /* ODR message queue */
} odr_object;

void util_ip_to_hostname(const char *, char *);

#endif
