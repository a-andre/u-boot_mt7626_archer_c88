/*
 *	Copyright 1994, 1995, 2000 Neil Russell.
 *	(See License)
 *	Copyright 2000, 2001 DENX Software Engineering, Wolfgang Denk, wd@denx.de
 */

#include "http_upgrade.h"
#include <command.h>
#include <net.h>
#include <asm/byteorder.h>
#include "lib_uip.h"
#include "rsaVerify.h"
#include "md5.h"
#include "flash_ops.h"


DECLARE_GLOBAL_DATA_PTR;



#define MD5SUM_LEN	16

/* specified in config.in */
#if 0
#define CONFIG_DEFAULT_PRODUCT_ID 0x0c800001
#endif
#define DEFAULT_PRODUCT_ID CONFIG_DEFAULT_PRODUCT_ID

#define MAC_ADDR_LEN	6

#define HTTP_DEBUG(fmt, ...) printf(fmt"\n", ##__VA_ARGS__)


#define TP_OEM_ID 		0x00
#define MECURY_OEM_ID 	0x01
#define FAST_OEM_ID 	0x02

#define RSA_KEY_LEN				128

#define FIRM_ALIGN_SIZE			0x1000

#define ALIGN_SIZE_BIG(_length, _padding) ((_length + _padding - 1) & (~(_padding - 1)))
#define ALIGN_SIZE_SMALL(_length, _padding) ((_length) & (~(_padding - 1)))

#define CONTEXT void

#define MD5_DIGEST_LEN		16		/* length of MD5 digest result */

/* Firmware information structure. */
typedef struct _FW_INFO
{
	unsigned char* buf;
	int size;
	unsigned char *fwAddr;
	int fwSize;
}FW_INFO;



/* Local uboot infomation structure. */
typedef struct _LOCAL_INFO
{
	uint8_t mac[6];
	uint8_t padding[2];
	uint32_t productId;
	uint32_t sid;
	uint32_t cid;
} LOCAL_INFO;

enum MD_ALGORITHM
{
	MD4 = 0x04,
	MD5 = 0x05
};

enum MD_TRANSFORM
{
	MD_TRANSFORM_NORMAL = 0,
	MD_TRANSFORM_SWAP_1 = 1,	/* 处理block时，4字节为单位进行前后翻转 */
};

