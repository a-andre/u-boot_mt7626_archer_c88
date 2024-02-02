#include "dhcpd.h"
#include "uip.h"


void dhcpd_appcall(void);
extern struct uip_udp_conn uip_udp_conns[UIP_UDP_CONNS];


/* globals */
struct dyn_lease *g_leases = NULL;
struct server_config_t g_server_config;
char g_run_dhcpd = 0;

const struct dhcp_optflag dhcp_optflags[] = {
	/* flags                                    code */
	{ OPTION_IP                   | OPTION_REQ, 0x01 }, /* DHCP_SUBNET        */
	{ OPTION_S32                              , 0x02 }, /* DHCP_TIME_OFFSET   */
	{ OPTION_IP | OPTION_LIST     | OPTION_REQ, 0x03 }, /* DHCP_ROUTER        */
	{ OPTION_IP | OPTION_LIST     | OPTION_REQ, 0x06 }, /* DHCP_DNS_SERVER    */
	{ OPTION_IP | OPTION_LIST                 , 0x09 }, /* DHCP_LPR_SERVER    */
	{ OPTION_STRING_HOST          | OPTION_REQ, 0x0c }, /* DHCP_HOST_NAME     */
	{ OPTION_U16                              , 0x0d }, /* DHCP_BOOT_SIZE     */
	{ OPTION_STRING_HOST          | OPTION_REQ, 0x0f }, /* DHCP_DOMAIN_NAME   */
	{ OPTION_IP                               , 0x10 }, /* DHCP_SWAP_SERVER   */
	{ OPTION_STRING                           , 0x11 }, /* DHCP_ROOT_PATH     */
	{ OPTION_U8                               , 0x17 }, /* DHCP_IP_TTL        */
	{ OPTION_U16                              , 0x1a }, /* DHCP_MTU           */
	{ OPTION_IP                   | OPTION_REQ, 0x1c }, /* DHCP_BROADCAST     */
	{ OPTION_IP_PAIR | OPTION_LIST | OPTION_REQ, 0x21 }, /* DHCP_ROUTES        */
	{ OPTION_STRING_HOST                      , 0x28 }, /* DHCP_NIS_DOMAIN    */
	{ OPTION_IP | OPTION_LIST                 , 0x29 }, /* DHCP_NIS_SERVER    */
	{ OPTION_IP | OPTION_LIST     | OPTION_REQ, 0x2a }, /* DHCP_NTP_SERVER    */
	{ OPTION_IP | OPTION_LIST                 , 0x2c }, /* DHCP_WINS_SERVER   */
	{ OPTION_U32                              , 0x33 }, /* DHCP_LEASE_TIME    */
	{ OPTION_IP                               , 0x36 }, /* DHCP_SERVER_ID     */
	{ OPTION_STRING                           , 0x38 }, /* DHCP_ERR_MESSAGE   */
//TODO: must be combined with 'sname' and 'file' handling:
	{ OPTION_STRING_HOST                      , 0x42 }, /* DHCP_TFTP_SERVER_NAME */
	{ OPTION_STRING                           , 0x43 }, /* DHCP_BOOT_FILE     */
	{ OPTION_STATIC_ROUTES | OPTION_LIST | OPTION_REQ, 0x79 }, /* DHCP_STATIC_ROUTES */
	{ OPTION_STATIC_ROUTES | OPTION_LIST | OPTION_REQ, 0xf9 }, /* DHCP_MS_STATIC_ROUTES */
	{ OPTION_6RD                              , 0xd4 }, /* DHCP_6RD (RFC)     */
	{ OPTION_6RD                              , 0x96 }, /* DHCP_6RD (Comcast) */
	{ OPTION_STRING                           , 0xfc }, /* DHCP_WPAD          */

	{ OPTION_IP                               , 0x32 }, /* DHCP_REQUESTED_IP  */
	{ OPTION_U8                               , 0x35 }, /* DHCP_MESSAGE_TYPE  */
	{ OPTION_U16                              , 0x39 }, /* DHCP_MAX_SIZE      */
	
	{ 0, 0 } /* zeroed terminating entry */
};

const uint8_t dhcp_option_lengths[] = {
	[OPTION_IP] =      4,
	[OPTION_IP_PAIR] = 8,
	[OPTION_STRING] =  1,  /* ignored by udhcp_str2optset */
	[OPTION_STRING_HOST] = 1,  /* ignored by udhcp_str2optset */
	[OPTION_U8] =      1,
	[OPTION_U16] =     2,
	[OPTION_U32] =     4,
	[OPTION_S32] =     4,
	/* Just like OPTION_STRING, we use minimum length here */
	[OPTION_STATIC_ROUTES] = 5,
	[OPTION_6RD]           = 22,
};

