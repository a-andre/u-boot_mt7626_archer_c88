#include "flash_ops.h"

DECLARE_GLOBAL_DATA_PTR;

#define CFG_FLASH_SECTOR_SIZE (4*1024)


/*
 * Function Name: addr_flash
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Test if an address is a flash address.
 * Parameter:
 *       addr: Address to test.
 * return:
 *       If the address is a flash address, return 0;
 *       else return ERR_BAD_ADDRESS 
 */
int addr_flash(void* addr)
{
	if((unsigned long)addr < gd->bd->bi_flashstart
	  || (unsigned long)addr >= gd->bd->bi_flashstart + gd->bd->bi_flashsize)
	{
		ERR("%p: Not a valid flash address!", addr);
		return ERR_BAD_ADDRESS;
	}
	return 0;
}

int check_addr_flash(void* addr)
{
	if((unsigned long)addr < gd->bd->bi_flashstart
	  || (unsigned long)addr >= gd->bd->bi_flashstart + gd->bd->bi_flashsize)
	{
		return -1;
	}
	return 0;
}

/*
 * Function Name: addr_mem
 * Description: Test if an address is a ram memory address.
 * Parameter:
 *       addr: Address to test.
 * return:
 *       If the address is a memory address, return 0;
 *       else return ERR_BAD_ADDRESS 
 */
int addr_mem(void* addr)
{
	if((unsigned long)addr < gd->bd->bi_dram[0].start
		|| (unsigned long)addr >= gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size)
	{
		ERR("%p: Not a valid ram address!ram_size:0x%x", addr, gd->bd->bi_dram[0].size);
		return ERR_BAD_ADDRESS;
	}
	return 0;
}

/*
 * Function Name: writeFlash
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Write data to given flash address.
 * Parameter:
 *       addrOffset: Flash address to be written to.
 *       buf: Points to the data buf to write.
 *       buflen: Length of the data buf in bytes.
 * return:
 *       0: Succeeded;
 *       ERR_WRITE_FLASH: Write command execution error.
 *       ERR_BAD_ADDRESS: Invalid address values passed. 
 */
int writeFlash(uint32_t addrOffset, uint8_t* buf, uint32_t buflen)
{
	int ret = 0;	
	char cmdbuf[70];
	
	//TODO:need to detect address type.
	ret = addr_flash((void*)addrOffset);
	if(ret < 0)
	{
		return ret;
	}

	ret = addr_mem(buf);
	if(ret < 0)
	{
		return ret;
	}

	sprintf(cmdbuf, "cp.b 0x%x 0x%x 0x%x", (uint32_t)buf, addrOffset, buflen);
	if(run_command(cmdbuf, 0) < 0)
	{
		return ERR_WRITE_FLASH;
	}

	return 0;
}

/*
 * Function Name: readFlash
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Read data from a given flash address.
 * Parameter:
 *       addrOffset: Flash address to be read from.
 *       buf: Points to the data buf to receive the read data.
 *       buflen: Length of the data buf in bytes.
 * return:
 *       0: Succeeded;
 *       ERR_READ_FLASH: Read command execution error.
 *       ERR_BAD_ADDRESS: Invalid address values passed. 
 */
int readFlash(uint32_t addrOffset, uint8_t* buf, uint32_t buflen)
{
	int ret = 0;
	int  readlen = 0;
	char cmdbuf[70];
	
	//TODO:need to detect address type.
	ret = addr_flash((void*)addrOffset);
	if(ret < 0)
	{
		return ret;
	}

	ret = addr_mem(buf);
	if(ret < 0)
	{
		return ret;
	}

	sprintf(cmdbuf, "cp.b 0x%x 0x%x 0x%x", addrOffset, (uint32_t)buf, buflen);
	if(run_command(cmdbuf, 0) < 0)
	{
		return ERR_READ_FLASH;
	}
	
	return 0;
}

/*
 * Function Name: eraseFlash
 * Author: CaiBin
 * Date: 2014-11-07
 * Description: Erase flash content of a given flash address.
 * Parameter:
 *       addrOffset: Flash address to erase.
 *       eraselen:   Erase length in bytes.
 *       buflen: Length of the data buf in bytes.
 * return:
 *       0: Succeeded;
 *       ERR_ERASE_FLASH: Erase command execution error.
 *       ERR_BAD_ADDRESS: Invalid address values passed. 
 */
int eraseFlash(uint32_t addrOffset, uint32_t eraselen)
{
	int ret;
	char cmdbuf[70];

	ret = addr_flash((void*)addrOffset);
	if(ret < 0)
	{
		return ret;
	}
	
	//if addr offset or eraselen is not on sector boundary, return err.
	if (0 != (addrOffset % CFG_FLASH_SECTOR_SIZE))
	{
		ERR("%s: addrOffset is 0x%08x\n, not on sector boundary!", __func__, addrOffset);
		return ERR_BAD_ADDRESS;
	}
	if (0 != (eraselen % CFG_FLASH_SECTOR_SIZE))
	{
		ERR("%s: eraselen is 0x%08x\n, not on sector boundary!", __func__, eraselen);
		return ERR_BAD_ADDRESS;
	}

	//TODO:need to detect address type.
	sprintf(cmdbuf, "erase 0x%x 0x%x", addrOffset, eraselen);
	if(run_command(cmdbuf, 0) < 0)
	{
		return ERR_ERASE_FLASH;
	}

	return 0;
}