typedef enum _TP_ERROR
{
	ENONE = 0,						/* 没有错误 */
	ENOMEMORY,						/* 内存不足 */
	EINVARG,						/* 参数错误 */
	EINVFMT,						/* 格式错误 */
	EINVEVT,						/* 不支持的事件 */
	EINVCODE,
	EFORBID,						/* 禁止的操作。 */
	EUNAUTH,						/* 认证失败。 */
	EOVERFLOW,
	EINVINSTRUCT,					/* 不支持的指令 */
	EMD5,							/* MD5校验失败 */
	EDESENCODE,						/* DES加密失败 */
	EDESDECODE,						/* DES解密失败 */
	ECHIPID,						/* 不支持的芯片类型； */
	EFLASHID,						/* 不支持的FLASH类型； */
	EPRODID,						/* 不支持的产品型号； */
	ELANGID,						/* 不支持的语言； */
	ESUBVER,						/* 不支持子版本号； */
	EOEMID,							/* 不支持的OEM类型； */
	ECOUNTRYID,						/* 不支持的国家； */
	ECODE,							/* 不支持的操作类型； */
	EWANTYPE,						/* 不支持的WAN口接入类型； */
	ETOOLONG,						/* 数据过长。 */
	ESYSTEM,						/* 系统错误。 */
	ENOECHO,						/* 超时无响应。 */
	ENODEVICE,						/* 找不到设备。 */
	EINVIP,							/* IP地址不正确。 */
	EINVMASK,						/* 掩码不正确。 */
	EINVGTW,						/* 网关不正确。 */
	EINVIPMASKPAIR,					/* IP和掩码不匹配。 */
	EGTWUNREACH,					/* 网关不可达。 */
	EINVMTU,						/* MTU错误 */
	EINVMACFMT,						/* MAC地址格式不正确。 */
	EENTRYEXIST,					/* 条目已存在。 */
	EENTRYNOTEXIST,					/* 条目不存在。 */
	EENTRYCONFLIC,					/* 条目冲突。 */
	ETABLEFULL,						/* 表满。 */
	ETABLEEMPTY,					/* 表空 */
	EINVPORT,						/* 超出端口范围*/
	EPORTRESERVED,					/* 端口冲突*/
	EINVPTC,						/* 不支持的协议类型。 */
	ECOMFLICTNET,					/* 网段冲突*/
	EINVNET,						/* 非法的网段 */
	EINVTYPE,						/* 非法的类型。 */
	EINVMODE,						/* 非法的模式。 */
	EINVTIME,
	EINVFDNSVR,						/* 非法的首选DNS */
	EINVSDNSVR,						/* 非法的备选DNS */
	EINVDATA,						/* 数据合法性验证失败 */
	EINVLEASETIME,					/* 非法的地址租期。 */
	EINVADDRPOOL,					/* 非法的地址池。 */
	EINVDATE,						/* 非法的日期 */
	EINVTIMEZONE,					/* 非法的时区 */
	ENOLINK,						/* WAN口未链接 */
	ESYSBUSY,						/* 系统繁忙。 */
	EINVNUM,
	EINVSIZE,
	EINVTIMEOUT,
	EINVMETRIC,
	EINVINTERVAL,					/* 时间间隔输入错误 */
	EINVBOOL = 69,					/* 布尔类型的取值只能是0或者1 */
	EINVSSIDLEN,					/* 无线SSID长度不合法 */
	EINVSECAUTH,					/* 无线安全设置的认证类型错误 */
	EINVWEPAUTH,					/* WEP认证类型错误 */
	EINVRADIUSAUTH,					/* RADIUS认证类型错误 */
	EINVPSKAUTH,					/* PSK认证类型错误 */
	EINVCIPHER,						/* 加密算法错误 */
	EINVRADIUSLEN,					/* radius密钥短语长度错误 */
	EINVPSKLEN,						/* psk密钥短语错误 */
	EINVGKUPINTVAL,					/* 组密钥更新周期错误 */
	EINVWEPKEYTYPE,					/* WEP密钥类型错误 */
	EINVWEPKEYIDX,					/* 默认WEP密钥索引错误, 80 */
	EINVWEPKEYLEN,					/* WEP密钥长度错误 */
	EINVACLDESCLEN,					/* MAC地址过滤条目描述信息长度错误 */
	EINVWPSPINLEN,					/* WPS PIN码长度错误 */
	EINVAPMODE,						/* 无线设备工作模式错误 */
	EINVWLSMODE,					/* 无线速率模式(bgn)错误 */
	EINVREGIONIDX,					/* 无线国家码错误 */
	EINVCHANWIDTH,					/* 频段带宽错误 */
	EINVRTSTHRSHLD,					/* 无线RTS阈值错误 */
	EINVFRAGTHRSHLD,				/* 无线分片阈值错误 */
	EINVBCNINTVL,					/* 无线beacon间隔错误, 90 */
	EINVTXPWR,						/* 无线Tx功率错误 */
	EINVDTIMINTVL,					/* 无线DTIM周期错误 */
	EINVWDSAUTH,					/* 无线WDS认证类型错误 */
	EINVA34DETECT,					/* 3/4地址格式配置错误 */
	EINVWLANPWD,					/* 无线密钥包含非法字符 */
	EINVHOSTNAMELEN,				/* 非法的主机名长度 */
	EINVGETIMEOUT,					/* 非法的访客网络超时时间 */
	EINVGETIMEMODE,					/* 非法的访客网络定时模式 */
	EINVMACGROUP,					/* MAC地址为组播地址 */
	ENAMEBLANK,						/* 用户名输入为空 */
	EPWDBLANK,						/* 密码输入为空 */
	EINVMACZERO,					/* MAC地址为全0 */
	EINVMACBROAD,					/* 广播MAC地址 */
	EHOSTNAMEEMP,					/* 受控主机名为空 */
	EOBJNAMEEMP,					/* 访问目标名为空 */
	EPLANNAMEEMP,					/* 日程计划名为空 */
	EOBJDOMAINALLEMP,				/* 访问目标域名全为空 */
	EREFERED,						/* 条目被关联了 */
	EDELPARTIAL,					/* 只删除了部分条目 */
	EDELNOTHING,					/* 一个条目都没有删除 */
	ERSACHECK,						/* RSA校验错误 */
	EINVLGPWDLEN,					/* 登录密码长度不合法 */
	EINLGVALCHAR,					/* 登录密码含有非法字符 */
	EINLGVALOLDSAME,				/* 新登录密码和旧登录密码一样 */
	EINVNETID,						/* 网络号全0或者1 */
	EINVHOSTID,						/* 超出范围 */
	EOUTOFRANGE,					/* RSA校验错误 */
	EINDOMAIN,						/* 非法的域名 */
	ELACKCFGINFO,					/* 缺少必要的配置信息 */
	EINVKEY,						/* 旧的登录密码错误 */	
	EINVRMTPORT,					/* 远程管理端口超出范围*/
	EILLEGALPORT,					/* 端口值非法 */
	EINVNAMELEN,					/* 用户名长度超出范围 */
	EINVPWDLEN,						/* 密码长度超出范围 */
	EINVNAME,						/* 用户名非法 */
	ENOTLANSUBNET,					/* 不是LAN网段IP */
	EHOSTALLEMPTY,					/* 受控主机IP全为空 */
	EOBJALLEMPTY,					/* 访问目标IP和端口全为空 */
	EINVGROUPIP,					/* 组播的IP地址 */
	EINVLOOPIP,						/* 回环的IP地址 */
	EINVIPFMT,						/* IP地址格式错误 */
	ENOTLANWANNET,					/* 网段不是LAN或WAN */
	ELANSUBNET,						/* LAN网段IP */
	EINVPWD,						/* 密码非法 */
	EIPRESERVED,					/* IP地址被占用 */
	EINVPORTFMT,					/* 端口格式错误 */
	EADDRPOOLNOTLANSUBNET,			/* 地址池不在LAN网段 */
	ERULECONFLICT,					/* 受控规则冲突 */
	EINVTIMETYPE,					/* 非法的时间设置方式 */
	EINDATE,						/* 非法日期 */
	EIPMACCONFLIC,					/* 添加的条目与IP&MAC绑定冲突 */
	EDHCPRESERVECONFLIC				/* 添加的条目与DHCP RESERVE冲突 */
}TP_ERROR;

static LOCAL_INFO localInfo = {{0}};
static FW_INFO fw_info = {0};



LOCAL UINT8 md5Key[MD5_DIGEST_LEN] = {0xCC, 0x96, 0x28, 0xEE, 0x8D, 0xFB, 0x21, 0xBB, 0x3D, 0xEF, 0x6C, 0xB5, 0x9F, 0x77, 0x4C, 0x7C};