extern unsigned long long get_ticks(void);

unsigned int time(char* p)
{
	return (unsigned int)get_ticks();
}

uint16_t udhcp_checksum(void *addr, int count)
{
	/* Compute Internet Checksum for "count" bytes
	 * beginning at location "addr".
	 */
	int32_t sum = 0;
	uint16_t *source = (uint16_t *) addr;

	while (count > 1)  {
		/*  This is the inner loop */
		sum += *source++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0) {
		/* Make sure that the left-over byte is added correctly both
		 * with little and big endian hosts */
		uint16_t tmp = 0;
		*(uint8_t*)&tmp = *(uint8_t*)source;
		sum += tmp;
	}
	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}

/* Construct a ip/udp header for a packet, send packet */
int  udhcp_send_raw_packet(struct dhcp_packet *dhcp_pkt,
		uint32_t source_nip, int source_port,
		uint32_t dest_nip, int dest_port, const uint8_t *dest_arp)
{
	int fd;
	int p_len = udhcp_get_payload_len(dhcp_pkt);
	
	memcpy(uip_appdata, dhcp_pkt, p_len);

	uip_udp_conn->ttl = 64;
	uip_udp_conn->rport =  htons(dest_port);
	
	uip_udp_conn->ripaddr[0] = htons((unsigned short)(ntohl(dest_nip) >> 16 & 0xFFFF));
	uip_udp_conn->ripaddr[1] = htons((unsigned short)(ntohl(dest_nip) & 0xFFFF));
		
 	uip_udp_send(p_len);

 	return 1;
}

/* Send a packet to a specific mac address and ip address by creating our own ip packet */
static void send_packet_to_client(struct dhcp_packet *dhcp_pkt, int force_broadcast)
{
	const uint8_t *chaddr;
	uint32_t ciaddr;
	const uint8_t MAC_BCAST_ADDR[6]  = {
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff
	};

	// Was:
	//if (force_broadcast) { /* broadcast */ }
	//else if (dhcp_pkt->ciaddr) { /* unicast to dhcp_pkt->ciaddr */ }
	//else if (dhcp_pkt->flags & htons(BROADCAST_FLAG)) { /* broadcast */ }
	//else { /* unicast to dhcp_pkt->yiaddr */ }
	// But this is wrong: yiaddr is _our_ idea what client's IP is
	// (for example, from lease file). Client may not know that,
	// and may not have UDP socket listening on that IP!
	// We should never unicast to dhcp_pkt->yiaddr!
	// dhcp_pkt->ciaddr, OTOH, comes from client's request packet,
	// and can be used.

	if (force_broadcast
	 || (dhcp_pkt->flags & htons(BROADCAST_FLAG))
	 || dhcp_pkt->ciaddr == 0
	) {
		MSG_LOG("Broadcasting packet to client\n");
		ciaddr = 0xFFFFFFFF;
		chaddr = MAC_BCAST_ADDR;
	} else {
		MSG_LOG("Unicasting packet to client ciaddr\n");
		ciaddr = dhcp_pkt->ciaddr;
		chaddr = dhcp_pkt->chaddr;
	}

	

	udhcp_send_raw_packet(dhcp_pkt,
		/*src*/ g_server_config.server_nip, SERVER_PORT,
		/*dst*/ ciaddr, CLIENT_PORT, chaddr);
}

int udhcp_get_payload_len(struct dhcp_packet *dhcp_pkt)
{
	return sizeof(struct dhcp_packet) 
		- DHCP_OPTIONS_BUFSIZE 
		+ udhcp_end_option(dhcp_pkt->options) 
		+ sizeof(dhcp_pkt->options[0]);
}

static void send_packet(struct dhcp_packet *dhcp_pkt, int force_broadcast)
{
	send_packet_to_client(dhcp_pkt, force_broadcast);
}



/* Return the position of the 'end' option (no bounds checking) */
int  udhcp_end_option(uint8_t *optionptr)
{
	int i = 0;

	while (optionptr[i] != DHCP_END) {
		if (optionptr[i] != DHCP_PADDING)
			i += optionptr[i + OPT_LEN] + OPT_DATA-1;
		i++;
	}
	return i;
}


