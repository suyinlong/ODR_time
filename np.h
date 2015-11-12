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
#define ODR_PATH            "/tmp/timeODR"

#define TIMESERV_PATH       "/tmp/timeServer"
#define TIMESERV_PORT       14508

#define IPADDR_BUFFSIZE     20
#define HWADDR_BUFFSIZE     6
#define HOSTNAME_BUFFSIZE   10

#define RTABLE_SIZE         20

#define IF_NAME     16  /* same as IFNAMSIZ    in <net/if.h> */
#define IF_HADDR    6   /* same as IFHWADDRLEN in <net/if.h> */
#define IP_ALIAS    1   /* hwa_addr is an alias */

typedef unsigned char uchar;

typedef struct hwa_info {
    char    if_name[IF_NAME];       /* interface name, null terminated */
    char    if_haddr[IF_HADDR];     /* hardware address */
    int     if_index;               /* interface index */
    short   ip_alias;               /* 1 if hwa_addr is an alias IP address */
    struct  sockaddr  *ip_addr;     /* IP address */
    struct  hwa_info  *hwa_next;    /* next of these structures */
} odr_itable;

/* function prototypes */
odr_itable     *get_hw_addrs(char*);
odr_itable     *Get_hw_addrs(char*);
void free_hwa_info(odr_itable *);

typedef struct odr_rtable_t {
    char    dst[IPADDR_BUFFSIZE];    //
    char    nexthop[HWADDR_BUFFSIZE];
    int     index;
    int     hops;
    long    timestamp;
} odr_rtable;

// odr_frame has 60 bytes, plus 4 Frame Check Sequence (CRC) equals 64 bytes
typedef struct odr_frame_t {
    unsigned char   h_dest[ETH_ALEN];   /* destination eth addr */
    unsigned char   h_source[ETH_ALEN]; /* source ether addr    */
    unsigned short  h_proto;            /* packet type ID field */
    char            data[46];
}__attribute__((packed)) odr_frame;

typedef struct odr_msg_t {
    int     type;
} odr_msg;

typedef struct odr_queue_t {
    odr_msg             msg;
    struct odr_queue_t  *next;
} odr_queue;

typedef struct odr_object_t {
    unsigned long   staleness;                      /* in seconds */
    odr_itable      *itable;                        /* Hardware information*/
    char            ipaddr[IPADDR_BUFFSIZE];        /* IP address */
    char            hostname[HOSTNAME_BUFFSIZE];    /* Host name */
    odr_rtable      *rtable[RTABLE_SIZE];           /* routing table */
    odr_queue       *queue;                         /* ODR message queue */
    int             d_sockfd;                       /* Domain socket */
    int             p_sockfd;                       /* PF_PACKET socket */
} odr_object;

void util_ip_to_hostname(const char *, char *);

#endif