const UINT8 ZERO_MAC[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const UINT8 FF_MAC[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
const UINT8 DEF_MAC[] = {0x00, 0x19, 0x66, 0xca, 0x8b, 0x07};

void get_eth_addr(uint8_t* addr)
{
	memcpy(addr, localInfo.mac, MAC_ADDR_LEN);

	if ((memcmp(addr, ZERO_MAC, MAC_ADDR_LEN) == 0) || (memcmp(addr, FF_MAC, MAC_ADDR_LEN) == 0))
	{
		printf("read mac invalid, use defalt mac\n");
		memcpy(addr, DEF_MAC, MAC_ADDR_LEN);
	}
}

/*
 * Function Name: localInfoInit
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Initialize local firmware information(currently only HwID).
 * Parameter:
 *       VOID
 * return:
 *       0: Succeeded;
 *       ERR_GENERIC: No HwID found.
 *       ERR_READ_FLASH: Read command execution error.
 *       ERR_BAD_ADDRESS: Invalid address values passed. 
 */
static int localInfoInit(void)
{
	int ret = 0;
	PROFILE *buf = NULL;
	uint8_t *p = NULL;
	
	memset(&localInfo, 0, sizeof(localInfo));

	buf = (PROFILE*)malloc(FACTORY_INFO_LEN);
	if (!buf)
	{
		ERR("malloc failed.");
		ret = ERR_MEMORY_ALLOC;
		goto out;
	}

	ret = readFlash(UIP_FLASH_BASE + FACTORY_INFO_OFFSET, buf, FACTORY_INFO_LEN);
	if (ret < 0)
	{
		ERR("read flash failed.");
		goto out_free_buf;
	}

#ifdef RECOVERY_UPGRADE_UNIT_TEST
	{
		int i;
		DBG_UNIT("content:");
		for (i=0, p = buf; i < FACTORY_INFO_LEN; p++, i++)
		{
			if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))
			{
				printf("%c ", *p);
			}
			else
			{
				printf("%02X ", (int)*p);
			}
		}
		printf("\n");
	}
#endif
	memcpy(localInfo.mac, buf->mac[0], MAC_ADDR_LEN);

	localInfo.productId = DEFAULT_PRODUCT_ID;

	memcpy((uint8_t *)&localInfo.sid, buf->SID, 4);
	
	localInfo.cid = 0;
	memcpy((uint8_t *)&localInfo.cid, buf->CID, 2);
#ifdef RECOVERY_UPGRADE_UNIT_TEST
	{
		int i;
		DBG_UNIT("got MAC:");
		for(i=0; i<6; i++)
		{
			printf("%02X ",localInfo.mac[i]);
		}
		printf("\n");		
		DBG_UNIT("got productID:");
		
		printf("%04X ",localInfo.productId);
		
		printf("\n");
	}
#endif

out_free_buf:
	free(buf);

out:
	return ret;
}



/*
 * Function Name: calcMd5
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Compute MD5 checksum.
 * Parameter:
 *       data: Source data .
 *       size: Source data length.
 *       md5: return the result md5 value.
 * return:
 *       VOID
 */
static void calcMd5(char *data, int size, char *md5)
{
	MD5_CTX ctx;

	MD5_Init(&ctx);
	MD5_Update(&ctx, data, size);
	MD5_Final(md5, &ctx);
}


/*!
	@brief		检查FIRMWARE是否合法
	@note		检查头部带有的MD5扩展数据
	@param      fwOff  flash中固件所在分区的偏移
	@param      fwSize flash中固件所在分区的大小
*/
int _validateLocalFirmware(UINT32 fwOff, UINT32 fwSize)
{
	int ret = 0;
    IMG_HEADER* header = (IMG_HEADER*)(UIP_FLASH_BASE + fwOff);
	UINT8 md5[MD5_DIGEST_LEN];
	UINT8 oriMd5[MD5_DIGEST_LEN];
#ifdef WEBFAILSAFE_LOCAL_VALIDATION_IN_RAM
	//copy flash image to RAM.
	DBG("copying flash to 0x%x", WEBFAILSAFE_UPLOAD_RAM_ADDRESS);
	ret = readFlash(UIP_FLASH_BASE + fwOff, WEBFAILSAFE_UPLOAD_RAM_ADDRESS, fwSize);
	if (ret < 0)
	{
		ERR("reading flash to RAM failed.");
		return ret;
	}

	header = (IMG_HEADER*)WEBFAILSAFE_UPLOAD_RAM_ADDRESS;
#endif
	
	DBG_UNIT("validate local firmware...\nTP Header at %p", header);

#ifdef UIP_CACHE_OPERATION
	uip_cache_enable();
#endif
	/* 需要考虑头部不正确的情况，目前先简单的根据内容判断，后续考虑是否加入CRC或MD5 */
	/* 检查整个firmware长度的合法性   */
	if ((ntohl(header->len) > fwSize) || (ntohl(header->len) < sizeof(IMG_HEADER)))
	{
		printf("firmware len error!\n");
		return -1;
	}
	/* 检查md5 offest合法性 */
	if ((ntohl(header->file[IMG_FILE_FIRMMD5_INDEX].fileOffset) > ntohl(header->len))
		|| (ntohl(header->file[IMG_FILE_FIRMMD5_INDEX].fileOffset) < sizeof(IMG_HEADER)))
	{
		printf("md5 offset error!\n");
		return -1;
	}
	int md5Addr = (UINT8 *)header + ntohl(header->file[IMG_FILE_FIRMMD5_INDEX].fileOffset);
	memcpy(oriMd5, md5Addr, MD5_DIGEST_LEN);
	memset(md5Addr, 0, MD5_DIGEST_LEN);
	calcMd5((UINT8 *)header, ntohl(header->len), md5);
	if (0 == memcmp(md5, oriMd5, MD5_DIGEST_LEN))
	{
		return 0;
	}
#ifdef UIP_CACHE_OPERATION
	uip_cache_disable();
#endif


	
	return -1;
}

