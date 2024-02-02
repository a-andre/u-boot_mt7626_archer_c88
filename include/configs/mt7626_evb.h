/*
 * (C) Copyright 2018 MediaTek.Inc
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __MT7626_H
#define __MT7626_H

#include <linux/sizes.h>
#include <configs/autoconf.h>


/**********************************************************************************************
 *                                      ARM Cortex A7
 **********************************************************************************************/
#define CONFIG_MEDIATEK

#define CONFIG_TINY_BOOT 1

/* Machine ID */
#define CONFIG_MACH_TYPE		7626

/* Serial drivers */
#define CONFIG_BAUDRATE 		115200

/*
 * SPL related defines
 */
#ifndef NORMAL_UBOOT
#define CONFIG_SPL
#endif
#define CONFIG_SPL_FRAMEWORK
#define CONFIG_SPL_DECOMPRESS_UBOOT
#define CONFIG_SPL_BOARD_INIT
#define CONFIG_SPL_LZMA
/*#define CONFIG_SPL_RAM_DEVICE*/

#define CONFIG_SPL_LIBCOMMON_SUPPORT	/* image.c */
#define CONFIG_SPL_LIBGENERIC_SUPPORT	/* string.c */
#define CONFIG_SPL_SERIAL_SUPPORT

#define CONFIG_SPL_TEXT_BASE		0x201000
#define CONFIG_SPL_MAX_SIZE 		0x20000     /* Max 128KB */
#define CONFIG_SPL_MAX_FOOTPRINT	0x10000

/* Just define any reasonable size */
#define CONFIG_SPL_STACK_SIZE		0x1000
/* SPL stack position - and stack goes down */
#define CONFIG_SPL_STACK 			0x101000

#define CONFIG_SPL_PAD_ALIGNMENT	0x4
#define CONFIG_MTK_BROM_HEADER_INFO	"media=nor"

/**********************************************************************************************
 *                                          Memory
 **********************************************************************************************/
/* Memory layout */
/* DRAM definition */
/*
 * Iverson 20140521 : We detect ram size automatically.
 *      CONFIG_SYS_SDRAM_SIZE define max uboot size.
 *      The max size that auto detection support is 256MB.
 */

#define CONFIG_NR_DRAM_BANKS		        1
#define CONFIG_SYS_SDRAM_BASE		        0x40000000

#define CONFIG_CUSTOMIZE_DRAM_SIZE	        1
#if defined(ON_BOARD_256Mb_DRAM_COMPONENT)
#define CONFIG_SYS_SDRAM_SIZE               SZ_32M
#else
#error "memory defined error!!!")
#endif

/* Uboot heap size for malloc() */
#define CONFIG_SYS_MALLOC_LEN               SZ_1M
#ifdef NORMAL_UBOOT
#define CONFIG_SYS_TEXT_BASE                0x41C00000
#else
#define CONFIG_SYS_TEXT_BASE                0x41E00000
#endif
#define CONFIG_SYS_SPL_ARGS_ADDR            0x40000000

#define CONFIG_SYS_UBOOT_START              CONFIG_SYS_TEXT_BASE
#define CONFIG_SYS_UBOOT_MAX_SIZE           SZ_2M
#define CONFIG_SYS_INIT_SP_ADDR             (CONFIG_SYS_TEXT_BASE + \
                                                CONFIG_SYS_UBOOT_MAX_SIZE - \
                                                GENERATED_GBL_DATA_SIZE)

/* RichOS memory partitions, load-addr is very important ! */
#define CONFIG_SYS_LOAD_ADDR                0x41007F1C
#define CONFIG_SYS_IMAGE_HDR_ADDR           CONFIG_SYS_LOAD_ADDR

/* Linux DRAM definition */
#define CONFIG_LOADADDR                     CONFIG_SYS_LOAD_ADDR

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 64 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CONFIG_SYS_BOOTM_LEN	            0x4000000

/*
 * For different DDR Sample Type Select
 */