/* Add an option (supplied in binary form) to the options.
 * Option format: [code][len][data1][data2]..[dataLEN]
 */
void  udhcp_add_binary_option(struct dhcp_packet *packet, uint8_t *addopt)
{
	unsigned len;
	uint8_t *optionptr = packet->options;
	unsigned end = udhcp_end_option(optionptr);

	len = OPT_DATA + addopt[OPT_LEN];
	/* end position + (option code/length + addopt length) + end option */
	if (end + len + 1 >= DHCP_OPTIONS_BUFSIZE) {
		MSG_ERR("option 0x%02x did not fit into the packet",
				addopt[OPT_CODE]);
		return;
	}
	memcpy(optionptr + end, addopt, len);
	optionptr[end + len] = DHCP_END;
}


/* Add an one to four byte option to a packet */
void  udhcp_add_simple_option(struct dhcp_packet *packet, uint8_t code, uint32_t data)
{
	const struct dhcp_optflag *dh;

	for (dh = dhcp_optflags; dh->code; dh++) {
		if (dh->code == code) {
			uint8_t option[6], len;

			option[OPT_CODE] = code;
			len = dhcp_option_lengths[dh->flags & OPTION_TYPE_MASK];
			option[OPT_LEN] = len;

#if (BYTE_ORDER == BIG_ENDIAN)
				data <<= 8 * (4 - len);
#endif
			/* Assignment is unaligned! */
			move_to_unaligned32(&option[OPT_DATA], data);
			udhcp_add_binary_option(packet, option);
			return;
		}
	}

	MSG_LOG("can't add option 0x%02x\n", code);
}


void  udhcp_init_header(struct dhcp_packet *packet, char type)
{
	memset(packet, 0, sizeof(*packet));
	packet->op = BOOTREQUEST; /* if client to a server */
	
	switch (type) {
	case DHCPOFFER:
	case DHCPACK:
	case DHCPNAK:
		packet->op = BOOTREPLY; /* if server to client */
	}
	
	packet->htype = 1; /* ethernet */
	packet->hlen = 6;
	packet->cookie = htonl(DHCP_MAGIC);
	if (DHCP_END != 0)
		packet->options[0] = DHCP_END;
	udhcp_add_simple_option(packet, DHCP_MESSAGE_TYPE, type);
}


static void init_packet(struct dhcp_packet *packet, struct dhcp_packet *oldpacket, char type)
{
	/* Sets op, htype, hlen, cookie fields
	 * and adds DHCP_MESSAGE_TYPE option */
	udhcp_init_header(packet, type);

	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, sizeof(oldpacket->chaddr));
	packet->flags = oldpacket->flags;
	packet->gateway_nip = oldpacket->gateway_nip;
	packet->ciaddr = oldpacket->ciaddr;
	udhcp_add_simple_option(packet, DHCP_SERVER_ID, g_server_config.server_nip);
}

/* Fill options field, siaddr_nip, and sname and boot_file fields.
 * TODO: teach this code to use overload option.
 */
static void add_server_options(struct dhcp_packet *packet)
{
	struct option_set *curr = g_server_config.options;

	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			udhcp_add_binary_option(packet, curr->data);
		curr = curr->next;
	}

	packet->siaddr_nip = g_server_config.siaddr_nip;

	if (g_server_config.sname)
		strncpy((char*)packet->sname, g_server_config.sname, sizeof(packet->sname) - 1);
}

/* Emit a string of hex representation of bytes */
char*  bin2hex(char *p, const char *cp, int count)
{
	char hexdigits_upcase[] = "0123456789ABCDEF";
	while (count) {
		unsigned char c = *cp++;
		/* put lowercase hex digits */
		*p++ = 0x20 | hexdigits_upcase[c >> 4];
		*p++ = 0x20 | hexdigits_upcase[c & 0xf];
		count--;
	}
	return p;
}