/*!
	@brief		检查本地的固件是否合法
	@note		检查头部带有的MD5扩展数据
*/
int validateLocalFirmware()
{
#ifdef CONFIG_DUAL_IMAGE
        BLOCK_INDEX block = getCurrentFirmwareIndex();
        int fwOff = getBlockAddr(block);
        int fwLen = getBlockMaxSize(block);
#else
        int fwOff = FIRMWARE_OFFSET;
        int fwLen = FIRMWARE_LEN;
#endif

    if (_validateLocalFirmware(fwOff, fwLen) < 0)
    {
#ifdef CONFIG_DUAL_IMAGE
        /* 查看另一镜像是否有效，如果有效，重写BOOTCFG分区的FIRMWARE配置 */
        block = getNextFirmwareIndex();
        fwOff = getBlockAddr(block);
        fwLen = getBlockMaxSize(block);

        if (_validateLocalFirmware(fwOff, fwLen) == 0)
        {
            setCurrentFirmwareIndex(block);
            return 0;
        }
        else
#endif
        {
            return -1;
        }
    }

    return 0;
}


/* 将内存src的内容设置到固件内容地址dst中 */
void *httpUpgrade_memsetdata(CONTEXT *context, void *dst, const void *src, size_t size)
{
#ifdef SUPPORT_LOW_MEM_UPGRADE

	if (context && context->memUnavaiInUpgrade)
	{

		char **pointer = (char **)&src;
		/* 将内存src复制到dst */
		return operateDataInFirstMemBlockAndNetPool(
			context->content, 
			context->contentLen, 
			context->mblkListInUpgrade, 
			dst, 
			size,
			http_memsetdata_cb,
			pointer,
			NULL,
			NULL
			);
	}
#endif
	return memcpy(dst, src, size);
}

/* 将内存src的内容设置到固件内容地址dst中 */
void *httpUpgrade_memset(CONTEXT *context, void *dst, int ch, size_t size)
{
#ifdef SUPPORT_LOW_MEM_UPGRADE
	if (context && context->memUnavaiInUpgrade)
	{

		UINT8 chx = ch;
		return operateDataInFirstMemBlockAndNetPool(
			context->content, 
			context->contentLen, 
			context->mblkListInUpgrade, 
			dst, 
			size,
			http_memset_cb,
			ch, 
			NULL,
			NULL
			);
	}
#endif
	return memset(dst, ch, size);
}

BOOL isRsaCheckFailPermit(void)
{
	return 1;
}

/* 从固件内容地址src中读取数据到内存dst */
void *httpUpgrade_memget(CONTEXT *context, void *dst, const void *src, size_t size)
{
#ifdef SUPPORT_LOW_MEM_UPGRADE

	if (context && context->memUnavaiInUpgrade)
	{

		char **pointer = (char **)&dst;
		return operateDataInFirstMemBlockAndNetPool(
			context->content, 
			context->contentLen, 
			context->mblkListInUpgrade, 
			src, 
			size,
			http_memget_cb,
			pointer,
			NULL,
			NULL
			);
	}
#endif
	return memcpy(dst, src, size);
}
void httpUpgrade_MD_string(CONTEXT *context, BYTE *bptr_string, ULONG length, BYTE *bptr_digest, enum MD_ALGORITHM algorithm, enum MD_TRANSFORM transform)
{
#ifdef SUPPORT_LOW_MEM_UPGRADE

	if (context && context->memUnavaiInUpgrade)
	{

		return MD_string_withNetPool(
			context->content, 
			context->contentLen, 
			context->mblkListInUpgrade, 
			bptr_string, 
			length,
			bptr_digest,
			algorithm,
			transform
			);
	}
#endif
	return calcMd5(bptr_string, length, bptr_digest);

}
void httpUpgrade_writeSpecifyBlock(CONTEXT *context, BLOCK_INDEX index, UINT32 len, char *buf)
{
#ifdef SUPPORT_LOW_MEM_UPGRADE

	if (context && context->memUnavaiInUpgrade)
	{

		return writeSpecifyBlockWithMemAndNetPool(
			context, 
			index,
			len,
			buf
			);
	}
#endif
	writeSpecifyBlock(index, len, buf);
}



/*!
	@brief			升级限制条目
*/
typedef struct
{
	UINT32 productId;			/*!<	product id, if 0 don't check */
	UINT32 SID;					/*!<	SID, if 0 don't check */
	UINT32 CID;					/*!<	CID, if 0 don't check */
	UINT32 reserved[7];			/*!<	reserved */
}
SUPPORT_LIST_ENTRY;

LOCAL BOOL checkSupportListEntry(SUPPORT_LIST_ENTRY *entry)
{
#if 0
	DEVICE device = {0};

	modelRead(DEVICE_DATA_ID, &device);
#endif
	
	if (ntohl(entry->productId) != 0)
	{
		if (localInfo.productId != ntohl(entry->productId))
		{
			printf("productid not match, 0x%x:0x%x\n", localInfo.productId, ntohl(entry->productId));
			return FALSE;
		}
	}

	if (ntohl(entry->SID) != 0)
	{
		if (0 != memcmp((UINT8 *)&localInfo.sid, (UINT8*)&entry->SID, MT_B_SID_LEN))
		{
			printf("SID not match, 0x%x:0x%x\n", (localInfo.sid), (entry->SID));
			return FALSE;
		}
	}

	if (ntohl(entry->CID) != 0)
	{
		if (0 != memcmp((UINT8 *)&localInfo.cid, (UINT8*)&entry->CID, MT_B_CID_LEN))
		{
			printf("CID not match, 0x%hx:0x%hx\n", localInfo.cid, entry->CID);
			return FALSE;
		}
	}	

	return TRUE;
}

/* 
	请注意，由于需要支持844的升级，读取升级文件的内存时，需要使用httpUpgrade_memget函数；
	此处传进来的pHdr和pVxImgFile不在连续的内存上
 */