#define DRAM_PIN_MUX_TYPE_DDR2X16_QFN_W     2
#define DRAM_PIN_MUX_TYPE_DDR2X16_QFN_E     3
#define CONFIG_DRAM_TYPE                    DRAM_PIN_MUX_TYPE_DDR2X16_QFN_W


/**********************************************************************************************
 *                                           Board
 **********************************************************************************************/

/* Board */
#if defined(MT7626_FPGA_BOARD)
#define CFG_FPGA_PLATFORM   1
#endif
#define CONFIG_ADD_MTK_HEADER               "n"

/* Not to access NOR NVRAM partition for bringup only, we can remove it late */
/*#define CONFIG_DELAY_ENVIRONMENT*/
#define CONFIG_BOARD_LATE_INIT


/**********************************************************************************************
 *                                          Devices
 **********************************************************************************************/

/********************** Flash *************************/
#if defined(ON_BOARD_SPI_NOR_FLASH_COMPONENT)
/* SPI Flash Configuration */

/* Parallel Nor Flash */
#define CONFIG_SYS_NO_FLASH

/* SPI NOR Flash */
#define CONFIG_CMD_NOR
#define CONFIG_SPL_NOR_SUPPORT

#define CONFIG_ENV_IS_IN_NOR

#define CONFIG_ENV_SIZE                    SZ_4K
#define CONFIG_ENV_SECT_SIZE               0x1000   /* 4KB */
#define CONFIG_ENV_OFFSET                  CONFIG_NOR_NVRAM_OFFSET

#define CONFIG_NOR_BOOTLOADER_OFFSET       0x00000  //boot-loader : u-boot-mtk.bin offset
#define CONFIG_NOR_BOOTLOADER_SIZE         0x1F000  //boot-loader max size : SPL 45.5K + u-boot.img 78.5K

#define CONFIG_NOR_UBOOT_OFFSET            0xB600     //u-boot.img offset
#define CONFIG_NOR_UBOOT_SIZE              0x13A00  //Uboot max size : u-boot.img 78.5K

#define CONFIG_NOR_PROFILE_OFFSET			0x1F000
#define CONFIG_NOR_PROFILE_SIZE				0x800

#define CONFIG_NOR_RADIO_OFFSET				0x1F800
#define CONFIG_NOR_RADIO_SIZE				0x800

#define CONFIG_NOR_CONFIG_OFFSET			0x20000
#define CONFIG_NOR_CONFIG_SIZE				0x5000

#define CONFIG_NOR_BOOTIMG_OFFSET			0x25000
#define CONFIG_NOR_BOOTIMG_SIZE				0x10000

#define CONFIG_NOR_TPHEAD_OFFSET			0x35000
#define CONFIG_NOR_TPHEAD_SIZE				0x200

#define CONFIG_NOR_KERNEL_OFFSET			0x35200
#define CONFIG_NOR_KERNEL_ADDR				0x30035200
#define CONFIG_NOR_NORMAL_UBOOT_ADDR		0x30025000

#define CONFIG_NOR_NVRAM_OFFSET            0x60000  //NVRAM offset
#define CONFIG_NOR_NVRAM_SIZE              0x10000  //NVRAM max size 64K

#define CONFIG_NOR_RF_OFFSET               0x70000  //RF offset
#define CONFIG_NOR_RF_SIZE                 0x40000  //RF max size 256K

#define CONFIG_NOR_LINUX_OFFSET            0xB0000  //Linux begin offset
#define CONFIG_NOR_LINUX_SIZE              0x720000 //Linux max size 7M

#define CONFIG_NOR_CTP_OFFSET              0xB0000  //CTP begin offset
#define CONFIG_NOR_CTP_SIZE                0x720000 //CTP max size 7M

#define CONFIG_NOR_FLASHIMAGE_OFFSET       0x0      //flashimage.bin begin offset
#define CONFIG_NOR_FLASHIMAGE_SIZE         0x800000 //flashimage.bin max size 8M

