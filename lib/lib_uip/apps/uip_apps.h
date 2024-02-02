/******************************************************************************
*
* Copyright (c) 1996 -2014 TP-LINK Technologies CO.,LTD.
* All rights reserved.
*
* FILE NAME  :   uip_apps.h
* VERSION    :   1.0
* DESCRIPTION:   the entry  for apps with uip..
*
* AUTHOR     :   HouXB <houxubo@tp-link.net>
* CREATE DATE:   06/30/2014
*
* HISTORY    :
* 01   06/30/2014  HouXB     Create.
*
******************************************************************************/
#ifndef UIP_APPS_H_
#define UIP_APPS_H_

#include "httpd.h"
#include "dhcpd.h"

/****** APP INIT*******/
void uip_apps_init();

/******** TCP ********/
void tcp_appcall(void);

#ifndef UIP_APPCALL
#define UIP_APPCALL		tcp_appcall
#endif

typedef struct httpd_state uip_tcp_appstate_t;

/* UIP_APPSTATE_SIZE: 
	The size of the application-specific state stored in the uip_conn structure. */
#ifndef UIP_APPSTATE_SIZE
#define UIP_APPSTATE_SIZE (sizeof(struct httpd_state))
#endif


/******** UDP ********/
void udp_appcall(void);

#ifndef UIP_UDP_APPCALL
#define UIP_UDP_APPCALL  udp_appcall
#endif

typedef struct dhcpd_state uip_udp_appstate_t;

#endif