LOCAL BOOL checkSupportList(CONTEXT *context, char *pVxImgFile, IMG_HEADER *pHdr)
{
	int supportListLen = ntohl(pHdr->file[IMG_FILE_SUPPORTLIST_INDEX].fileSize);

	/* length not valid, quit */
	if (supportListLen == 0)
	{
		return FALSE;
	}
	
	SUPPORT_LIST_ENTRY entry = {0};

	char *supportListStart =((char *)pVxImgFile + ntohl(pHdr->file[IMG_FILE_SUPPORTLIST_INDEX].fileOffset));
	
	char *curr = supportListStart;
	
	while (curr < ((char *)supportListStart + supportListLen))
	{
		memset(&entry, 0, sizeof(entry));
		httpUpgrade_memget(context, &entry, curr, sizeof(SUPPORT_LIST_ENTRY));
		if (TRUE == checkSupportListEntry(&entry))
			return TRUE;

		curr += sizeof(entry);
	}

	return FALSE;
}

LOCAL UINT32 getFlashImgHeaderMagic(void)
{
	return 0;
}

LOCAL TP_ERROR httpChkImgHeader(CONTEXT *context, char *pVxImgFile, IMG_HEADER *pHdr, BOOL *shouldResetConfig)
{
	TP_ERROR ret = ENONE;
	#if 0
	DEVICE device;
	modelRead(DEVICE_DATA_ID, &device);
	#endif
	/* REVIEW: 这里的IMG_COMMON_MAGIC是什么？如果是这个magic就不用做任何检查？ */
	if ((IMG_COMMON_MAGIC == getFlashImgHeaderMagic()) || (IMG_COMMON_MAGIC == ntohl(pHdr->magic)))
	{
		return ENONE;
	}
	/* uboot中flash header可能处于破坏状态 */
#if 0
	if (getFlashImgHeaderMagic() != ntohl(pHdr->magic))
	{
		HTTP_DEBUG("incorrect magic number 0x%x, should be 0x%x!", 
					ntohl(pHdr->magic), getFlashImgHeaderMagic());
		return EINVFMT;
	}
#endif
	/* 如果有supportList，使用supportList判断；否则就使用老的productid判断方式 */
	if(ntohl(pHdr->file[IMG_FILE_SUPPORTLIST_INDEX].fileOffset) != 0)
	{
		if (!checkSupportList(context, pVxImgFile, pHdr))
			return EPRODID;
	}
	else 
	{
		if (localInfo.productId != ntohl(pHdr->prodid))
		{
			HTTP_DEBUG("incorrect product ID 0x%x, should be 0x%x!", 
						ntohl(pHdr->prodid), localInfo.productId); 
			return EPRODID;
		}
	}
	/* uboot中flash header可能处于破坏状态 */
#if 0
	/* 第4个版本号我们不用，但还是需要和之前一致 */
	if ((device.mainVer & 0xFF000000) != (ntohl(pHdr->mainVersion) & 0xFF000000))
	{
		HTTP_DEBUG("incorrect main version 0x%x, should be 0x%x!", 
				ntohl(pHdr->mainVersion) & 0xFF000000, device.prodId & 0xFF000000); 
		return ESUBVER;
	}

	/* 高版本号和中版本号必须不小于现有版本 */
	/* 低版本号变更允许互升 */
	/* 如果高版本号和中版本号变化，需恢复出厂 */
	if ((device.mainVer & 0x00FFFF00) > (ntohl(pHdr->mainVersion) & 0x00FFFF00))
	{
		HTTP_DEBUG("main version 0x%x is smaller than 0x%x!", 
				ntohl(pHdr->mainVersion) & 0x00FFFF00, device.mainVer & 0x00FFFF00); 
		return ESUBVER;
	}
	
	if ((device.mainVer & 0x00FFFF00) < (ntohl(pHdr->mainVersion) & 0x00FFFF00))
	{
		HTTP_DEBUG("upgrading firmware's main version(middle, high) 0x%x is bigger than current 0x%x, should perform config reset", 
				ntohl(pHdr->mainVersion) & 0x00FFFF00, device.mainVer & 0x00FFFF00); 
		if (shouldResetConfig)
			*shouldResetConfig = TRUE;
	}

	/* 目前子版本号暂未使用，默认为1 */

	/* oemId主要用于标示使用哪一个rsa key，有TP MECURY FAST. 如果公钥使用不正确，会导致签名校验失败。当前使用MERCURY，所以为1 */
	if ((device.oemId != 0) && (device.oemId != ntohl(pHdr->oemId)))
	{
		HTTP_DEBUG("incorrect oem id 0x%x, should be 0x%x!", 
				ntohl(pHdr->oemId), device.oemId);
		return EOEMID;
	}
#endif
	return ENONE;
}

