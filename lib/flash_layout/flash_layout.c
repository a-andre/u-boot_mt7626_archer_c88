#include "flash_layout.h"
#include "flash_ops.h"




int getBlockAddr(int index)
{
	switch (index)
	{
		case BOOTIMG_BLOCK_INDEX:
			return FACTORY_BOOT_OFFSET;

		case FIRMWARE_BLOCK_INDEX:
			return FIRMWARE_OFFSET;

		case CONFIG_BLOCK_INDEX:
			return USER_CONFIG_OFFSET;

		case PROFILE_BLOCK_INDEX:
			return FACTORY_INFO_OFFSET;

		case BOOTCFG_BLOCK_INDEX:
			return BOOTCFG_OFFSET;

		case FIRMWARE2_BLOCK_INDEX:
			return FIRMWARE2_OFFSET;

		default:
			return 0;

	}
	return 0;

}

int getBlockMaxSize(int index)
{
	switch (index)
	{
		case BOOTIMG_BLOCK_INDEX:
			return FACTORY_BOOT_LEN;

		case FIRMWARE_BLOCK_INDEX:
			return FIRMWARE_LEN;

		case CONFIG_BLOCK_INDEX:
			return USER_CONFIG_LEN;

		case PROFILE_BLOCK_INDEX:
			return FACTORY_INFO_LEN;

		case BOOTCFG_BLOCK_INDEX:
			return BOOTCFG_LEN;

		case FIRMWARE2_BLOCK_INDEX:
			return FIRMWARE2_LEN;

		default:
			return 0;

	}
	return 0;

}

/*
 * Function Name: executeUpgrade
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Do actual upgrade work.
 * Parameter:
 *       addrOffset: Flash address to be copyed to.
 *       base: Points to the data to be copyed.
 *       size: Data size to be copyed in bytes.
 * return:
 *       0: Succeeded;
 *       ERR_WRITE_FLASH: Write command execution error.
 *       ERR_READ_FLASH: Read command execution error.
 *       ERR_ERASE_FLASH: Erase command execution error.
 *       ERR_BAD_ADDRESS: Invalid address values passed.
 */
static int executeUpgrade(uint32_t addrOffset, char* base, uint32_t size)
{
	int ret = 0;

	DBG("execute upgrade...");
	DBG_UNIT("write offset: %x, from: %p, size: %x", addrOffset, base, size);

	//erase copy region.
	ret = eraseFlash(addrOffset,  size);
	if (ret < 0)
	{
		ERR("erase flash failed!");
		return ret;
	}

	//start to write firmware.
	ret = writeFlash(addrOffset, (uint8_t*)base, size);
	if (ret < 0)
	{
		ERR("write flash failed!");
		return ret;
	}

	return 0;
}



int writeSpecifyBlock(BLOCK_INDEX index, UINT32 len, char *buf)
{
	return executeUpgrade(UIP_FLASH_BLOCK_ADDR(index), buf, len);
}

int readSpecifyBlock(BLOCK_INDEX index, int len, char *buf)
{
	return readFlash(UIP_FLASH_BLOCK_ADDR(index), buf, len);
}



