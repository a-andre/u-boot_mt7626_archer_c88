#ifndef _FLASH_OPS_H_
#define _FLASH_OPS_H_
#include "flash_layout.h"

int writeFlash(uint32_t addrOffset, uint8_t* buf, uint32_t buflen);
int readFlash(uint32_t addrOffset, uint8_t* buf, uint32_t buflen);
int eraseFlash(uint32_t addrOffset, uint32_t eraselen);

#endif