TP_ERROR serialFirmware(CONTEXT *context, char *pVxImgFile, int nVxImgFileSize)
{
	IMG_HEADER *pHeader = NULL;
	TP_ERROR tpError = ENONE;

	IMG_HEADER imgHeader;
	
	if ((NULL == pVxImgFile))
	{
		return EINVFMT;
	}


	httpUpgrade_memget(context, &imgHeader, pVxImgFile, sizeof(IMG_HEADER));
	pHeader = &imgHeader;
	
	/* 注释掉任务锁，此处会导致无线Beacon长时间无法发送 */
	/* taskLock(); */
#if 0
	gpioLedEventNotify(GPIO_EVENT_SYSTEM_UPGRADE);
#endif
	/* 升级uboot */
	if (pHeader->includeBoot)
	{
		int ubootOffset = ntohl(pHeader->file[IMG_FILE_BOOT_INDEX].fileOffset);
		int ubootSize = ntohl(pHeader->file[IMG_FILE_BOOT_INDEX].fileSize);
		if (ubootOffset == 0)
		{
			/* 老的代码的做法，不灵活，升级文件本身应自洽，不依赖于本地分区表 */
			ubootOffset = sizeof(IMG_HEADER);
			ubootSize = getBlockMaxSize(BOOTIMG_BLOCK_INDEX);
		}

		
		
		pVxImgFile += ubootOffset;
		nVxImgFileSize -= ubootOffset;
		printf("upgrading boot partition...\n");
		httpUpgrade_writeSpecifyBlock(context, BOOTIMG_BLOCK_INDEX, ALIGN_SIZE_BIG(ubootSize, FIRM_ALIGN_SIZE), pVxImgFile);

		pVxImgFile += ubootSize;
		nVxImgFileSize -= ubootSize;

		/* 重新获取header */
		httpUpgrade_memget(context, 
			&imgHeader, 
			(pVxImgFile), 
			sizeof(IMG_HEADER));

		pHeader = &imgHeader;
	}

	/* 升级vxWorks.bin对应的压缩程序 */
	int firmSize = ntohl(pHeader->len);
	printf("upgrading firmware partition...\n");
	httpUpgrade_writeSpecifyBlock(context, FIRMWARE_BLOCK_INDEX, ALIGN_SIZE_BIG(firmSize, FIRM_ALIGN_SIZE), pVxImgFile);
#ifdef CONFIG_DUAL_IMAGE
    httpUpgrade_writeSpecifyBlock(context, FIRMWARE2_BLOCK_INDEX, ALIGN_SIZE_BIG(firmSize, FIRM_ALIGN_SIZE), pVxImgFile);
#endif

	/* taskUnlock(); */

	return ENONE;
}


/*!
	@note:
	从浏览器拿到的升级文件构成：
	HTTP_FILE_HEADER + (IMG_HEADER) +(UBOOT) + IMG_HEADER + FIRMWARE
	UBOOT不是必须的；如果有UBOOT有，则必须带一个IMG_HEADER；
	
	HTTP_FILE_HEADER：
	md5保护从HTTP_FILE_HEADER到FIRMWARE整个升级文件，计算时，需要先备份md5，将md5填入MD5KEY，计算出的整个文件的MD5再和备份的md5对比
	
	uboot: 
	如果带，则第一个header的includeBoot会指明为1；
	uboot的长度，老的做法是根据本机的分区表获取，新的代码中根据header的file参数获取，但新代码中也兼容原来的做法
	
	IMG_HEADER：img头部
	该头部需要和现有flash中的img头部对比做具体检查，包括magic、proid等要一致；

	firmware：vxlzma+minifs+rsa等，具体位置在file属性中

	minifs：
	rsa签名：
	原始数据是升级文件（不包括HTTP_FILE_HEADER）的MD5，然后使用rsa私钥加密作为签名。


	uboot（不包括img头）、firmware（包括img头）会被烧写到FLASH中。
*/

