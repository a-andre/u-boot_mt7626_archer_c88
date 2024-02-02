/******************************************************************************
*
* Copyright (c) 1996 -2014 TP-LINK Technologies CO.,LTD.
* All rights reserved.
*
* FILE NAME  :   lib_uip.c
* VERSION    :   1.0
* DESCRIPTION:   functions provided by lib_uip..
*
* AUTHOR     :   HouXB <houxubo@tp-link.net>
* CREATE DATE:   06/30/2014
*
* HISTORY    :
* 01   06/30/2014  HouXB     Create.
*
******************************************************************************/

#include "lib_uip.h"
#include "uipopt.h"
#include "uip.h"
#include "uip_arp.h"
#include "uip_apps.h"
#include "http_upgrade.h"
#include "NetDriver.h"

DECLARE_GLOBAL_DATA_PTR;

unsigned char *webfailsafe_data_pointer = NULL;
int	webfailsafe_is_running = 0;
int	webfailsafe_ready_for_upgrade = 0;
int	webfailsafe_firmware_chk_success = 0;

#ifdef CONFIG_UIP

/* start uIP stack,  http service,	dhcp service.*/
void startFailsafeService()
{
	uip_apps_init();
}

static int arptimer = 0;

void HttpdHandler(void){
	int i;

	for(i = 0; i < UIP_CONNS; i++){
		uip_periodic(i);

		if(uip_len > 0){
			uip_arp_out();
			NetSendHttpd(uip_buf, uip_len);
		}
	}

	if(++arptimer == 2){
		uip_arp_timer();
		arptimer = 0;
	}
}

/* *************************************
 *
 * HTTP web server for web failsafe mode
 *
 ***************************************/
int NetLoopHttpd(void){
	bd_t *bd = gd->bd;
	unsigned short int ip[2];
	struct uip_eth_addr eaddr;
	unsigned long long start;
	unsigned long long access_time = 0;
	unsigned long long elapse_time = 0;
	unsigned long long ack_time = 0;
	int on = 1;
	int mark_start = 0;
	int mark_ack = 0;
	ulong freq = get_tbclk();
	ulong flash_delay = freq / 20;
	ulong idle_delay = freq * 5;
	ulong finish_delay = freq * 3;
	ulong ack_delay = freq * 3;

	NetInitGlobalHttpd();
	// restart label
	restart:
	
	// show current progress of the process
	do_http_progress(WEBFAILSAFE_PROGRESS_START);

	if(NetInitLocalHttpd() < 0)
	{
		do_http_progress(WEBFAILSAFE_PROGRESS_UPGRADE_ABORTED);
		return -1;
	}
	
	startFailsafeService();

	get_eth_addr(eaddr.addr);

	// set MAC address
	uip_setethaddr(eaddr);

	// start server...
	printf("HTTP server is starting at IP: %u.%u.%u.%u\n",
			(ntohl(bd->bi_ip_addr) & 0xff000000) >> 24, 
			(ntohl(bd->bi_ip_addr) & 0x00ff0000) >> 16, 
			(ntohl(bd->bi_ip_addr) & 0x0000ff00) >> 8, 
			(ntohl(bd->bi_ip_addr) & 0x000000ff));

	// set local host ip address
	ip[0] = htons((ntohl(bd->bi_ip_addr) & 0xFFFF0000) >> 16);
	ip[1] = htons(ntohl(bd->bi_ip_addr) & 0x0000FFFF);

	printf("file: %s,line: %d==:uip set %x-%x. --debug by HouXB\n",
			__FILE__, __LINE__, ip[0], ip[1]);
	uip_sethostaddr(ip);

	// set network mask (255.255.255.0 -> local network)
	ip[0] = htons((0xFFFFFF00 & 0xFFFF0000) >> 16);
	ip[1] = htons(0xFFFFFF00 & 0x0000FFFF);

	uip_setnetmask(ip);

	// should we also set default router ip address?
	//uip_setdraddr();

	webfailsafe_is_running = 1;
	elapse_time = access_time = get_ticks();
	printf("file: %s,line: %d==:start infinite loop! --debug by HouXB\n",
			__FILE__, __LINE__);
	
	// infinite loop
	for(;;){

		/* blink leds. */
		if (get_ticks() - elapse_time > flash_delay) {
			elapse_time = get_ticks();
			on = !on;
			set_sys_led(on);
		}
		/*
		 *	Check the ethernet for a new packet.
		 *	The ethernet receive routine will process it.
		 */
		if(eth_rx() > 0) {
			access_time = get_ticks();
		}
		else {
			//if time out.
			if (get_ticks() - access_time > idle_delay) {
				HttpdHandler();
				access_time = get_ticks();
			}
		}

		// if CTRL+C was pressed -> return!
		if(ctrlc()){
			eth_halt();

			// reset global variables to default state
			webfailsafe_is_running = 0;
			webfailsafe_ready_for_upgrade = 0;

			do_http_progress(WEBFAILSAFE_PROGRESS_UPGRADE_ABORTED);

			return(-1);
		}

		//if post is done wait 3 sec for the client to ack the response.
		if(!webfailsafe_ready_for_upgrade && webfailsafe_firmware_chk_success)
		{
			if(!mark_ack)
			{
				mark_ack = 1;
				ack_time = get_ticks();
			}
			else if(get_ticks() - ack_time >= ack_delay)
			{
				webfailsafe_ready_for_upgrade = 1;
			}
		}
		
		// until upload is not completed, get back to the start of the loop
		if(!webfailsafe_ready_for_upgrade){
			continue;
		}
		else if(!mark_start)
		{
			mark_start = 1;
			start = get_ticks();
			continue;
		}
		else if(get_ticks() - start < finish_delay)
		{
			continue;
		}
		else
		{
			mark_start = 0;
		}

		// stop eth interface
		eth_halt();

		// show progress
		do_http_progress(WEBFAILSAFE_PROGRESS_UPLOAD_READY);

		// try to make upgrade!
		if(do_http_upgrade(NetBootFileXferSize) >= 0){
			milisecdelay(500);

			do_http_progress(WEBFAILSAFE_PROGRESS_UPGRADE_READY);

			milisecdelay(500);

			// reset the board
			do_reset(NULL, 0, 0, NULL);

		}
		break;
	}

	// reset global variables to default state
	webfailsafe_is_running = 0;
	webfailsafe_ready_for_upgrade = 0;
	webfailsafe_firmware_chk_success = 0;

	NetBootFileXferSize = 0;

	do_http_progress(WEBFAILSAFE_PROGRESS_UPGRADE_FAILED);

	mark_start = 0;
	mark_ack = 0;
	// go to restart
	goto restart;

	return(-1);
}

#define BUF	((struct uip_eth_hdr *)&uip_buf[0])
void ProcessReceivedPacket();

void NetReceiveHttpd(volatile uchar * inpkt, int len){
	/* printf("\nNetReceiveHttpd called -- Debug by CaiBin\n"); */
	memcpy(uip_buf, (const void *)inpkt, len);
	uip_len = len;
	ProcessReceivedPacket();
}

void ProcessReceivedPacket()
{
	if(uip_len==0)
		return;
	if(BUF->type == htons(UIP_ETHTYPE_IP)){
		uip_arp_ipin();
		uip_input();

		if(uip_len > 0){
			uip_arp_out();
			NetSendHttpd(uip_buf, uip_len);
		}
	} else if(BUF->type == htons(UIP_ETHTYPE_ARP)){
		uip_arp_arpin();

		if(uip_len > 0){
			NetSendHttpd(uip_buf, uip_len);
		}
	}
}

#endif
