#include "tpTLV.h"
#include "bootCfg.h"
#include "flash_ops.h"
#define BOOTCFG_DYNAMIC_MALLOC(_size)       malloc(_size)
#define BOOTCFG_DYNAMIC_FREE(_addr)         free(_addr)

#define BOOTCFG_WARNING(fmt, ...)           WARNING(fmt, ##__VA_ARGS__)
#define BOOTCFG_ERROR(fmt, ...)             ERR(fmt, ##__VA_ARGS__)
#define BOOTCFG_DEBUG(fmt, ...)             DEBUG(fmt, ##__VA_ARGS__)

LOCAL unsigned int get_crc32(unsigned char *buf, int len)
{
    if ( NULL == buf || 0 == len )
    {
        return 0;
    }
    return crc32(0, buf, len );
}

/*!
*\fn        INT32 bootCfgHeaderFormat(UINT8 *buf, UINT32 len)
*\brief     将buf初始化为BOOTCFG分区的格式，crc32值初始化为0
*\param[in] buf     待格式化的buf
*\param[in] len     buf的长度
*\return    OK, or ERROR if failed
*\retval    INT32
*/
LOCAL INT32 bootCfgHeaderFormat(UINT8 *buf, UINT32 len)
{
    BOOT_CONFIG_HEADER *pHeader = NULL;
    UINT32 partitionSize = getBlockMaxSize(BOOTCFG_BLOCK_INDEX);

    if (buf == NULL || len < sizeof(BOOT_CONFIG_HEADER) || partitionSize == 0)
    {
        return ERROR;
    }

    memset(buf, 0, len);
    pHeader = (BOOT_CONFIG_HEADER *)buf;
    pHeader->magic = htonl(BOOT_CONFIG_MAGIC);
    pHeader->crc32 = htonl(0);
    pHeader->partitionSize = htonl(partitionSize);
    pHeader->len = htonl(sizeof(BOOT_CONFIG_HEADER));

    return OK;
}

/*!
*\fn        BOOL checkBootCfgFormat(UINT8 *buf)
*\brief     检查buf是否为BOOTCFG分区的格式
*\param[in] buf     BOOTCFG分区的buf
*\return    TRUE or FALSE
*\retval    BOOL
*/
BOOL checkBootCfgFormat(UINT8 *buf)
{
    UINT32 partitionSize = getBlockMaxSize(BOOTCFG_BLOCK_INDEX);
    BOOT_CONFIG_HEADER *pHeader = NULL;
    UINT32 crcBackup = 0;
    UINT32 crcCalc = 0;

    if (buf == NULL || partitionSize == 0)
    {
        return FALSE;
    }

    pHeader = (BOOT_CONFIG_HEADER *)buf;
    if ((ntohl(pHeader->magic) != BOOT_CONFIG_MAGIC)
        || (ntohl(pHeader->partitionSize) != partitionSize)
        || (ntohl(pHeader->len) < sizeof(BOOT_CONFIG_HEADER) || ntohl(pHeader->len) > partitionSize))
    {
        BOOTCFG_WARNING("magic:%x size:%d len:%d\n", ntohl(pHeader->magic), ntohl(pHeader->partitionSize), ntohl(pHeader->len));
        return FALSE;
    }

    crcBackup = ntohl(pHeader->crc32);
    pHeader->crc32 = htonl(0);
    crcCalc = get_crc32(buf, ntohl(pHeader->len));
    pHeader->crc32 = htonl(crcBackup);

    if (crcCalc != crcBackup)
    {
        BOOTCFG_WARNING("crc error %x:%x\n", crcCalc, crcBackup);
        return FALSE;
    }

    return TRUE;
}