/* Get an option with bounds checking (warning, result is not aligned) */
uint8_t* udhcp_get_option(struct dhcp_packet *packet, int code)
{
	uint8_t *optionptr;
	int len;
	int rem;
	int overload = 0;
	char tmpBuf[128] = {0};
	
	enum {
		FILE_FIELD101  = FILE_FIELD  * 0x101,
		SNAME_FIELD101 = SNAME_FIELD * 0x101,
	};

	/* option bytes: [code][len][data1][data2]..[dataLEN] */
	optionptr = packet->options;
	rem = sizeof(packet->options);
	
	while (1) {
		if (rem <= 0) {
			MSG_ERR("bad packet, malformed option field\n");
			return NULL;
		}
		
		if (optionptr[OPT_CODE] == DHCP_PADDING) {
			rem--;
			optionptr++;
			continue;
		}
		
		if (optionptr[OPT_CODE] == DHCP_END) {
			if ((overload & FILE_FIELD101) == FILE_FIELD) {
				/* can use packet->file, and didn't look at it yet */
				overload |= FILE_FIELD101; /* "we looked at it" */
				optionptr = packet->file;
				rem = sizeof(packet->file);
				continue;
			}
			
			if ((overload & SNAME_FIELD101) == SNAME_FIELD) {
				/* can use packet->sname, and didn't look at it yet */
				overload |= SNAME_FIELD101; /* "we looked at it" */
				optionptr = packet->sname;
				rem = sizeof(packet->sname);
				continue;
			}
			break;
		}
		
		len = 2 + optionptr[OPT_LEN];
		rem -= len;
		
		if (rem < 0)
			continue; /* complain and return NULL */

		if (optionptr[OPT_CODE] == code) {
			*bin2hex(tmpBuf, (void*) (optionptr+ OPT_DATA), optionptr[OPT_LEN]) = '\0';
			MSG_LOG("Option found code 0x%02X; str: %s\n", code, tmpBuf);
			return optionptr + OPT_DATA;
		}

		if (optionptr[OPT_CODE] == DHCP_OPTION_OVERLOAD) {
			overload |= optionptr[OPT_DATA];
			/* fall through */
		}
		optionptr += len;
	}

	//MSG_LOG("Option 0x%02x not found", code);
	
	return NULL;
}

static uint32_t select_lease_time(struct dhcp_packet *packet)
{
	uint32_t lease_time_sec = g_server_config.max_lease_sec;
	uint8_t *lease_time_opt = udhcp_get_option(packet, DHCP_LEASE_TIME);
	
	if (lease_time_opt) {
		move_from_unaligned32(lease_time_sec, lease_time_opt);
		lease_time_sec = ntohl(lease_time_sec);
		if (lease_time_sec > g_server_config.max_lease_sec)
			lease_time_sec = g_server_config.max_lease_sec;
		if (lease_time_sec < g_server_config.min_lease_sec)
			lease_time_sec = g_server_config.min_lease_sec;
	}
	return lease_time_sec;
}

/* True if a lease has expired */
int  is_expired_lease(struct dyn_lease *lease)
{
	return (lease->expires < (leasetime_t) time(NULL));
}

/* Find the first lease that matches IP, NULL is no match */
struct dyn_lease* find_lease_by_nip(uint32_t nip)
{
	unsigned i;

	for (i = 0; i < g_server_config.max_leases; i++)
		if (g_leases[i].lease_nip == nip)
			return &g_leases[i];

	return NULL;
}

/* Find the oldest expired lease, NULL if there are no expired leases */
static struct dyn_lease *oldest_expired_lease(void)
{
	struct dyn_lease *oldest_lease = NULL;
	leasetime_t oldest_time = time(NULL);
	unsigned i;

	/* Unexpired leases have g_leases[i].expires >= current time
	 * and therefore can't ever match */
	for (i = 0; i < g_server_config.max_leases; i++) {
		if (g_leases[i].expires < oldest_time) {
			oldest_time = g_leases[i].expires;
			oldest_lease = &g_leases[i];
		}
	}
	return oldest_lease;
}

/* Clear out all leases with matching nonzero chaddr OR yiaddr.
 * If chaddr == NULL, this is a conflict lease.
 */
static void clear_leases(const uint8_t *chaddr, uint32_t yiaddr)
{
	unsigned i;

	for (i = 0; i < g_server_config.max_leases; i++) {
		if ((chaddr && memcmp(g_leases[i].lease_mac, chaddr, 6) == 0)
		 || (yiaddr && g_leases[i].lease_nip == yiaddr)
		) {
			memset(&g_leases[i], 0, sizeof(g_leases[i]));
		}
	}
}


/* Add a lease into the table, clearing out any old ones.
 * If chaddr == NULL, this is a conflict lease.
 */
