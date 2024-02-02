#ifndef __BOOT_CFG_H__
#define __BOOT_CFG_H__
#include "flash_layout.h"


/*
BOOTCFG分区格式：BOOT_CONFIG_HEADER + tpTLV格式数据
tpTLV格式：Type(2Byte) + Length(2Byte,not include TLV_HDR_LEN or padding length) + Value(4BytePadding)
*/

#define BOOT_CONFIG_MAGIC                   (0x42434647)

#define BOOT_CONFIG_TLV_HEAD_LEN            (4) /*TLV_HDR_LEN*/
#define BOOT_CONFIG_TLV_PAD_LEN             (4) /*sizeof(gTLVPaddings)*/
#define ROUND_DOWN(x, align)				((int)(x) & ~(align - 1))
#define BOOT_CONFIG_TLV_TOTAL_LEN(len)      ROUND_DOWN(len + 4 + 4, 4)

/*!
    @brief      ptTLV值的类型
*/
enum
{
    BOOT_TLV_TYPE_UINT8 = 0,                /*!< UINT8*/
    BOOT_TLV_TYPE_UINT16,                   /*!< UINT16*/
    BOOT_TLV_TYPE_UINT32,                   /*!< UINT32*/
    BOOT_TLV_TYPE_UINT64,                   /*!< UINT64*/
    BOOT_TLV_TYPE_STRING,                   /*!< STRING*/
    BOOT_TLV_TYPE_MAX,
};

/*!
    @brief      BOOTCFG分区配置类型
*/
enum BOOT_CONFIG_TYPE
{
    BOOT_CONFIG_TYPE_FIRMWARE = 0x0001,     /*!< 当前使用的FIRMWARE:UINT8*/
};

#define BOOT_CONFIG_FIRMWARE_1              (0)
#define BOOT_CONFIG_FIRMWARE_2              (1)
#define BOOT_CONFIG_FIRMWARE_DEF            BOOT_CONFIG_FIRMWARE_1


/*!
    @brief      BOOTCFG分区头部
*/
typedef struct _BOOT_CONFIG_HEADER
{
    UINT32        magic;                    /*!< magic number, "BCFG" */
    UINT32        crc32;                    /*!< crc32，计算前crc先清零，校验从头部开始计算，长度为len */
    UINT32        partitionSize;            /*!< 分区大小 */
    UINT32        len;                      /*!< Length of config including header */
    UINT32        reserved[16];             /*!< reserved */
    UINT8         data[0];                  /*!< tpTLV格式，详见tpTLV.c */
} BOOT_CONFIG_HEADER;



BOOL checkBootCfgFormat(UINT8 *buf);
INT32 getBootCfgValue(UINT16 cfgType, VOID *pData, UINT32 valueType, UINT32 maxSize);
INT32 setBootCfgValue(UINT16 cfgType, VOID *pData, UINT32 valueType);

BLOCK_INDEX getCurrentFirmwareIndex(void);
BLOCK_INDEX getNextFirmwareIndex(void);
INT32 setCurrentFirmwareIndex(BLOCK_INDEX index);

#endif /* __BOOT_CFG_H__ */