/*!
*\fn        INT32 getBootCfgValue(UINT16 cfgType, VOID *pData, UINT32 valueType, UINT32 maxSize)
*\brief     获取BOOTCFG分区的配置值
*\param[in] cfgType     待获取的配置类型
*\param[out]pData       待获取的配置值的地址
*\param[in] valueType   配置值的类型
*\param[in] maxSize     pDate的长度，仅当valueType=BOOT_TLV_TYPE_STRING类型时有用
*\return    OK, or ERROR if failed
*\retval    INT32
*/
INT32 getBootCfgValue(UINT16 cfgType, VOID *pData, UINT32 valueType, UINT32 maxSize)
{
    UINT32 len = getBlockMaxSize(BOOTCFG_BLOCK_INDEX);
    UINT8 *buf = NULL;
    BOOT_CONFIG_HEADER *pHeader = NULL;
    UINT8 *ptrTLV = NULL;
    INT32 ret = ERROR;

    if (len == 0)
    {
        return ERROR;
    }

    buf = BOOTCFG_DYNAMIC_MALLOC(len);
    if (buf == NULL)
    {
        BOOTCFG_ERROR("malloc%d fail\n", len);
        return ERROR;
    }

    if (readSpecifyBlock(BOOTCFG_BLOCK_INDEX, len, (char *)buf) == ERROR)
    {
        BOOTCFG_ERROR("readSpecifyBlock:%d fail\n", BOOTCFG_BLOCK_INDEX);
        goto exit;
    }

    if (checkBootCfgFormat(buf) == FALSE)
    {
        BOOTCFG_ERROR("checkBootCfgFormat fail\n");
        goto exit;
    }

    pHeader = (BOOT_CONFIG_HEADER *)buf;
    ptrTLV = pHeader->data;

    while (ptrTLV < buf + ntohl(pHeader->len))
    {
        if (getTLVType(ptrTLV) == cfgType)
        {
            switch (valueType)
            {
                case BOOT_TLV_TYPE_UINT8:
                    ret = getTLVUint8(ptrTLV, (UINT8 *)pData);
                    break;
                case BOOT_TLV_TYPE_UINT16:
                    ret = getTLVUint16(ptrTLV, (UINT16 *)pData);
                    break;
                case BOOT_TLV_TYPE_UINT32:
                    ret = getTLVUint32(ptrTLV, (UINT32 *)pData);
                    break;
                case BOOT_TLV_TYPE_UINT64:
                    ret = getTLVUint64(ptrTLV, (UINT64 *)pData);
                    break;
                case BOOT_TLV_TYPE_STRING:
                    ret = getTLVString(ptrTLV, (UINT8 *)pData, maxSize);
                    break;

                default:
                    break;
            }
            break;
        }
        ptrTLV += getTLVSpaceSize(ptrTLV);
    }

exit:
    if (buf)
        BOOTCFG_DYNAMIC_FREE(buf);
    return ret;
}

/*!
*\fn        BOOL checkBootCfgSpaceEnough(UINT8 *buf, UINT32 valueType, UINT32 strLen)
*\brief     检查BOOTCFG分区是否足够保存指定格式的数据
*\param[in] buf         BOOTCFG分区buf
*\param[in] valueType   配置值的类型
*\param[in] strLen      BOOT_TLV_TYPE_STRING类型时的字符串长度
*\return    TRUE or FALSE
*\retval    BOOL
*/
LOCAL BOOL checkBootCfgSpaceEnough(UINT8 *buf, UINT32 valueType, UINT32 strLen)
{
    BOOT_CONFIG_HEADER *pHeader = NULL;
    UINT32 remainLen = 0;
    UINT32 needLen = 0;

    if (buf == NULL)
    {
        return FALSE;
    }

    pHeader = (BOOT_CONFIG_HEADER *)buf;
    remainLen = ntohl(pHeader->partitionSize) - ntohl(pHeader->len);
    switch (valueType)
    {
        case BOOT_TLV_TYPE_UINT8:
            needLen = sizeof(UINT8);
            break;
        case BOOT_TLV_TYPE_UINT16:
            needLen = sizeof(UINT16);
            break;
        case BOOT_TLV_TYPE_UINT32:
            needLen = sizeof(UINT32);
            break;
        case BOOT_TLV_TYPE_UINT64:
            needLen = sizeof(UINT64);
            break;
        case BOOT_TLV_TYPE_STRING:
            needLen = strLen;
            break;

        default:
            return FALSE;
    }

    needLen = BOOT_CONFIG_TLV_TOTAL_LEN(needLen);
    if (needLen > remainLen)
    {
        return FALSE;
    }

    return TRUE;
}