struct dyn_lease*  add_lease(
		const uint8_t *chaddr, uint32_t yiaddr,
		leasetime_t leasetime,
		const char *hostname, int hostname_len)
{
	struct dyn_lease *oldest;

	/* clean out any old ones */
	clear_leases(chaddr, yiaddr);

	oldest = oldest_expired_lease();

	if (oldest) {
		memset(oldest, 0, sizeof(*oldest));
		if (hostname) {
			char *p;

			hostname_len++; /* include NUL */
			if (hostname_len > sizeof(oldest->hostname))
			{
				/* one char will be left for NUL char at the end of hostname */
				hostname_len = sizeof(oldest->hostname) - 1;
			}
			p = strncpy(oldest->hostname, hostname, hostname_len);
			/* sanitization (s/non-ASCII/^/g) */
			while (*p) {
				if (*p < ' ' || *p > 126)
					*p = '^';
				p++;
			}
		}
		if (chaddr)
			memcpy(oldest->lease_mac, chaddr, 6);
		oldest->lease_nip = yiaddr;
		if (leasetime == 0xFFFFFFFF) 
		{
			/* the static lease time will be 0xFFFFFFFF */
			oldest->expires = leasetime;
		} 
		else {
			oldest->expires = time(NULL) + leasetime;
		}
	}

	return oldest;
}

/* Check if the IP is taken; if it is, add it to the lease table */
static int nobody_responds_to_arp(uint32_t nip, const uint8_t *safe_mac)
{
/* TODO. by HouXB, 28May14 */
#if 0
	struct in_addr temp;
	int r;

	r = arpping(nip, safe_mac,
			g_server_config.server_nip,
			g_server_config.server_mac,
			g_server_config.interface);
	if (r)
		return r;

	temp.s_addr = nip;
	add_lease(NULL, nip, g_server_config.conflict_time, NULL, 0);
#endif	
	return 0;
}

/* Find a new usable (we think) address */
uint32_t  find_free_or_expired_nip(const uint8_t *safe_mac)
{
	uint32_t addr;
	struct dyn_lease *oldest_lease = NULL;


	addr = g_server_config.start_ip;
#define stop (g_server_config.end_ip + 1)
	do {
		uint32_t nip;
		struct dyn_lease *lease;

		/* ie, 192.168.55.0 */
		if ((addr & 0xff) == 0)
			goto next_addr;
		
		/* ie, 192.168.55.255 */
		if ((addr & 0xff) == 0xff)
			goto next_addr;
		
		nip = htonl(addr);
		/* skip our own address */
		if (nip == g_server_config.server_nip)
			goto next_addr;

		lease = find_lease_by_nip(nip);
		if (!lease) {
//TODO: DHCP servers do not always sit on the same subnet as clients: should *ping*, not arp-ping!
			//if (nobody_responds_to_arp(nip, safe_mac))
				return nip;
		} else {
			if (!oldest_lease || lease->expires < oldest_lease->expires)
				oldest_lease = lease;
		}

 next_addr:
		addr++;
	} while (addr != stop);

	if (oldest_lease
	 && is_expired_lease(oldest_lease)
	 && nobody_responds_to_arp(oldest_lease->lease_nip, safe_mac)
	) {
		return oldest_lease->lease_nip;
	}

	return 0;
}


/* We got a DHCP DISCOVER. Send an OFFER. */
/* NOINLINE: limit stack usage in caller */
static void send_offer(struct dhcp_packet *oldpacket,		
		struct dyn_lease *lease,
		uint8_t *requested_ip_opt)
{
	struct dhcp_packet packet;
	uint32_t lease_time_sec;
	const char *p_host_name;

	/* We have no static lease for client's chaddr */
	uint32_t req_nip;

	init_packet(&packet, oldpacket, DHCPOFFER);

	if (lease) {
		/* We have a dynamic lease for client's chaddr.
		 * Reuse its IP (even if lease is expired).
		 * Note that we ignore requested IP in this case.
		 */
		packet.yiaddr = lease->lease_nip;
	}
	/* Or: if client has requested an IP */
	else if (requested_ip_opt != NULL
			/* (read IP) */
			&& (move_from_unaligned32(req_nip, requested_ip_opt), 1)
			/* and the IP is in the lease range */
			&& ntohl(req_nip) >= g_server_config.start_ip
			&& ntohl(req_nip) <= g_server_config.end_ip
			/* and */
			&& (  !(lease = find_lease_by_nip(htonl(req_nip))) /* is not already taken */
				|| is_expired_lease(lease) /* or is taken, but expired */
			   )
			) {
		packet.yiaddr = req_nip;
	}
	else {
		/* Otherwise, find a free IP */
		packet.yiaddr = find_free_or_expired_nip(oldpacket->chaddr);
	}

	if (!packet.yiaddr) {
		MSG_ERR("no free IP addresses. OFFER abandoned\n");
		return;
	}
	/* Reserve the IP for a short time hoping to get DHCPREQUEST soon */
	p_host_name = (const char*) udhcp_get_option(oldpacket, DHCP_HOST_NAME);
	lease = add_lease(packet.chaddr, packet.yiaddr,
			g_server_config.offer_time,
			p_host_name,
			p_host_name ? (unsigned char)p_host_name[OPT_LEN - OPT_DATA] : 0
	);
	if (!lease) {
		MSG_ERR("no free IP addresses. OFFER abandoned");
		return;
	}
	lease_time_sec = select_lease_time(oldpacket);	

	udhcp_add_simple_option(&packet, DHCP_LEASE_TIME, htonl(lease_time_sec));
	add_server_options(&packet);
	
	/* send_packet emits error message itself if it detects failure */
	send_packet(&packet, /*force_bcast:*/ 0);
}