/*!
	@note
	双uboot升级逻辑：
	在双uboot中，第一个uboot是老代码中的uboot，和firmware是分开的；第二个uboot是firmware的一部分；
	因此可以兼容原有的做法。


*/
TP_ERROR httpCheckFirmware(CONTEXT *context, char *content, UINT32 length, UINT8 **firmwareAddr, UINT32 *firmwareLen, BOOL *shouldResetConfig)
{
	/* MD5校验*/
	UINT8 calcMd5[MD5_DIGEST_LEN] = {0};
	char rsaSignFile[RSA_KEY_LEN] = {0};

	/* 升级内容（如果有uboot则包括uboot）的起始地址 */
	char *pVxImgFile = NULL;
	/* 升级内容的长度 */
	int nVxImgFileSize = NULL;
	UINT32 headLen = 0;
	
	TP_ERROR tpError = ENONE;

	/* 镜像头部指针，可能是uboot的头部也可能是vx的头部 */
	IMG_HEADER *pVxHeader = NULL;

	BOOL signCheckOk = FALSE;
	#if 0
	DEVICE devData;
	UINT32 rsaKeyIndex = FAST_OEM_ID;
	#else
	UINT32 rsaKeyIndex = MECURY_OEM_ID;
	#endif
	/* 镜像头部 */
	IMG_HEADER imgHeader;

	/* 镜像指针 */
	char *pVxHeaderPointer = NULL;

	/* http文件头部 */
	HTTP_FILE_HEADER httpFileHeader;

	/* 请务必注意：不要让这些检查超过content+length的范围 */
	/* 
		检查http文件 
		http文件组成：HTTP_FILE_HEADER IMG_HEADER content
		长度位于IMG_HEADER中，所以需要首先读出两个头部
	*/

	/* 内存长度必须大于两个头部之和 */
	headLen = sizeof(HTTP_FILE_HEADER) + sizeof(IMG_HEADER);
	if ((NULL == content) || (length < headLen))
	{
		return EINVFMT;
	}

	/* 升级文件起始地址 */
	pVxImgFile = content;

	/* 获取镜像头部    */
	httpUpgrade_memget(context, &imgHeader, pVxImgFile + sizeof(HTTP_FILE_HEADER), sizeof(IMG_HEADER));
	pVxHeader = &imgHeader;

	/* 实际固件长度不能超过内存长度  */
	if (ntohl(pVxHeader->len) > (length - sizeof(HTTP_FILE_HEADER)))
	{
		return EINVFMT;
	}

	/* 升级文件长度，包括HTTP文件头部和实际固件（包括镜像头部) */
	nVxImgFileSize = ntohl(pVxHeader->len) + sizeof(HTTP_FILE_HEADER);

	/* 获取HTTP文件头部 */
	httpUpgrade_memget(context, &httpFileHeader, pVxImgFile, sizeof(HTTP_FILE_HEADER));

	
	/* 对整个升级文件进行校验 */
	/* 将HTTP文件头部中的MD5设置为密钥内容，然后再计算MD5*/
	httpUpgrade_memsetdata(context, pVxImgFile + sizeof(httpFileHeader.magic), md5Key, MD5_DIGEST_LEN);


	httpUpgrade_MD_string(context, pVxImgFile, nVxImgFileSize, calcMd5, MD5, MD_TRANSFORM_NORMAL);

	
	/* compare the checksum */
	if (memcmp(httpFileHeader.md5, calcMd5, MD5_DIGEST_LEN) != 0)
	{
		HTTP_DEBUG("md5 checksum is not correct!");
		return EMD5;
	}

	/* HTTP升级文件校验完毕，跳过HTTP文件头部 */
	pVxImgFile += sizeof(HTTP_FILE_HEADER);
	nVxImgFileSize -= sizeof(HTTP_FILE_HEADER);

	/* 页面升级文件大小起码包含两个IMG_HEADER与一个bootrom，后面读取第二个IMG_HEADER的内容才有意义。 */
	/* 20180917: Uboot may be absent */
	if (nVxImgFileSize <= sizeof(IMG_HEADER))
	{
		return EINVFMT;
	}

	/* 镜像头部地址，如果升级文件带有uboot，则可能变化 */
	pVxHeaderPointer = pVxImgFile;

	/* 检查升级镜像的ImgHeader头部合法性校验 */
	if (ENONE != (tpError = httpChkImgHeader(context, pVxHeaderPointer, pVxHeader, shouldResetConfig)))
	{
		return tpError;
	}
	
	/* 如果存在uboot，对uboot长度做检查 */
	if (pVxHeader->includeBoot)
	{
		/* 取出实际的vxWorks(不包含bootrom)升级镜像的ImgHeader */		

		/*
			注意：以前没有指定uboot的偏移和长度，直接默认
			uboot的偏移就是imgheader结束的位置，长度是当前分区表中uboot的大小。
			这样会造成升级文件浪费空间（比如uboot实际的长度小于分区表中uboot的长度）。
			现在把这个长度放到img header的file属性中
		*/
		int ubootOffset = ntohl(pVxHeader->file[IMG_FILE_BOOT_INDEX].fileOffset);
		int ubootSize = ntohl(pVxHeader->file[IMG_FILE_BOOT_INDEX].fileSize);
		if (ubootOffset == 0)
		{
			ubootOffset = sizeof(IMG_HEADER);
			ubootSize = getBlockMaxSize(BOOTIMG_BLOCK_INDEX);
		}
		if (ALIGN_SIZE_BIG(ubootSize, FIRM_ALIGN_SIZE) > ALIGN_SIZE_SMALL(getBlockMaxSize(BOOTIMG_BLOCK_INDEX), FIRM_ALIGN_SIZE))
		{
			HTTP_DEBUG("uboot size 0x%x in upgrading file is larger than uboot partition size 0x%x!\n", 
				ALIGN_SIZE_BIG(ubootSize, FIRM_ALIGN_SIZE), ALIGN_SIZE_SMALL(getBlockMaxSize(BOOTIMG_BLOCK_INDEX), FIRM_ALIGN_SIZE)
				);
			return EINVFMT;
		}

		pVxHeaderPointer = pVxImgFile + ubootOffset + ubootSize;
		if (pVxHeaderPointer + sizeof(IMG_HEADER) > pVxImgFile + nVxImgFileSize)
		{
			HTTP_DEBUG("uboot size error!\n");
			return EINVFMT;
		}
		
		httpUpgrade_memget(context, &imgHeader, pVxHeaderPointer, sizeof(IMG_HEADER));
		pVxHeader = &imgHeader;
		
		/* 检查vxWorks(不包含bootrom)升级镜像的ImgHeader头部合法性校验 */
		if (ENONE != (tpError = httpChkImgHeader(context, pVxHeaderPointer, pVxHeader, NULL)))
		{
			return tpError;
		}

	}
	
	/* 做合法性检查 */
	/* 此处firmware size包括了firmware以及附属数据如rsa签名 */
	int firmSize = ntohl(pVxHeader->len);
	if (ALIGN_SIZE_BIG(firmSize, FIRM_ALIGN_SIZE) > ALIGN_SIZE_SMALL(getBlockMaxSize(FIRMWARE_BLOCK_INDEX), FIRM_ALIGN_SIZE))
	{
		HTTP_DEBUG("firmware size in upgrading file is larger than uboot partition size!\n");
		return EINVFMT;
	}

	if (pVxHeaderPointer + firmSize > pVxImgFile + nVxImgFileSize)
	{
		HTTP_DEBUG("firmware size error!\n");
		return EINVFMT;
	}
	
	/* 读取RSA签名的值，后面计算md5时，需要将该部分内容置为0，从而使得文件与添加RSA之前保持一致 */
	/* 注意这里的offset的起始地址是pVxHeaderPointer即镜像头部的地址 */
	httpUpgrade_memget(context, rsaSignFile, (char *)pVxHeaderPointer + ntohl(pVxHeader->file[IMG_FILE_SIGN_INDEX].fileOffset), RSA_KEY_LEN);
	httpUpgrade_memset(context, (char *)pVxHeaderPointer + ntohl(pVxHeader->file[IMG_FILE_SIGN_INDEX].fileOffset), 0, RSA_KEY_LEN);

	memset(calcMd5, 0, MD5_DIGEST_LEN);

	/* 计算升级内容的MD5（如果有uboot镜像则包括uboot)，rsa签名的原始数据就是升级内容的MD5 */
	httpUpgrade_MD_string(context, pVxImgFile, nVxImgFileSize, calcMd5, MD5, MD_TRANSFORM_NORMAL);
#if 0
	modelRead(DEVICE_DATA_ID, (UINT8 *)&devData);
	rsaKeyIndex = devData.oemId;
	if (rsaKeyIndex > FAST_OEM_ID)
	{
		rsaKeyIndex = TP_OEM_ID;
	}
#endif
	
	if (!signCheckOk)
	{
		HTTP_DEBUG("RSA SIGN is error!");
		/* 依然检测RSA，但是失败的时候不退出，这样串口能看到 */
#ifdef UPGRADE_RSA_CHECK_FAIL_PERMIT
		if (isRsaCheckFailPermit())
		{
			printf("upgrade rsa check fail is permitted, go on.\n");
		}
		else
		{
			return ERSACHECK;
		}
#else
		return ERSACHECK;
#endif
	}
	if (firmwareAddr)
		*firmwareAddr = pVxImgFile;

	if (firmwareLen)
		*firmwareLen = nVxImgFileSize;
	return ENONE;
}