#define CONFIG_NOR_BASE                    0x30000000	/* start of FLASH */
#define CONFIG_SYS_UBOOT_BASE              (CONFIG_NOR_BASE + CONFIG_NOR_UBOOT_OFFSET) /* u-boot.img image */
#define CONFIG_SYS_OS_BASE                 (CONFIG_NOR_BASE + CONFIG_NOR_LINUX_OFFSET) /* linux image */
#define CONFIG_SYS_FDT_BASE                (CONFIG_NOR_BASE) /* Not used, only for NOR build */


#define ENV_BOOT_WRITE_IMAGE \
	"boot_wr_img=filesize_check " __stringify(CONFIG_NOR_LINUX_SIZE) \
	";if test ${filesize_result} = good; then snor erase " __stringify(CONFIG_NOR_LINUX_OFFSET) " ${file_align_size}" \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_LINUX_OFFSET) " ${filesize};fi\0"
#define ENV_BOOT_READ_IMAGE \
	"boot_rd_img=snor read ${loadaddr} " __stringify(CONFIG_NOR_LINUX_OFFSET) " 2000" \
	";image_blks 1" \
	";snor read ${loadaddr} " __stringify(CONFIG_NOR_LINUX_OFFSET) " ${img_align_size}\0"
#define ENV_WRITE_UBOOT \
	"wr_uboot=filesize_check " __stringify(CONFIG_NOR_BOOTLOADER_SIZE) \
	";if test ${filesize_result} = good; then snor erase " __stringify(CONFIG_NOR_BOOTLOADER_OFFSET) " " __stringify(CONFIG_NOR_BOOTLOADER_SIZE) \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_BOOTLOADER_OFFSET) " " __stringify(CONFIG_NOR_BOOTLOADER_SIZE) ";fi\0"
#define ENV_WRITE_CTP \
	"wr_ctp=filesize_check " __stringify(CONFIG_NOR_CTP_SIZE) \
	";if test ${filesize_result} = good; then snor erase " __stringify(CONFIG_NOR_CTP_OFFSET) " " __stringify(CONFIG_NOR_CTP_OFFSET) \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_CTP_OFFSET) " " __stringify(CONFIG_NOR_CTP_SIZE) ";fi\0"
#define ENV_BOOT_READ_CTP \
	"boot_rd_ctp=snor read 0x40000000 " __stringify(CONFIG_NOR_CTP_OFFSET) " 0x720000\0"
#define ENV_WRITE_CUSTOM_IMAGE \
	"wr_cumstom_image=custom_image_check " __stringify(CONFIG_NOR_FLASHIMAGE_SIZE) \
	";if test ${img_result} = good; then snor erase " __stringify(CONFIG_NOR_FLASHIMAGE_OFFSET) " " __stringify(CONFIG_NOR_FLASHIMAGE_SIZE) \
	";snor write 0x40000000 "  __stringify(CONFIG_NOR_FLASHIMAGE_OFFSET) " " __stringify(CONFIG_NOR_FLASHIMAGE_SIZE) ";fi\0"
#define ENV_WRITE_FLASHIMAGE \
	"wr_flashimage=filesize_check " __stringify(CONFIG_NOR_FLASHIMAGE_SIZE) \
	";if test ${filesize_result} = good; then snor erase " __stringify(CONFIG_NOR_FLASHIMAGE_OFFSET) " " __stringify(CONFIG_NOR_FLASHIMAGE_SIZE) \
	";snor write ${loadaddr} " __stringify(CONFIG_NOR_FLASHIMAGE_OFFSET) " ${filesize};fi\0"


#else

#error "No any flash component error!!!"

#define CONFIG_SYS_NO_FLASH
#define CONFIG_ENV_IS_NOWHERE
#define CONFIG_ENV_SIZE                     SZ_4K

#define ENV_BOOT_WRITE_IMAGE  "boot_wr_img=none\0"
#define ENV_BOOT_READ_IMAGE   "boot_rd_img=none\0"
#define ENV_WRITE_UBOOT       "wr_uboot=none\0"
#define ENV_WRITE_CTP
#define ENV_BOOT_READ_CTP

#endif

#define CONFIG_ENV_VARS_UBOOT_CONFIG

/* Watchdog */
#define CFG_HW_WATCHDOG 1
#define CONFIG_WATCHDOG_OFF

