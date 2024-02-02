#ifndef _NET_DRIVER_H_
#define _NET_DRIVER_H_
#include "net.h"

#ifdef CONFIG_UIP

extern ulong	NetBootFileXferSize;
void NetInitGlobalHttpd();
int NetInitLocalHttpd();
void get_eth_addr(uint8_t* addr);

#endif // #ifdef CONFIG_UIP
#endif //_NET_DRIVER_H_