/* NOINLINE: limit stack usage in caller */
static void send_NAK(struct dhcp_packet *oldpacket)
{
	struct dhcp_packet packet;

	init_packet(&packet, oldpacket, DHCPNAK);

	MSG_LOG("Sending NAK");
	send_packet(&packet, /*force_bcast:*/ 1);
}

/* NOINLINE: limit stack usage in caller */
static void send_ACK(struct dhcp_packet *oldpacket, uint32_t yiaddr)
{
	struct dhcp_packet packet;
	uint32_t lease_time_sec;

	const char *p_host_name;

	init_packet(&packet, oldpacket, DHCPACK);
	packet.yiaddr = yiaddr;

	lease_time_sec = select_lease_time(oldpacket);

	udhcp_add_simple_option(&packet, DHCP_LEASE_TIME, htonl(lease_time_sec));

	add_server_options(&packet);

	send_packet(&packet, /*force_bcast:*/ 0);

	p_host_name = (const char*) udhcp_get_option(oldpacket, DHCP_HOST_NAME);
	add_lease(packet.chaddr, packet.yiaddr,
			lease_time_sec,
			p_host_name,
			p_host_name ? (unsigned char)p_host_name[OPT_LEN - OPT_DATA] : 0
			);

}

struct option_set* udhcp_find_option(struct option_set *opt_list, uint8_t code)
{
	while (opt_list && opt_list->data[OPT_CODE] < code)
		opt_list = opt_list->next;

	if (opt_list && opt_list->data[OPT_CODE] == code)
		return opt_list;
	return NULL;
}

static void send_inform(struct dhcp_packet *oldpacket)
{
	struct dhcp_packet packet;

	/* "If a client has obtained a network address through some other means
	 * (e.g., manual configuration), it may use a DHCPINFORM request message
	 * to obtain other local configuration parameters.  Servers receiving a
	 * DHCPINFORM message construct a DHCPACK message with any local
	 * configuration parameters appropriate for the client without:
	 * allocating a new address, checking for an existing binding, filling
	 * in 'yiaddr' or including lease time parameters.  The servers SHOULD
	 * unicast the DHCPACK reply to the address given in the 'ciaddr' field
	 * of the DHCPINFORM message.
	 * ...
	 * The server responds to a DHCPINFORM message by sending a DHCPACK
	 * message directly to the address given in the 'ciaddr' field
	 * of the DHCPINFORM message.  The server MUST NOT send a lease
	 * expiration time to the client and SHOULD NOT fill in 'yiaddr'."
	 */
	//TODO: do a few sanity checks: is ciaddr set?
	//Better yet: is ciaddr == IP source addr?
	init_packet(&packet, oldpacket, DHCPACK);
	add_server_options(&packet);

	send_packet(&packet, /*force_bcast:*/ 0);
}


/* Find the first lease that matches MAC, NULL if no match */
struct dyn_lease* find_lease_by_mac(const uint8_t *mac)
{
	unsigned i;

	for (i = 0; i < g_server_config.max_leases; i++)
		if (memcmp(g_leases[i].lease_mac, mac, 6) == 0)
			return &g_leases[i];

	return NULL;
}