/* Console configuration */
#define CONFIG_SYS_CBSIZE		            1024
#define CONFIG_SYS_PBSIZE		            (CONFIG_SYS_CBSIZE +		\
					                            sizeof(CONFIG_SYS_PROMPT) + 16)
#define CONFIG_SYS_CONSOLE_IS_IN_ENV

#define ENV_DEVICE_SETTINGS \
	"stdin=serial\0" \
	"stdout=serial\0" \
	"stderr=serial\0"

#if defined (ON_BOARD_2M_FLASH_COMPONENT)
#define CFG_FLASH_SIZE			0x00200000
#elif defined (ON_BOARD_4M_FLASH_COMPONENT)
#define CFG_FLASH_SIZE			0x00400000
#elif defined (ON_BOARD_8M_FLASH_COMPONENT)
#define CFG_FLASH_SIZE			0x00800000
#elif defined (ON_BOARD_16M_FLASH_COMPONENT)
#define CFG_FLASH_SIZE			0x01000000
#endif


#define CFG_FLASH_BASE			0x30000000
#define UIP_FLASH_BASE          CFG_FLASH_BASE
#if defined(MT7628_ASIC_BOARD)
#define UIP_FLASH_SIZE          (gd->bd->bi_flashsize)
#elseif defined(MT7621_ASIC_BOARD)
#define UIP_FLASH_SIZE          (gd->bd->bi_flashsize)
#endif

/* Copy from slp platform factory_boot rt2880.h */
#ifdef CONFIG_UIP

#undef CONFIG_LOADADDR
#define CONFIG_LOADADDR		0x40200000

/*
 * Web Failsafe configuration
 */
#define WEBFAILSAFE_UPLOAD_RAM_ADDRESS				CONFIG_LOADADDR
#define WEBFAILSAFE_UPLOAD_UBOOT_ADDRESS			CFG_FLASH_BASE

/*
// Firmware partition offset
//#define WEBFAILSAFE_UPLOAD_KERNEL_ADDRESS			WEBFAILSAFE_UPLOAD_UBOOT_ADDRESS + 0x20000

// U-Boot partition size
//#define WEBFAILSAFE_UPLOAD_UBOOT_SIZE_IN_BYTES		(64 * 1024)

// ART partition size
//#define WEBFAILSAFE_UPLOAD_ART_SIZE_IN_BYTES		(64 * 1024)

// max. firmware size <= (FLASH_SIZE -  WEBFAILSAFE_UPLOAD_LIMITED_AREA_IN_BYTES)
// TP-Link: 128k(Factory Boot, MAC/model/WPS pin block, Radio Data block), 64k(User Config)
//#define WEBFAILSAFE_UPLOAD_LIMITED_AREA_IN_BYTES	(192 * 1024)
*/

/* progress state info */
#define WEBFAILSAFE_PROGRESS_START			0
#define WEBFAILSAFE_PROGRESS_TIMEOUT			1
#define WEBFAILSAFE_PROGRESS_UPLOAD_READY		2
#define WEBFAILSAFE_PROGRESS_UPGRADE_READY		3
#define WEBFAILSAFE_PROGRESS_UPGRADE_FAILED		4
#define WEBFAILSAFE_PROGRESS_UPGRADE_ABORTED    	5

/* Flash layout constants. */
#if defined(CONFIG_SINGLE_BOOT)
#define UPGRADE_FW_LEN              (0x200 + 0x7C0000) //Upgrade Header + kernel + romfs + jffs2
#else
#define UPGRADE_FW_LEN			(0x200 + 0x10000 + 0x7C0000) //Upgrade Header + uboot + kernel + romfs + jffs2
#endif

/*!
	flash layout:
	factory uboot
	firmwar(including normal uboot)
	userconfig
	exception log
	profile
	radio

*/

/* moved to config.in */
#if 0
/*!
	@brief		factory uboot offset
*/
#define FACTORY_BOOT_OFFSET		0x0
#define FACTORY_BOOT_LEN		0x25000

/*!
	@brief		firmware offset
*/
#define FIRMWARE_OFFSET			0x25000
#define FIRMWARE_LEN			0x3d5000

