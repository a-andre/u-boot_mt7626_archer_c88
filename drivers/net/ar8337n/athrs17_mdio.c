/******************************************************************************
 * Copyright (C) 2021, TP-LINK TECHNOLOGIES CO., LTD.
 *
 * File name:     athrs17_mdio.c
 * Version:       1.0
 * Description:   AR8337N phy read/write ops
 *
 * Authors:       Lin Qi <qilin@tp-link.com.cn>
 ****************************************************************************/

#include <common.h>
#include "athrs17_phy.h"

/*
 * Externel Common mdio read
 */
extern u32 mii_mgr_read(u32 phy_addr, u32 phy_register, u32 *read_data);
extern u32 mii_mgr_write(u32 phy_addr, u32 phy_register, u32 write_data);

u32 mii_mgr_write_adpt(uint32_t phy_addr, uint8_t phy_register, uint16_t write_data)
{
	return mii_mgr_write(phy_addr, (uint32_t)phy_register, (uint32_t)write_data);
}

void mii_mgr_read_adpt(uint32_t phy_addr, uint8_t phy_register, uint16_t *read_data)
{
	uint32_t tmp_read_data = 0;

	mii_mgr_read(phy_addr, (uint32_t)phy_register, &tmp_read_data);
	*read_data = tmp_read_data;
}


/******************************************************************************
 * FUNCTION DESCRIPTION: Read switch internal register.
 *                       Switch internal register is accessed through the
 *                       MDIO interface. MDIO access is only 16 bits wide so
 *                       it needs the two time access to complete the internal
 *                       register access.
 * INPUT               : register address
 * OUTPUT              : Register value
 *
 *****************************************************************************/
uint32_t athrs17_reg_read(uint32_t reg_addr)
{
	uint32_t reg_word_addr;
	uint32_t phy_addr, reg_val;
	uint16_t phy_val;
	uint16_t tmp_val;
	uint8_t phy_reg;

	/* change reg_addr to 16-bit word address, 32-bit aligned */
	reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

	/* configure register high address */
	phy_addr = 0x18;
	phy_reg = 0x0;
	phy_val = (uint16_t) ((reg_word_addr >> 8) & 0x1ff);  /* bit16-8 of reg address */
	mii_mgr_write_adpt(phy_addr, phy_reg, phy_val);

	/*
	 * For some registers such as MIBs, since it is read/clear, we should
	 * read the lower 16-bit register then the higher one
	 */

	/* read register in lower address */
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	mii_mgr_read_adpt(phy_addr, phy_reg, &phy_val);

	/* read register in higher address */
	reg_word_addr++;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	mii_mgr_read_adpt(phy_addr, phy_reg, &tmp_val);
	reg_val = (tmp_val << 16 | phy_val);

	return reg_val;
}

/******************************************************************************
 * FUNCTION DESCRIPTION: Write switch internal register.
 *                       Switch internal register is accessed through the
 *                       MDIO interface. MDIO access is only 16 bits wide so
 *                       it needs the two time access to complete the internal
 *                       register access.
 * INPUT               : register address, value to be written
 * OUTPUT              : NONE
 *
 *****************************************************************************/
void athrs17_reg_write(uint32_t reg_addr, uint32_t reg_val)
{
	uint32_t reg_word_addr;
	uint32_t phy_addr;
	uint16_t phy_val;
	uint8_t phy_reg;

	/* change reg_addr to 16-bit word address, 32-bit aligned */
	reg_word_addr = (reg_addr & 0xfffffffc) >> 1;

	/* configure register high address */
	phy_addr = 0x18;
	phy_reg = 0x0;
	phy_val = (uint16_t) ((reg_word_addr >> 8) & 0x1ff);  /* bit16-8 of reg address */
	mii_mgr_write_adpt(phy_addr, phy_reg, phy_val);

	/* write register in lower address */
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	phy_val = (uint16_t) (reg_val & 0xffff);
	mii_mgr_write_adpt(phy_addr, phy_reg, phy_val);

	/* read register in higher address */
	reg_word_addr++;
	phy_addr = 0x10 | ((reg_word_addr >> 5) & 0x7); /* bit7-5 of reg address */
	phy_reg = (uint8_t) (reg_word_addr & 0x1f);   /* bit4-0 of reg address */
	phy_val = (uint16_t) ((reg_val >> 16) & 0xffff);
	mii_mgr_write_adpt(phy_addr, phy_reg, phy_val);


}

int athrs17_phy_read(unsigned int phy_addr, unsigned int reg, unsigned short* data)
{
	unsigned int registerData = 0;
	registerData = 0xc8000000 | (phy_addr & 0x1f) << 21 | (reg & 0x1f) << 16;

	while(athrs17_reg_read(0x3c) & 0x80000000);
	athrs17_reg_write(0x3c, registerData);

	do
	{
		registerData = athrs17_reg_read(0x3c);
	}while(registerData & 0x80000000);

	*data = registerData & 0xffff;

	return 0;
}

int athrs17_phy_write(unsigned int phy_addr, unsigned int reg, unsigned short data)
{
	unsigned int registerData = 0;

	registerData = 0xc0000000 | (phy_addr & 0x1f) << 21 | (reg & 0x1f) << 16 | (data & 0xffff);
	while(athrs17_reg_read(0x3c) & 0x80000000);
	athrs17_reg_write(0x3c, registerData);
	return 0;
}

void athrs17_phy_dbg_write(unsigned int phy_addr, unsigned short dbg_addr, unsigned short dbg_data)
{
	athrs17_phy_write(phy_addr, S17C_MII_DBG_ADDR, dbg_addr);
	athrs17_phy_write(phy_addr, S17C_MII_DBG_DATA, dbg_data);
}

void athrs17_phy_dbg_read(unsigned int phy_addr, unsigned short dbg_addr, unsigned short *dbg_data)
{
	athrs17_phy_write(phy_addr, S17C_MII_DBG_ADDR, dbg_addr);
	athrs17_phy_read(phy_addr, S17C_MII_DBG_DATA, dbg_data);
}

