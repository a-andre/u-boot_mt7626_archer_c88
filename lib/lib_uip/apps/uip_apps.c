/******************************************************************************
*
* Copyright (c) 1996 -2014 TP-LINK Technologies CO.,LTD.
* All rights reserved.
*
* FILE NAME  :   uip_apps.c
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
#include "uip.h"

/* 	max listen ports:
	tcp: UIP_LISTENPORTS
	udp: UIP_UDP_CONNS
	by HouXB, 30Jun14 */
void uip_apps_init()
{
	uip_init();
	httpd_init();
	dhcpd_init();	
}

/******************************************************************************
* FUNCTION		: tcp_appcall()
* AUTHOR		: HouXB <houxubo@tp-link.net>
* DESCRIPTION	: tcp stream handler, service is based on port. 
* INPUT			: 
* OUTPUT		: 
* RETURN		: 
******************************************************************************/
void tcp_appcall(void)
{

	switch(uip_conn->lport)
	{
	case HTONS(HTTPD_LISTEN_PORT):
		httpd_appcall();
		break;
	default:
		break;
	}
}


/******************************************************************************
* FUNCTION		: udp_appcall()
* AUTHOR		: HouXB <houxubo@tp-link.net>
* DESCRIPTION	: udp data package handler, service is based on port. 
* INPUT			: 
* OUTPUT		: 
* RETURN		: 
******************************************************************************/
void udp_appcall(void)
{
	switch(uip_udp_conn->lport)
	{
	case HTONS(DHCPD_LISTEN_PORT):
		dhcpd_appcall();
		break;
	default:
		uip_abort();
		break;		
	}	
}