/*!
*\fn        INT32 setBootCfgTlv(UINT8 *ptrTLV, UINT16 cfgType, VOID *pData, UINT32 valueType)
*\brief     将指定类型的数据设置到ptrTLV指向的TLV结构中
*\param[in] ptrTLV      TLV结构
*\param[in] cfgType     配置类型
*\param[in] pData       配置值的地址
*\param[in] valueType   配置值的类型
*\return    tlv length or ERROR if failed
*\retval    INT32
*/
LOCAL INT32 setBootCfgTlv(UINT8 *ptrTLV, UINT16 cfgType, VOID *pData, UINT32 valueType)
{
    if (ptrTLV == NULL || pData == NULL)
    {
        return ERROR;
    }

    switch (valueType)
    {
        case BOOT_TLV_TYPE_UINT8:
            return setTLVUint8(ptrTLV, cfgType, *((UINT8 *)pData));
        case BOOT_TLV_TYPE_UINT16:
            return setTLVUint16(ptrTLV, cfgType, *((UINT16 *)pData));
        case BOOT_TLV_TYPE_UINT32:
            return setTLVUint32(ptrTLV, cfgType, *((UINT32 *)pData));
        case BOOT_TLV_TYPE_UINT64:
            return setTLVUint64(ptrTLV, cfgType, *((UINT64 *)pData));
        case BOOT_TLV_TYPE_STRING:
            return setTLVString(ptrTLV, cfgType, (UINT8 *)pData);

        default:
            return ERROR;
    }

    return ERROR;
}

/*!
*\fn        INT32 addBootCfgValue(UINT8 *buf, UINT16 cfgType, VOID *pData, UINT32 valueType)
*\brief     将指定类型的数据设置到BOOTCFG分区的buf尾部，并更新BOOT_CONFIG_HEADER
*\param[in] buf         BOOTCFG分区的buf
*\param[in] cfgType     配置类型
*\param[in] pData       配置值的地址
*\param[in] valueType   配置值的类型
*\return    tlv length or ERROR if failed
*\retval    INT32
*/
LOCAL INT32 addBootCfgValue(UINT8 *buf, UINT16 cfgType, VOID *pData, UINT32 valueType)
{
    BOOT_CONFIG_HEADER *pHeader = NULL;
    UINT8 *ptrTLV = NULL;
    UINT32 length = 0;
    INT32 ret = ERROR;

    if (buf == NULL || pData == NULL)
    {
        return ERROR;
    }

    pHeader = (BOOT_CONFIG_HEADER *)buf;
    length = ntohl(pHeader->len);
    ptrTLV = buf + length;

    if (checkBootCfgSpaceEnough(buf, valueType, (valueType == BOOT_TLV_TYPE_STRING) ? strlen(pData) : 0))
    {
        BOOTCFG_DEBUG("cfg:%d Space enough\n", cfgType);
        ret = setBootCfgTlv(ptrTLV, cfgType, pData, valueType);
        if (ret != ERROR)
        {
            length += getTLVSpaceSize(ptrTLV);
            pHeader->len = htonl(length);
            pHeader->crc32 = htonl(0);
            pHeader->crc32 = htonl(get_crc32(buf, length));
        }
    }
    else
    {
        BOOTCFG_ERROR("cfg:%d Space not enough\n", valueType);
    }

    return ret;
}

