#ifndef _DHCPD_H_
#define _DHCPD_H_
#include "timer.h"
#include "common.h"

void dhcpd_appcall(void);

struct dhcpd_state
{
	int state;
};

#define DHCPD_LISTEN_PORT 67

#define SNAME	"DHCPD"

#ifndef LITTLE_ENDIAN 
#define LITTLE_ENDIAN 0x11223344
#endif

#ifndef BIG_ENDIAN
#define BIG_ENDIAN 0x44332211
#endif

#ifndef BYTE_ORDER 
#define BYTE_ORDER BIG_ENDIAN
#endif

/* DHCP protocol. See RFC 2131 */
#define DHCP_MAGIC              0x63825363
#define DHCP_OPTIONS_BUFSIZE    308
#define BOOTREQUEST             1
#define BOOTREPLY               2


/* Defaults you may want to tweak */
/* Default max_lease_sec */
#define DEFAULT_LEASE_TIME      (60*60*24 * 10)

/* DHCP option codes (partial list). See RFC 2132 and
 * http://www.iana.org/assignments/bootp-dhcp-parameters/
 * Commented out options are handled by common option machinery,
 * uncommented ones have spacial cases (grep for them to see).
 */
#define DHCP_PADDING            0x00
#define DHCP_SUBNET             0x01
#define DHCPS_HOSTNAME 33
#define DHCP_VENDOR             0x3c /* client's vendor (a string) */

#define DHCP_END                0xff

#define DHCP_HOST_NAME          0x0c /* either client informs server or server gives name to client */
#define DHCP_REQUESTED_IP       0x32 /* sent by client if specific IP is wanted */
#define DHCP_LEASE_TIME         0x33
#define DHCP_OPTION_OVERLOAD    0x34
#define DHCP_MESSAGE_TYPE       0x35
#define DHCP_SERVER_ID          0x36 /* by default server's IP */

/* Offsets in option byte sequence */
#define OPT_CODE                0
#define OPT_LEN                 1
#define OPT_DATA                2

/* Bits in "overload" option */
#define OPTION_FIELD            0
#define FILE_FIELD              1
#define SNAME_FIELD             2

/* DHCP_MESSAGE_TYPE values */
#define DHCPDISCOVER            	1 /* client -> server */
#define DHCPOFFER               	2 /* client <- server */
#define DHCPREQUEST             	3 /* client -> server */
#define DHCPDECLINE             	4 /* client -> server */
#define DHCPACK                 		5 /* client <- server */
#define DHCPNAK                 	6 /* client <- server */
#define DHCPRELEASE             	7 /* client -> server */
#define DHCPINFORM              	8 /* client -> server */
#define DHCP_MINTYPE 		DHCPDISCOVER
#define DHCP_MAXTYPE 		DHCPINFORM


#define SERVER_PORT 67
#define CLIENT_PORT 68

#define MSG_LOG printf
#define MSG_ERR printf

#ifndef UIP_USE_NATIVE_DATA_TYPE
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
#endif

typedef uint32_t leasetime_t;
typedef int signed_leasetime_t;


#define move_from_unaligned32(v, u32p)  (memcpy(&(v), (u32p), 4), v = ntohl(v)) 

# define move_to_unaligned32(u32p, v) do { \
	uint32_t __t = htonl(v); \
	memcpy((u32p), &__t, 4); \
} while (0)

enum {
	OPTION_IP = 1,
	OPTION_IP_PAIR,
	OPTION_STRING,
	/* Opts of STRING_HOST type will be sanitized before they are passed
	 * to udhcpc script's environment: */
	OPTION_STRING_HOST,
	OPTION_U8,
	OPTION_U16,
	OPTION_U32,
	OPTION_S32,
	OPTION_BIN,
	OPTION_STATIC_ROUTES,
	OPTION_6RD,
	OPTION_TYPE_MASK = 0x0f,
	/* Client requests this option by default */
	OPTION_REQ  = 0x10,
	/* There can be a list of 1 or more of these */
	OPTION_LIST = 0x20,
};

struct dhcp_optflag {
	uint8_t flags;
	uint8_t code;
};

struct option_set {
	uint8_t *data;
	struct option_set *next;
};

