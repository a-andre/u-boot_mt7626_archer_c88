#ifndef __IMAGE_HEADER_H__
#define IMG_COMMON_MAGIC		0x1BEC493F
#define IMG_MAGIC				0x2876C44D	/* MD5[-4:-1]:"TPOS" */

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long UINT64;
typedef short INT16;
typedef int INT32;
typedef long long INT64;
/*!

	@brief			Header definitions related with user configuration and 
					firmare upgrading. Shared between uboot and vx image.
	@author			xiejiabai
	@history		2019.8.2, create.
*/

/*!
	@brief			Length of mac stored in FLASH
*/
#define MT_B_MAC_LEN						6

/*!
	@brief			Length of pin stored in FLASH
*/
#define MT_B_PIN_LEN						8

/*!
	@brief			Length of device id stored in FLASH
*/
#define MT_B_DEVID_LEN						20

/*!
	@brief			Length of hardware id stored in FLASH
*/
#define MT_B_HWID_LEN						16

/*!
	@brief			Length of nation code stored in FLASH
*/
#define MT_B_CID_LEN						2

/*!
	@brief			Length of special id stored in FLASH
*/
#define MT_B_SID_LEN						4

/*!
	@brief			Length of firmware id stored in FLASH
*/
#define MT_B_FWID_LEN						16



/*!
	@brief			User configuration. If not the same with that in flash, the flash user config is regarded as invalid.
					If you want to force facotry resetting after firmware upgrading, this magic can be modified.
*/
#define CONFIG_MAGIC					"SDMP_CONF_V005"

/*!

	@brief			User configuration header.
*/
typedef struct _CONFIG_FILE_HEADER
{
	UINT8 magic[16];	/*!< 魔术字"SDMP-CONFIG"，标识是SDMP新平台的用户配置文件 */
	UINT32 length;		/*!< FLASH中的配置数据（经过了压缩和加密）长度，不包括CONFIG_FILE_HEADER */
	UINT32 oriLength;	/*!< 配置数据的原始长度，不包括CONFIG_FILE_HEADER */
	UINT8 data[0];		/*!< 数据 */
}CONFIG_FILE_HEADER;

/*!
	@brief			在目前的设计中，profile文件存储了设备的MAC地址和PIN码等信息。
*/
typedef struct _PROFILE
{
	UINT8 mac[2][8];	/*!< 
							desc:
								MAC Address. Only the first one is used now. 
								This mac will be used as LAN mac. WAN mac and wlan mac are derived from it.
							write:	
								Flashed during T30test in manufacturing.
							format:
								6-bytes HEX, i.e. 00 01 02 03 04 05; 
								If all 0x00 or all 0xff, regarded as invalid.
								
							
						*/
	UINT8 pin[8];	/*!< 
							desc:
								PIN. For wireless wps function and default wireless psk. 
							write:
								Flashed during T30test in manufacturing. For TP-LINK models.
							format:
								8-bytes ASCII, i.e. "12345678" => 30 31 32 33 34 35 36, no \0 ended. 
								if all 0x00 or all 0xff, regarded as invalid.
								Should pass PIN code format checking.
								
						*/
	UINT8 reserved[8];	
	UINT8 devID[20];	/*!< 
							desc:
								Device ID For MIC and Cloud services. Unique for every DUT.
								
							write:	
								Flashed during T10test in manufacturing. For MIC-used models.
								
							format:
								20bytes HEX, i.e. ff0123456789ff01234567. 
								If all 0x00 or all 0xff, regarded as invalid.
								Device id should come from MIC server. DO NOT generate it by yourself!!!

						*/
	UINT8 hwID[16];		/*!<
							desc:
								Hardware ID. We DO NOT flash it. DONT'T USE IT!!!

							write:
								NO.
								
							format:
								16bytes HEX, i.e. 64 72 1A D1 19 35 73 DB 9F 91 52 9D A2 9F 93 64.
								If all 0x00 or all 0xff, regarded as invalid.
								hw id should come from TP-Link DMS server. DO NOT generate it by yourself!!!
						*/
								
	char  SID[12];		/*!< 
							desc:
								Special ID(ugly name). 
								Used ONLY!!! in wifi zone limit.
								Only TP-Link foreign products use it now.

							write:
								Flashed during T30test in manufacturing. For some models.
								
							format:
								4bytes HEX, i.e. 45 53 00 00, 4553->"US", 0000->reserved. NO \0 ended!!!. 
						*/

	char  CID[12];		/*!< 
							desc:
								Country ID(ugly name). 
								Used in soft functions limit now.(i.e. timezone, language, even model name...)
								Only TP-Link foreign products use it now.
	
							write:
								Flashed during T30test in manufacturing. For some models.
								
							format:
								2bytes ASCII, i.e. "US" -> 45 53. NO \0 ended!!!. 
						*/

} PROFILE;

/*!

	@brief			firmware附属数据编号
					附属数据属于扩展性质，头部中指定了附属数据的偏移和具体长度
*/
typedef enum _IMG_FILE_INDEX
{
	IMG_FILE_MINIFS_INDEX = 0,		/*!<	minifs data, minifs模块初始化时需要 */
	IMG_FILE_CONFIG_INDEX,			/*!<	user config data， 配置导入导出时使用 */
	IMG_FILE_VXIMG_INDEX,			/*!<	vxlzma.bin data，目前暂未看到使用 */
	IMG_FILE_SUPPORTLIST_INDEX,		/*!<	support list??? */
	IMG_FILE_SIGN_INDEX, 			/*!<	RSA校验签名文件，用于升级校验 */
	IMG_FILE_BOOT_INDEX,			/*!<	uboot data，标记uboot数据的位置 */
	IMG_FILE_FIRMMD5_INDEX,			/*!<	MD5数据，标记整个firmware的校验码，用于FLASH固件完整性检查 */
	IMG_FILE_END_INDEX = 8			/*!<	Max number of individual files */
}IMG_FILE_INDEX;


/*!
	@brief			附属数据定义
*/
typedef struct _IMG_FILE
{
	UINT32		fileOffset;			/*!<	数据偏移（以header地址作为起点）*/
	UINT32		fileSize;			/*!<	数据长度 */
}IMG_FILE;

/*!
	@brief		升级固件头部
*/
typedef struct _HTTP_FILE_HEADER
{
	UINT32 		magic; 				/*!<	magic */
	UINT8		md5[16];			/*!<	md5 */
}
HTTP_FILE_HEADER;

/*!
	@brief		固件头部定义
*/
typedef struct _IMG_HEADER
{
	UINT32		magic;				/*!<	magic number, "IMG0" */
	UINT32		len;				/*!<	Length of file including header */
	UINT32		prodid;				/*!<	PRODUCT_ID, i.e. 0x08440001. Unique for every model. */
	UINT32		subVersion;			/*!<	sub version number, not used now, should be 0. */
	UINT32		mainVersion;		/*!<	main version of platform, 0x55010002 => 1.0.2 */
	UINT8		includeBoot;		/*!<	if uboot is included in the firmware to be upgraded. */
	UINT8		noused[3];			/*!<	for padding. */
	UINT32		specialId;			/*!<	special id. Used in config file generation. */
	UINT32		countryCode;		/*!<	countryCode. Used in config file generation. */
	UINT32		oemId;				/*!<	用来标记是tp mecury fast品牌，计算rsa校验时用到 */
	UINT32		reserved[7];		/*!<	reserved. */
	IMG_FILE	file[IMG_FILE_END_INDEX];	/*!< Offsets of partitions from start of header, see IMG_FILE_INDEX */
}IMG_HEADER;

#endif

