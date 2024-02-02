#ifndef __FLASH_LAYOUT_H__
#define __FLASH_LAYOUT_H__
#include <common.h>
#include "image_header.h"
#include "malloc.h"


#define LOCAL static
#define BOOL int
#define BYTE UINT8
#define ULONG UINT32

#define FALSE 0
#define TRUE 1

#define VOID void
#define OK 0
#define ERROR (-1)

/* Error code for uboot upgrade. */
#define ERR_NONE                           0
#define ERR_GENERIC                       -1
#define ERR_RSA_CHECK_FAIL                -2
#define ERR_HWID_NOT_FOUND                -3
#define ERR_FWID_NOT_FOUND                -4
#define ERR_HWID_NOT_SUPPORTED            -5
#define ERR_FWID_NOT_SUPPORTED            -6
#define ERR_PARTITION_TYPE_NOT_SUPPORTED  -7
#define ERR_INCORRECT_FILE_SIZE           -8
#define ERR_READ_FLASH                    -9
#define ERR_WRITE_FLASH                   -10
#define ERR_ERASE_FLASH                   -11
#define ERR_BAD_ADDRESS                   -12
#define ERR_MEMORY_ALLOC                  -13
#define ERR_INVALID_TP_HEADER             -14
#define ERR_PARTITION_VALIDATION_FAILED   -15

#ifndef UIP_FLASH_BASE
#define UIP_FLASH_BASE CFG_FLASH_BASE
#endif
#ifndef UIP_FLASH_SIZE
#define UIP_FLASH_SIZE CFG_FLASH_SIZE
#endif
#define ALIGN_4K(x)	(((x)|0xfff)+1)

#define UIP_FLASH_BLOCK_ADDR(index)		(UIP_FLASH_BASE + getBlockAddr(index))
#define IMG_HEADER_SIZE					(sizeof(IMG_HEADER))

#define RECOVERY_UPGRADE_VEBOSE
//#define RECOVERY_UPGRADE_UNIT_TEST

#ifdef RECOVERY_UPGRADE_VEBOSE
#define DBG(fmt, ...)   printf( fmt "\n", ## __VA_ARGS__ );
#else
#define DBG(fmt,...)
#endif

#ifdef RECOVERY_UPGRADE_UNIT_TEST
#define DBG_UNIT DBG
#else
#define DBG_UNIT(...)
#endif

#define WARNING(fmt, ...)	{ printf("warning: " fmt "\n", ## __VA_ARGS__ ); }
#define ERR(fmt, ...)		{ printf("error: " fmt "\n", ## __VA_ARGS__ ); }
#define DEBUG(fmt, ...)		{ printf("debug: " fmt "\n", ## __VA_ARGS__ ); }

typedef enum _BLOCK_INDEX
{
	BOOTIMG_BLOCK_INDEX = 0,
	FIRMWARE_BLOCK_INDEX = 1,
	CONFIG_BLOCK_INDEX = 6,
	PROFILE_BLOCK_INDEX = 8,
	BOOTCFG_BLOCK_INDEX = 11,
	FIRMWARE2_BLOCK_INDEX = 12
}BLOCK_INDEX;

int getBlockMaxSize(int index);
int getBlockAddr(int index);
int writeSpecifyBlock(BLOCK_INDEX index, UINT32 len, char *buf);
int readSpecifyBlock(BLOCK_INDEX index, int len, char *buf);


#endif /* __FLASH_LAYOUT_H__ */
