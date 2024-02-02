#ifndef _HTTP_UPGRADE_H_
#define _HTTP_UPGRADE_H_
#include "flash_layout.h"

typedef struct _RSA_SIGN_INFO
{
	unsigned char rsasign[256];
	unsigned short rsaLen;
}RSA_SIGN_INFO;

/* board specific implementation */
extern int do_http_check(const ulong size);
extern int do_http_upgrade(const ulong size);
extern int do_http_progress(const int state);
extern int validateLocalFirmware();

#endif //_HTTP_UPGRADE_H_