struct static_lease {
	struct static_lease *next;
	uint32_t nip;
	uint8_t mac[6];
};

//TODO: rename ciaddr/yiaddr/chaddr
struct dhcp_packet {
	uint8_t op;      /* BOOTREQUEST or BOOTREPLY */
	uint8_t htype;   /* hardware address type. 1 = 10mb ethernet */
	uint8_t hlen;    /* hardware address length */
	uint8_t hops;    /* used by relay agents only */
	uint32_t xid;    /* unique id */
	uint16_t secs;   /* elapsed since client began acquisition/renewal */
	uint16_t flags;  /* only one flag so far: */
#define BROADCAST_FLAG 0x8000 /* "I need broadcast replies" */
	uint32_t ciaddr; /* client IP (if client is in BOUND, RENEW or REBINDING state) */
	uint32_t yiaddr; /* 'your' (client) IP address */
	/* IP address of next server to use in bootstrap, returned in DHCPOFFER, DHCPACK by server */
	uint32_t siaddr_nip;
	uint32_t gateway_nip; /* relay agent IP address */
	uint8_t chaddr[16];   /* link-layer client hardware address (MAC) */
	uint8_t sname[64];    /* server host name (ASCIZ) */
	uint8_t file[128];    /* boot file name (ASCIZ) */
	uint32_t cookie;      /* fixed first four option bytes (99,130,83,99 dec) */
	uint8_t options[DHCP_OPTIONS_BUFSIZE];
} ;

struct server_config_t {
	char *interface;                /* interface to use */
//TODO: ifindex, server_nip, server_mac
// are obtained from interface name.
// Instead of querying them *once*, create update_server_network_data_cache()
// and call it before any usage of these fields.
// update_server_network_data_cache() must re-query data
// if more than N seconds have passed after last use.
	int ifindex;
	uint32_t server_nip;
	uint8_t server_mac[6];          /* our MAC address (used only for ARP probing) */
	struct option_set *options;     /* list of DHCP options loaded from the config file */
	/* start,end are in host order: we need to compare start <= ip <= end */
	uint32_t start_ip;              /* start address of leases, in host order */
	uint32_t end_ip;                /* end of leases, in host order */
	uint32_t max_lease_sec;         /* maximum lease time (host order) */
	uint32_t min_lease_sec;         /* minimum lease time a client can request */
	uint32_t max_leases;            /* maximum number of leases (including reserved addresses) */
	uint32_t auto_time;             /* how long should udhcpd wait before writing a config file.
	                                 * if this is zero, it will only write one on SIGUSR1 */
	uint32_t decline_time;          /* how long an address is reserved if a client returns a
	                                 * decline message */
	uint32_t conflict_time;         /* how long an arp conflict offender is leased for */
	uint32_t offer_time;            /* how long an offered address is reserved */
	uint32_t siaddr_nip;            /* "next server" bootp option */
	char *lease_file;
	char *pidfile;
	char *notify_file;              /* what to run whenever leases are written */
	char *sname;                    /* bootp server name */
	char *boot_file;                /* bootp boot file option */
	struct static_lease *static_leases; /* List of ip/mac pairs to assign static leases */
} ;


struct dyn_lease {
	/* Unix time when lease expires. Kept in memory in host order.
	 * When written to file, converted to network order
	 * and adjusted (current time subtracted) */
	leasetime_t expires;
	/* "nip": IP in network order */
	uint32_t lease_nip;
	/* We use lease_mac[6], since e.g. ARP probing uses
	 * only 6 first bytes anyway. We check received dhcp packets
	 * that their hlen == 6 and thus chaddr has only 6 significant bytes
	 * (dhcp packet has chaddr[16], not [6])
	 */
	uint8_t lease_mac[6];
	
	/* modified by liyi<liyi@tp-link.net> at 2014/04/18
	 * 33 means hostname can be at most 32 Englist characters
	 * or 16 Chinese characters(2 bytes each) */
	char hostname[DHCPS_HOSTNAME];
	/* pad must make the stucture a multiply of 4*/
	uint8_t pad[1];
	/* total size is a multiply of 4 */
} ;

void dhcpd_init();

#endif