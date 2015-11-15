#ifndef __np_h
#define __np_h

#include <stdio.h>
#include <errno.h>      /* error numbers */
#include <sys/ioctl.h>  /* ioctls */
#include <sys/file.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include "unp.h"

#define PROTOCOL_ID         61375
#define ODR_FRAME_LEN       128
#define ODR_PATH            "/tmp/14508-61375-timeODR"

#define TIMESERV_PATH       "/tmp/14508-61375-timeServer"
#define TIMESERV_PORT       14508

#define TIMECLIE_PATH       "/tmp/14508-61375-timeClient-XXXXXX"

#define IPADDR_BUFFSIZE     20
#define HWADDR_BUFFSIZE     6
#define HOSTNAME_BUFFSIZE   10
#define PATHNAME_BUFFSIZE   108

#define ODR_FRAME_PAYLOAD   (124 - 2 * sizeof(uchar) * ETH_ALEN - 2 * sizeof(ushort))
#define ODR_RPACKET_PAYLOAD (ODR_FRAME_PAYLOAD - 2 * sizeof(char) * IPADDR_BUFFSIZE - sizeof(odr_rpacket_flag) - 2 * sizeof(uint))
#define ODR_APACKET_PAYLOAD (ODR_FRAME_PAYLOAD - 2 * sizeof(char) * IPADDR_BUFFSIZE - 4 * sizeof(int))

#define ODR_FRAME_RREQ      0
#define ODR_FRAME_RREP      1
#define ODR_FRAME_APPMSG    2
#define ODR_FRAME_ROUTE     3
#define ODR_FRAME_DATA      9

#define ODR_DGRAM_DATALEN   ODR_APACKET_PAYLOAD

#define IF_NAME             16
#define IF_HADDR            6
#define IP_ALIAS            1

typedef unsigned char   BITFIELD8;
typedef unsigned char   uchar;
typedef unsigned short  ushort;
typedef unsigned int    uint;
typedef unsigned long   ulong;

// Interface table entry
// Modified hardware address information
//   * Ignore interfaces: lo, eth0
typedef struct hwa_info {
    char    if_name[IF_NAME];       /* interface name, null terminated      */
    char    if_haddr[IF_HADDR];     /* hardware address                     */
    int     if_index;               /* interface index                      */
    short   ip_alias;               /* 1 if hwa_addr is an alias IP address */
    struct  sockaddr  *ip_addr;     /* IP address                           */
    struct  hwa_info  *hwa_next;    /* next of these structures             */
} odr_itable;

// Route table entry
typedef struct odr_rtable_t {
    char    dst[IPADDR_BUFFSIZE];       /* destination IP addr  */
    char    nexthop[HWADDR_BUFFSIZE];   /* next hop MAC address */
    int     index;                      /* interface index      */
    uint    hopcnt;                     /* hop count            */
    uint    bcast_id;                   /* max broadcast id     */
    long    timestamp;                  /* timestamp of update  */
    struct odr_rtable_t *next;          /* next entry pointer   */
} odr_rtable;

// Port table entry
typedef struct odr_ptable_t {
    int     port;                       /* port number  */
    char    path[PATHNAME_BUFFSIZE];    /* path name    */
    ulong   timestamp;                  /* timestamp    */
    struct odr_ptable_t *next;          /* next item    */
} odr_ptable;

// frame structure
// odr_frame has 124 bytes, plus 4 Frame Check Sequence (CRC) equals 128 bytes
typedef struct odr_frame_t {
    uchar   h_dest[ETH_ALEN];           /* destination eth addr */
    uchar   h_source[ETH_ALEN];         /* source ether addr    */
    ushort  h_proto;                    /* packet type ID field */
    ushort  h_type;                     /* frame type           */
    char    data[ODR_FRAME_PAYLOAD];    /* frame payload        */
}__attribute__((packed)) odr_frame;

// route packet flag structure
typedef struct odr_rpacket_flag_t {
    BITFIELD8   req : 1; /* RREQ flag */
    BITFIELD8   rep : 1; /* RREP flag */
    BITFIELD8   frd : 1; /* forced (re)discovery flag */
    BITFIELD8   res : 1; /* reply already sent flag */
    BITFIELD8   r04 : 1;
    BITFIELD8   r05 : 1;
    BITFIELD8   r06 : 1;
    BITFIELD8   r07 : 1;
} odr_rpacket_flag;

// route packet structure
// length: ODR_FRAME_PAYLOAD
typedef struct odr_rpacket_t {
    char                dst[IPADDR_BUFFSIZE];   /* destination ip addr  */
    char                src[IPADDR_BUFFSIZE];   /* source ip addr       */
    odr_rpacket_flag    flag;                   /* route packet flag    */
    uint                hopcnt;                 /* hop count            */
    uint                bcast_id;               /* broadcast id         */
    char                unused[ODR_RPACKET_PAYLOAD];
} odr_rpacket;

// application packet structure
// length: ODR_FRAME_PAYLOAD
typedef struct odr_apacket_t {
    char    dst[IPADDR_BUFFSIZE];       /* destination IP address   */
    int     dst_port;                   /* destination port number  */
    char    src[IPADDR_BUFFSIZE];       /* source IP address        */
    int     src_port;                   /* source port number       */
    int     length;                     /* data length              */
    uint    hopcnt;                     /* hop count                */
    char    data[ODR_APACKET_PAYLOAD];  /* data payload (app)       */
} odr_apacket;

// datagram structure
// exchange between ODR service and application
typedef struct odr_dgram_t {
    char    ipaddr[IPADDR_BUFFSIZE];    /* IP address                   */
    int     port;                       /* port number                  */
    int     flag;                       /* forced discovery flag        */
    char    data[ODR_DGRAM_DATALEN];    /* data field in odr_apacket    */
} odr_dgram;

// odr apacket queue (waiting to send)
typedef struct odr_queue_item_t {
    odr_apacket apacket;    /* apacket content          */
    int         flag;       /* forced discovery flag    */
    struct odr_queue_item_t *next;
} odr_queue_item;
typedef struct odr_queue_t {
    odr_queue_item *head;
    odr_queue_item *tail;
} odr_queue;

// Main ODR information object
typedef struct odr_object_t {
    unsigned long   staleness;                      /* in seconds           */
    char            ipaddr[IPADDR_BUFFSIZE];        /* IP address           */
    char            hostname[HOSTNAME_BUFFSIZE];    /* Host name            */
    odr_itable      *itable;                        /* Hardware information */
    odr_rtable      *rtable;                        /* routing table        */
    odr_ptable      *ptable;                        /* port and path table  */
    odr_queue       queue;                          /* ODR message queue    */
    int             d_sockfd;                       /* Domain socket        */
    int             p_sockfd;                       /* PF_PACKET socket     */
    uint            bcast_id;                       /* Broadcast ID         */
    int             free_port;                      /* free port number     */
} odr_object;

odr_itable *get_hw_addrs(char *);
odr_itable *Get_hw_addrs(char *);
void free_hwa_info(odr_itable *);

odr_itable *get_item_itable(int, odr_object *);
odr_rtable *get_item_rtable(const char *, odr_object *);

#endif