/*!
	@brief		profile offset
*/
#define FACTORY_INFO_OFFSET		0x3fe000
#define FACTORY_INFO_LEN		0x1000

/*!
	@brief		user config offset
*/
#define USER_CONFIG_OFFSET		0x3fa000
#define USER_CONFIG_LEN		0x2000


/*!
	@brief		radio partition offset 
*/
#define RADIO_OFFSET			0x3ff000
#define RADIO_LEN			0x1000

#endif

#define WEBFAILSAFE_DO_LOCAL_VALIDATION
#define WEBFAILSAFE_RESET_BUTTON
#define WEBFAILSAFE_RESET_BUTTON_SECS 5
/* #define WEBFAILSAFE_DO_FULL_LOCAL_VALIDATION */

#define WEBFAILSAFE_LOCAL_VALIDATION_IN_RAM

#if !defined(NO_NEED_CHECK_RSA)
#define UPGRADE_RSA_CHECK_FAIL_PERMIT
#endif

#define WEBFAILSAFE_FW_ID        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

#define milisecdelay(_x)			udelay((_x) * 1000)
#endif

/********************** Ethernet *************************/
#define CONFIG_ETHADDR                      00:0C:E7:11:22:33
#define CONFIG_IPADDR                       CONFIG_DUT_IP
#define CONFIG_SERVERIP                     CONFIG_SERVER_IP
#define CONFIG_BOOTFILE                     "lede_uImage"
#define CONFIG_CMD_NET
#define CONFIG_RT2880_ETH

#define RALINK_REG(x)		(*((volatile u32 *)(x)))

#define CONFIG_NORMAL_UBOOT_ADDR	0x30025080
#define CONFIG_FIRMWARE_ADDR		0x30025000

/**********************************************************************************************
 *                                       Boot Menu
 **********************************************************************************************/
#if defined(MT7626_FPGA_BOARD)
#define CONFIG_BOOTDELAY                    5
#else
#define CONFIG_BOOTDELAY                    1
#endif

#ifdef CONFIG_TINY_BOOT
#ifdef NORMAL_UBOOT
/*	The next step for normal uboot is to boot vxworks image, so use bootm */
#define CONFIG_BOOTCOMMAND                  "bootm " __stringify(CONFIG_FIRMWARE_ADDR)
#else
/* The next step for factory uboot is to boot normal uboot, so use jmpaddr */
#if 1
#define CONFIG_BOOTCOMMAND                  "jmpaddr " __stringify(CONFIG_NORMAL_UBOOT_ADDR)
#else
#define CONFIG_BOOTCOMMAND                  "bootm " __stringify(CONFIG_NOR_NORMAL_UBOOT_ADDR)
#endif
#endif
#define CONFIG_AUTOBOOT_KEYED
#define CONFIG_AUTOBOOT_PROMPT   "Autobooting in %d seconds\n"
#define CONFIG_AUTOBOOT_STOP_STR "tpl"
#undef  CONFIG_AUTOBOOT_DELAY_STR
#define DEBUG_BOOTKEYS       0
#else
#define CONFIG_BOOTCOMMAND                  "No"
#define CONFIG_CMD_BOOTMENU
#define CONFIG_MENU
#define CONFIG_MENU_SHOW
#define CONFIG_MENU_ACTIVE_ENTRY            2

#define ENV_BOOT_CMD0 \
    "boot0=download_setting kernel;tftpboot ${loadaddr} ${kernel_filename};bootm\0"

#define ENV_BOOT_CMD1 \
    "boot1=download_setting kernel;tftpboot ${loadaddr} ${kernel_filename};run boot_wr_img;run boot_rd_img;bootm\0"

#define ENV_BOOT_CMD2 \
    "boot2=bootm " __stringify(CONFIG_NOR_KERNEL_ADDR) "\0"

#define ENV_BOOT_CMD3 \
    "boot3=download_setting uboot;tftpboot ${loadaddr} ${uboot_filename};run wr_uboot;invaild_env\0"