/*!
*\fn        INT32 modifyBootCfgTlv(UINT8 *buf, UINT8 *ptrTLV, UINT16 cfgType, VOID *pData, UINT32 valueType)
*\brief     修改指定类型的数据，并更新BOOT_CONFIG_HEADER。对于BOOT_TLV_TYPE_STRING的类型先删除，再添加到结尾
*\param[in] buf         BOOTCFG分区的buf
*\param[in] ptrTLV      待修改的TLV结构
*\param[in] cfgType     配置类型
*\param[in] pData       配置值的地址
*\param[in] valueType   配置值的类型
*\return    tlv length or ERROR if failed
*\retval    INT32
*/
LOCAL INT32 modifyBootCfgTlv(UINT8 *buf, UINT8 *ptrTLV, UINT16 cfgType, VOID *pData, UINT32 valueType)
{
    BOOT_CONFIG_HEADER *pHeader = NULL;
    UINT32 alllength = 0;
    UINT32 oldLen = 0;
    UINT32 newLen = 0;
    UINT8 *oldBuf = NULL;
    INT32 ret = ERROR;

    if (buf == NULL || ptrTLV == NULL || pData == NULL)
    {
        return ERROR;
    }

    pHeader = (BOOT_CONFIG_HEADER *)buf;
    alllength = ntohl(pHeader->len);

    if (ptrTLV < pHeader->data || ptrTLV >= buf + alllength)
    {
        return ERROR;
    }

    oldLen = getTLVSpaceSize(ptrTLV);
    newLen = oldLen;
    
    if (valueType == BOOT_TLV_TYPE_STRING)
    {
        newLen = BOOT_CONFIG_TLV_TOTAL_LEN(strlen(pData));

        /* 如果长度不一样，将该TLV移至尾部，后续在末尾修改 */
        if (oldLen != newLen)
        {
            /* 剩余长度不足则报错 */
            if ((newLen > oldLen) && (ntohl(pHeader->partitionSize) - alllength < newLen - oldLen))
            {
                BOOTCFG_ERROR("cfg:%d size not enough, allLen=%d, need=%d\n", valueType, alllength, newLen - oldLen);
                return ERROR;
            }
            if ((oldBuf = BOOTCFG_DYNAMIC_MALLOC(oldLen)) == NULL)
            {
                BOOTCFG_ERROR("cfg:%d back up failed: malloc:%d failed\n", valueType, oldLen);
                return ERROR;
            }
            /* 先备份该TLV结构*/
            memcpy(oldBuf, ptrTLV, oldLen);

            /* 后面的TLV配置前移 */
            bcopy(ptrTLV + oldLen, ptrTLV, buf + alllength - ptrTLV - oldLen);

            /* 备份的数据还原到尾部 */
            ptrTLV = buf + alllength - oldLen;
            memcpy(ptrTLV, oldBuf, oldLen);
            BOOTCFG_DYNAMIC_FREE(oldBuf);

            /* 后续的setBootCfgTlv不会报错，之后还要计算CRC，这里不再更新CRC */
        }
    }

    ret = setBootCfgTlv(ptrTLV, cfgType, pData, valueType);
    if (ret != ERROR)
    {
        alllength += newLen;
        alllength -= oldLen;
        pHeader->len = htonl(alllength);
        pHeader->crc32 = htonl(0);
        pHeader->crc32 = htonl(get_crc32(buf, alllength));
    }

    return ret;
}

/*!
*\fn        INT32 setBootCfgValue(UINT16 cfgType, VOID *pData, UINT32 valueType)
*\brief     设置并写入BOOTCFG分区的配置
*\param[in] cfgType     配置类型
*\param[in] pData       配置值的地址
*\param[in] valueType   配置值的类型
*\return    tlv length or ERROR if failed
*\retval    INT32
*/
INT32 setBootCfgValue(UINT16 cfgType, VOID *pData, UINT32 valueType)
{
    UINT32 len = getBlockMaxSize(BOOTCFG_BLOCK_INDEX);
    UINT8 *buf = NULL;
    BOOT_CONFIG_HEADER *pHeader = NULL;
    UINT8 *ptrTLV = NULL;
    INT32 ret = ERROR;

    if (pData == NULL || len == 0)
    {
        BOOTCFG_ERROR("pDate is null, or no BOOTCFG block\n");
        return ERROR;
    }

    buf = BOOTCFG_DYNAMIC_MALLOC(len);
    if (buf == NULL)
    {
        BOOTCFG_ERROR("malloc%d fail\n", len);
        return ERROR;
    }

    if (readSpecifyBlock(BOOTCFG_BLOCK_INDEX, len, (char *)buf) == ERROR)
    {
        BOOTCFG_ERROR("readSpecifyBlock:%d fail\n", BOOTCFG_BLOCK_INDEX);
        goto exit;
    }

    pHeader = (BOOT_CONFIG_HEADER *)buf;

    if (checkBootCfgFormat(buf) == FALSE)
    {
        /*分区不合法则格式化分区，并写入配置*/
        if (bootCfgHeaderFormat(buf, len) == ERROR)
        {
            BOOTCFG_ERROR("format fail\n");
            goto exit;
        }

        ret = addBootCfgValue(buf, cfgType, pData, valueType);
        goto exit;
    }

    /*检查该配置是否存在，存在则替换，不存在则添加*/
    ptrTLV = pHeader->data;
    while (ptrTLV < buf + ntohl(pHeader->len))
    {
        if (getTLVType(ptrTLV) == cfgType)
        {
            break;
        }
        ptrTLV += getTLVSpaceSize(ptrTLV);
    }

    if (ptrTLV >= buf + ntohl(pHeader->len))
    {
        BOOTCFG_DEBUG("cfg:%d not exit\n", cfgType);
        ret = addBootCfgValue(buf, cfgType, pData, valueType);
    }
    else
    {
        BOOTCFG_DEBUG("cfg:%d exit\n", cfgType);
        ret = modifyBootCfgTlv(buf, ptrTLV, cfgType, pData, valueType);
    }

exit:
    if (ret != ERROR)
    {
        if (writeSpecifyBlock(BOOTCFG_BLOCK_INDEX, len, (char *)buf) ==  ERROR)
        {
            BOOTCFG_DEBUG("writeSpecifyBlock error\n");
            ret = ERROR;
        }
    }

    if (buf)
        BOOTCFG_DYNAMIC_FREE(buf);

    return ret;
}