/* helper: add an option to the opt_list */
static  void attach_option(
		struct option_set **opt_list,
		const struct dhcp_optflag *optflag,
		char *buffer,
		int length)
{
	struct option_set *existing, *will, **curr;

	existing = udhcp_find_option(*opt_list, optflag->code);
	if (!existing) {
		MSG_LOG("Attaching option %02x to list\n", optflag->code);		

		/* make a new option */
		will = malloc(sizeof(*will));
		will->data = malloc(length + OPT_DATA);
		will->data[OPT_CODE] = optflag->code;
		will->data[OPT_LEN] = length;
		memcpy(will->data + OPT_DATA, buffer, length);

		curr = opt_list;
		while (*curr && (*curr)->data[OPT_CODE] < optflag->code)
			curr = &(*curr)->next;

		will->next = *curr;
		*curr = will;
		goto ret;
	}

	if (optflag->flags & OPTION_LIST) {
		unsigned old_len;

		/* add it to an existing option */
		MSG_LOG("Attaching option %02x to existing member of list\n", optflag->code);

		old_len = existing->data[OPT_LEN];
		if (old_len + length < 255) {
			/* actually 255 is ok too, but adding a space can overlow it */

			existing->data = realloc(existing->data, OPT_DATA + 1 + old_len + length);
			if ((optflag->flags & OPTION_TYPE_MASK) == OPTION_STRING
			 || (optflag->flags & OPTION_TYPE_MASK) == OPTION_STRING_HOST
			) {
				/* add space separator between STRING options in a list */
				existing->data[OPT_DATA + old_len] = ' ';
				old_len++;
			}
			memcpy(existing->data + OPT_DATA + old_len, buffer, length);
			existing->data[OPT_LEN] = old_len + length;
		} /* else, ignore the data, we could put this in a second option in the future */
	} /* else, ignore the new data */

 ret:
 	return;

}

int init_config()
{
	struct dhcp_optflag optflag;
	
	memset(&g_server_config, 0, sizeof(struct server_config_t));	

	char buf[4] = {0xFF, 0xFF, 0xFF, 0x00};

	
	g_server_config.start_ip = DHCPD_POOL_START_IP_HEX; /* 192.168.0.12 */
	g_server_config.end_ip = DHCPD_POOL_END_IP_HEX; /* 192.168.0.100 */
	g_server_config.max_lease_sec = 86400; //24 hours
	g_server_config.min_lease_sec = 43200;
	g_server_config.max_leases = g_server_config.end_ip - g_server_config.start_ip;

	g_server_config.server_nip = DHCPD_SERVER_IP_HEX; /* 192.168.0.1 */
	g_server_config.server_mac[0] = 0x01;
	g_server_config.server_mac[1] = 0x02;
	g_server_config.server_mac[2] = 0x03;
	g_server_config.server_mac[3] = 0x04;
	g_server_config.server_mac[4] = 0x05;
	g_server_config.server_mac[5] = 0x06;
	
	g_server_config.offer_time = 30;

	if(g_leases == NULL)
	{
		g_leases = malloc(g_server_config.max_leases*sizeof(struct dyn_lease));
	}
	memset(g_leases, 0, g_server_config.max_leases*sizeof(struct dyn_lease));
	
	/* DHCP_SUBNET */
	optflag.flags = OPTION_IP | OPTION_REQ;
	optflag.code = 0x01;
	attach_option(&g_server_config.options, &optflag,  buf, 4);

#if 0
	/* DHCP_LEASE_TIME */
	optflag.flags = OPTION_U32;
	optflag.code = 0x33;
	attach_option(&g_server_config.options, &optflag,  "lease", strlen("lease"));
#endif
	uint32_t server = htonl(DHCPD_SERVER_IP_HEX);
	char *serverBuf = (uint8_t *)&server;

	/* DHCP ROUTER.*/
	optflag.flags = OPTION_IP |OPTION_LIST | OPTION_REQ;
	optflag.code = 0x03 ;
	/* buf may not be 4-aligned. */
	memcpy(buf, serverBuf, sizeof(server));

	attach_option(&g_server_config.options, &optflag,  buf, 4);

	/* DHCP DNS SERVER. */
	optflag.flags = OPTION_IP |OPTION_LIST | OPTION_REQ;
	optflag.code = 0x06;
	memcpy(buf, serverBuf, sizeof(server));

	attach_option(&g_server_config.options, &optflag,  buf, 4);

	g_server_config.sname = (char*)malloc(strlen(SNAME));

	if (NULL == g_server_config.sname)
	{
		MSG_ERR("malloc failed!\n");		
		
		if (g_server_config.sname)
			free(g_server_config.sname);		
		return 0;
	}
	
	strncpy(g_server_config.sname, SNAME, strlen(SNAME));
	
}