#define ENV_BOOT_CMD4 \
    "boot4=loadb;run wr_uboot;invaild_env\0"

#define ENV_BOOT_CMD5 \
    "boot5=download_setting ctp;tftpboot ${loadaddr} ${ctp_filename};run wr_ctp\0"

#define ENV_BOOT_CMD6 \
    "boot6=run wr_cumstom_image;invaild_env\0"

#define ENV_BOOT_CMD7 \
    "boot7=download_setting flashimage;tftpboot ${loadaddr} ${flashimage_filename};run wr_flashimage;invaild_env\0"


#define ENV_BOOT_CMD \
    ENV_BOOT_WRITE_IMAGE \
    ENV_BOOT_READ_IMAGE \
    ENV_WRITE_UBOOT \
    ENV_WRITE_CTP \
    ENV_BOOT_READ_CTP \
    ENV_WRITE_CUSTOM_IMAGE \
    ENV_WRITE_FLASHIMAGE \
    ENV_BOOT_CMD0 \
    ENV_BOOT_CMD1 \
    ENV_BOOT_CMD2 \
    ENV_BOOT_CMD3 \
    ENV_BOOT_CMD4 \
    ENV_BOOT_CMD5 \
    ENV_BOOT_CMD6 \
    ENV_BOOT_CMD7

#define ENV_BOOT_MENU \
    "bootmenu_0=1. System Load Linux to SDRAM via TFTP.=run boot0\0" \
    "bootmenu_1=2. System Load Linux Kernel then write to Flash via TFTP.=run boot1\0" \
    "bootmenu_2=3. Boot system code via Flash.=run boot2\0" \
    "bootmenu_3=4. System Load U-Boot then write to Flash via TFTP.=run boot3\0" \
    "bootmenu_4=5. System Load U-Boot then write to Flash via Serial.=run boot4\0" \
    "bootmenu_5=6. System Load CTP then write to Flash via TFTP.=run boot5\0" \
    "bootmenu_6=7. Debugger load image then write to Flash.=run boot6\0" \
    "bootmenu_7=8. System Load flashimage then write to Flash via TFTP.=run boot7\0" \
    "bootmenu_delay=30\0" \
    ""

#define CONFIG_EXTRA_ENV_SETTINGS \
    "kernel_filename=lede-kernel.bin\0" \
    "uboot_filename=u-boot-mtk.bin\0" \
    "ctp_filename=ctp.bin\0" \
    "flashimage_filename=flashimage.bin\0" \
    "invaild_env=no\0" \
    ENV_DEVICE_SETTINGS \
    ENV_BOOT_CMD \
    ENV_BOOT_MENU

#endif /* CONFIG_TINY_BOOT */

/**********************************************************************************************
 *                                      FDT
 **********************************************************************************************/
#ifndef CONFIG_TINY_BOOT
#define CONFIG_FIT
#define CONFIG_OF_LIBFDT
#define CONFIG_FDT_DEBUG
#endif
/**********************************************************************************************
 *                                       UBoot Command
 **********************************************************************************************/
/* Shell */
#define CONFIG_SYS_MAXARGS		            8
#define CONFIG_SYS_PROMPT		            "MT7626> "
#define CONFIG_COMMAND_HISTORY

/* Commands */
#ifdef CONFIG_TINY_BOOT
#define CONFIG_CMD_MEMORY	/* md */
#define CONFIG_CMD_NET		/* tftpboot */
#undef CONFIG_CMD_CRC32
#else
#include <config_cmd_default.h>
#endif

/* Device tree support */
#define CONFIG_OF_BOARD_SETUP
/* ATAGs support for bootm/bootz */
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_CMDLINE_TAG
#define CONFIG_INITRD_TAG

#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_NFS

#ifndef CONFIG_TINY_BOOT
#define CONFIG_SYS_LONGHELP
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_CMD_PING
#endif

/**********************************************************************************************
 *                                      Compression
 **********************************************************************************************/
/*
 * Iverson 20150510 :
 *      Denali-2 use lk to decompress kernel.
 */
#define CONFIG_LZMA
#define CONFIG_MTGPIO

#endif