/*!
*\fn        BLOCK_INDEX getCurrentFirmwareIndex(void)
*\brief     获取当前加载的FIRMWARE分区
*\return    FIRMWARE_BLOCK_INDEX or FIRMWARE2_BLOCK_INDEX
*\retval    BLOCK_INDEX
*/
BLOCK_INDEX getCurrentFirmwareIndex(void)
{
    UINT8 firmwareIndex = BOOT_CONFIG_FIRMWARE_DEF;

    getBootCfgValue(BOOT_CONFIG_TYPE_FIRMWARE, &firmwareIndex, BOOT_TLV_TYPE_UINT8, sizeof(firmwareIndex));

    if (firmwareIndex == BOOT_CONFIG_FIRMWARE_2)
    {
        return FIRMWARE2_BLOCK_INDEX;
    }
    else
    {
        return FIRMWARE_BLOCK_INDEX;
    }
}

/*!
*\fn        BLOCK_INDEX getNextFirmwareIndex(void)
*\brief     获取另一个FIRMWARE分区
*\return    FIRMWARE_BLOCK_INDEX or FIRMWARE2_BLOCK_INDEX
*\retval    BLOCK_INDEX
*/
BLOCK_INDEX getNextFirmwareIndex(void)
{
    UINT8 firmwareIndex = BOOT_CONFIG_FIRMWARE_DEF;

    getBootCfgValue(BOOT_CONFIG_TYPE_FIRMWARE, &firmwareIndex, BOOT_TLV_TYPE_UINT8, sizeof(firmwareIndex));

    if (firmwareIndex == BOOT_CONFIG_FIRMWARE_2)
    {
        return FIRMWARE_BLOCK_INDEX;
    }
    else
    {
        return FIRMWARE2_BLOCK_INDEX;
    }
}

/*!
*\fn        INT32 setCurrentFirmwareIndex(BLOCK_INDEX index)
*\brief     设置指定的FIRMWARE到BOOTCFG分区
*\param[in] index       FIRMWARE_BLOCK_INDEX or FIRMWARE2_BLOCK_INDEX
*\return    config length, or ERROR if failed
*\retval    INT32
*/
INT32 setCurrentFirmwareIndex(BLOCK_INDEX index)
{
    UINT8 value = 0;

    if (index == FIRMWARE_BLOCK_INDEX)
    {
        value = BOOT_CONFIG_FIRMWARE_1;
    }
    else if (index == FIRMWARE2_BLOCK_INDEX)
    {
        value = BOOT_CONFIG_FIRMWARE_2;
    }
    else
    {
        return ERROR;
    }

    return setBootCfgValue(BOOT_CONFIG_TYPE_FIRMWARE, &value, BOOT_TLV_TYPE_UINT8);
}