/*
 * Function Name: upgrade_init
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Initiate local data and server page info.
 * Parameter:
 *       N/A
 * return:
 *       0: Succeeded;
 *       ERR_HWID_NOT_FOUND: current hardware id not found.
 *       ERR_FWID_NOT_FOUND: allowed firmware id not found.
 *       ERR_READ_FLASH: Read command execution error.
 *       ERR_BAD_ADDRESS: Invalid address values passed. 
 *       ERR_GENERIC: Web content error.
 */
int upgrade_init()
{
	int ret = 0;

	//init local IDs.
	ret = localInfoInit();

	if (ret < 0)
	{
		DBG("local info init failed, exit");
		return ret;
	}

	return 0;
}

int checkFirmware(FW_INFO *fw_info)
{
	if (ENONE != httpCheckFirmware(NULL, fw_info->buf, fw_info->size, &(fw_info->fwAddr), &(fw_info->fwSize), NULL))
		return -1;
	else
		return 0;
}
/*
 * Function Name: do_http_check
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Firmware check.
 * Parameter:
 *       size: Upgrading firmware size.
 * return:
 *       0: Succeeded;
 *       ERR_INCORRECT_FILE_SIZE: File size is incorrect.
 *       ERR_RSA_CHECK_FAIL: Signature is incorrect.
 *       ERR_HWID_NOT_SUPPORTED: HwID is not supported.
 *       ERR_PARTITION_TYPE_NOT_SUPPORTED: Firmware has not all partitions.
 *       ERR_WRITE_FLASH: Write command execution error.
 *       ERR_READ_FLASH: Read command execution error.
 *       ERR_BAD_ADDRESS: Invalid address values passed. 
 */
int do_http_check(const ulong size)
{
	int ret = 0;

	fw_info.buf = (unsigned char*)WEBFAILSAFE_UPLOAD_RAM_ADDRESS;
	fw_info.size = size;

#ifdef UIP_CACHE_OPERATION
	uip_cache_enable();
#endif

	//check firmware size, RSA signature, HwID, content type, and validate partitions. 
	ret = checkFirmware(&fw_info);

#ifdef UIP_CACHE_OPERATION
	uip_cache_disable();
#endif
	
	if (ret < 0)
	{
		DBG("check firmware failed, exit");
		return ret;
	}

	return ret;
}
int upgradeFirmware(FW_INFO *fw_info)
{
	int ret = serialFirmware(NULL, fw_info->fwAddr, fw_info->fwSize);

	/* uboot升级固件以后直接恢复出厂 */
	if (ret == ENONE)
	{
#ifdef CONFIG_DUAL_IMAGE
		/* 更新BOOTCFG分区信息，默认使用第一个分区 */
		setCurrentFirmwareIndex(FIRMWARE_BLOCK_INDEX);
#endif
		printf("erase user config partition...\n");
		eraseFlash(UIP_FLASH_BASE + USER_CONFIG_OFFSET, USER_CONFIG_LEN);
		return 0;
	}
	else
	{
		return -1;
	}
}
/*
 * Function Name: do_http_upgrade
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Firmware upgrade.
 * Parameter:
 *       size: Upgrading firmware size.
 * return:
 *       0: Succeeded;
 *       ERR_WRITE_FLASH: Write command execution error.
 *       ERR_READ_FLASH: Read command execution error.
 *       ERR_ERASE_FLASH: Erase command execution error.
 *       ERR_BAD_ADDRESS: Invalid address values passed. 
 */
int do_http_upgrade(const ulong size)
{
	int ret = 0;

	//write upgrade data into flash.
	ret = upgradeFirmware(&fw_info);
	if (ret < 0)
	{
		DBG("upgrade firmware failed, exit");
		return ret;
	}

	return ret;
}

// info about current progress of failsafe mode
int do_http_progress(const int state){
	unsigned char i = 0;

	/* toggle LED's here */
	switch(state){
		case WEBFAILSAFE_PROGRESS_START:

			/* turn on sys led. */
			all_leds_on(1);
			printf("HTTP server is ready!\n\n");
			break;

		case WEBFAILSAFE_PROGRESS_TIMEOUT:
			//printf("Waiting for request...\n");
			break;

		case WEBFAILSAFE_PROGRESS_UPLOAD_READY:
			all_leds_on(0);
			printf("HTTP upload is done! Upgrading...\n");
			break;

		case WEBFAILSAFE_PROGRESS_UPGRADE_READY:
			all_leds_on(0);
			printf("HTTP ugrade is done! Rebooting...\n\n");
			break;

		case WEBFAILSAFE_PROGRESS_UPGRADE_ABORTED:
			all_leds_on(0);
			printf("\nWeb failsafe mode aborted!\n\n");
			break;

		case WEBFAILSAFE_PROGRESS_UPGRADE_FAILED:
			all_leds_on(0);
			printf("## Error: HTTP ugrade failed!\n\n");

			// wait 1 sec
			milisecdelay(1000);

			break;
	}

	return(0);
}