void dhcpd_init()
{
	init_config();
	uip_udp_listen(HTONS(DHCPD_LISTEN_PORT));

	printf("file: %s,line: %d==:dhcpd init OK. --debug by HouXB\n",
			__FILE__, __LINE__);
}

/* Find static lease IP by mac */
uint32_t  get_static_nip_by_mac(struct static_lease *st_lease, void *mac)
{
	while (st_lease) {
		if (memcmp(st_lease->mac, mac, 6) == 0)
			return st_lease->nip;
		st_lease = st_lease->next;
	}

	return 0;
}

struct dhcp_packet g_packet;



int handle_dhcpd()
{
	struct dhcp_packet *packet = &g_packet;
	

	uint8_t *state;
	uint8_t *server_id_opt;
	uint8_t *requested_ip_opt;
	uint32_t requested_nip = requested_nip; /* for compiler */
	struct dyn_lease *lease;
	struct option_set *option;
	uint8_t *prequest;
	
	//hex_dump( (unsigned char*)(uip_appdata), uip_datalen());
	memset(packet, 0, sizeof(g_packet));
	memcpy(packet, uip_appdata, uip_datalen());

	
	if (packet->hlen != 6) 
	{
		MSG_ERR("MAC length != 6, ignoring packet\n");
		return -1;
	}
	
	if (packet->op != BOOTREQUEST) {
		MSG_ERR("not a REQUEST, ignoring packet\n");
		return -2;
	}
	
	state = udhcp_get_option(packet, DHCP_MESSAGE_TYPE);
	
	if (state == NULL || state[0] < DHCP_MINTYPE || state[0] > DHCP_MAXTYPE) {
		MSG_ERR("no or bad message type option, ignoring packet\n");
		return -3;
	}

	/* Get SERVER_ID if present */
	server_id_opt = udhcp_get_option(packet, DHCP_SERVER_ID);
	if (server_id_opt) {
		uint32_t server_id_network_order;
		move_from_unaligned32(server_id_network_order, server_id_opt);
		
		if (server_id_network_order != g_server_config.server_nip) 
		{
			/* client talks to somebody else */
			MSG_LOG("server ID doesn't match, ignoring\n");
			return -4;
		}
	}		

	lease = find_lease_by_mac(packet->chaddr);

	/* Get REQUESTED_IP if present */
	requested_ip_opt = udhcp_get_option(packet, DHCP_REQUESTED_IP);
	if (requested_ip_opt) {
		move_from_unaligned32(requested_nip, requested_ip_opt);
	}
	prequest = udhcp_get_option(packet, DHCP_MESSAGE_TYPE);

	switch (state[0]) {

	case DHCPDISCOVER:
		MSG_LOG("Received DISCOVER\n");

		send_offer(packet, lease, requested_ip_opt);
		break;

	case DHCPREQUEST:
		MSG_LOG("Received REQUEST");

		if (!requested_ip_opt) {
			requested_nip = ntohl(packet->ciaddr);
			if (requested_nip == 0) {
				MSG_LOG("no requested IP and no ciaddr, ignoring");
				break;
			}
		}
		
		if (lease && requested_nip == ntohl(lease->lease_nip)) {
			/* client requested or configured IP matches the lease.
			 * ACK it, and bump lease expiration time. */
			send_ACK(packet, lease->lease_nip);
			break;
		}
		/* No lease for this MAC, or lease IP != requested IP */

		if (server_id_opt    /* client is in SELECTING state */
		 || requested_ip_opt /* client is in INIT-REBOOT state */
		 || prequest         /* client is in REQUEST state, add by liyi 14/04/23 */
		) {
			/* "No, we don't have this IP for you" */
			send_NAK(packet);
		} /* else: client is in RENEWING or REBINDING, do not answer */

		break;

	case DHCPDECLINE:			
		MSG_LOG("Received DECLINE\n");
		if (server_id_opt
		 && requested_ip_opt
		 && lease  /* chaddr matches this lease */
		 && requested_nip == ntohl(lease->lease_nip)
		) {
			memset(lease->lease_mac, 0, sizeof(lease->lease_mac));
			lease->expires = time(NULL) + g_server_config.decline_time;
		}
		break;

	case DHCPINFORM:
		MSG_LOG("Received INFORM\n");
		send_inform(packet);
		break;
	}

	return 0;
}

void dhcpd_appcall(void)
{
	handle_dhcpd();
}

