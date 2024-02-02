/*
 * Copyright (c) 2015-2016, 2020 The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * Manage the atheros ethernet PHY.
 *
 * All definitions in this file are operating system independent!
 */

#include <common.h>
#include "athrs17_phy.h"
#include "athrs17_mii.h"

#define		ATHRS17_DEVICE_ID	0x13

extern uint32_t athrs17_reg_read(uint32_t reg_addr);
extern void athrs17_reg_write(uint32_t reg_addr, uint32_t reg_val);
extern int athrs17_phy_read(unsigned int phy_addr, unsigned int reg, unsigned short* data);
extern int athrs17_phy_write(unsigned int phy_addr, unsigned int reg, unsigned short data);
extern void athrs17_phy_dbg_write(unsigned int phy_addr, unsigned short dbg_addr, unsigned short dbg_data);
extern void athrs17_phy_dbg_read(unsigned int phy_addr, unsigned short dbg_addr, unsigned short *dbg_data);
extern void switch_reset_gpio(unsigned int gpio, unsigned int time_gap_ms);

static void athrs17_phy_linkdown_all(void)
{
	int i;
	unsigned short phy_val;

	for (i = 0; i < ATHRS17_PHY_NUM; i++)
	{
		athrs17_phy_write(i, 0x0, 0x0800);	// phy powerdown

		athrs17_phy_dbg_read(i, 0x3d, &phy_val);
		phy_val &= ~0x0040;
		athrs17_phy_dbg_write( i, 0x3d, phy_val);

		/*PHY will stop the tx clock for a while when link is down
			1. en_anychange  debug port 0xb bit13 = 0  //speed up link down tx_clk
			2. sel_rst_80us  debug port 0xb bit10 = 0  //speed up speed mode change to 2'b10 tx_clk
		*/
		athrs17_phy_dbg_read(i, 0xb, &phy_val);
		phy_val &= ~0x2400;
		athrs17_phy_dbg_write(i, 0xb, phy_val);

		mdelay(100);
	}
}

#ifdef SWITCH_NEED_PHY_DOWN_AFTER_GPIO_RESET
void switch_phy_linkdown_all(void)
{
	athrs17_phy_linkdown_all();
}
#endif

static void athrs17_phy_enable(void)
{
	int i = 0;

	for (i = 0; i < ATHRS17_PHY_NUM; i++) {
		unsigned short value = 0;

		/* start autoneg*/
		athrs17_phy_write(i, MII_ADVERTISE, ADVERTISE_ALL |
						     ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);
		//phy reg 0x9, b10,1 = Prefer multi-port device (master)
		athrs17_phy_write(i, MII_CTRL1000, (0x0400 | ADVERTISE_1000FULL));

		athrs17_phy_write(i, MII_BMCR, BMCR_RESET | BMCR_ANENABLE);

		athrs17_phy_dbg_read(i, 0, &value);
		value &= (~(1<<12));
		athrs17_phy_dbg_write(i, 0, value);

		mdelay(100);
	}
}

/*********************************************************************
 * FUNCTION DESCRIPTION: set port isolation by port's vlan member(PXLOOKUP_CTRL_REG)
 * INPUT : NONE
 * OUTPUT: NONE
 *********************************************************************/
void athrs17_port_isolation_config(void)
{
	unsigned int i = 0;
	unsigned int port_lookup_ctrl_base = S17_P0LOOKUP_CTRL_REG;
	unsigned int port_vlan_ctrl0_base = S17_P0VLAN_CTRL0_REG;
	unsigned int port_vlan_ctrl1_base = S17_P0VLAN_CTRL1_REG;

	/* 都使用port vlan 1，用port vlan member做端口隔离 */
	unsigned int vid = 1;

	/* port0连RGMII，故这里从port1开始 */
	for (i = ATHRS17_PORT1; i < ATHRS17_PORT_END; i++)
	{
		if (i == ATHRS17_CPU_PORT)
		{
			athrs17_reg_write(port_lookup_ctrl_base + (i * 0xc),
				PORT_VID_MEM_ALL_PORTS_WITHOUT_CPU | PORT_STATE_FORWARD | PORT_LEARN_ENABLE | FORCE_PORT_VLAN_EN);
		}
		else
		{
			athrs17_reg_write(port_lookup_ctrl_base + (i * 0xc),
				PORT_VID_MEM_ONLY_CPU | PORT_STATE_FORWARD | PORT_LEARN_ENABLE | FORCE_PORT_VLAN_EN);
		}

		athrs17_reg_write(port_vlan_ctrl0_base + (i * 0x8),
			(vid << PORT_DEFAULT_SVID_OFFSET) | (vid << PORT_DEFAULT_CVID_OFFSET));

		athrs17_reg_write(port_vlan_ctrl1_base + (i * 0x8),
			PORT_VLAN_PROP_EN | PORT_EGRESS_VLAN_MODE_UNTAGED);
	}

	printf("%s ...done\n", __func__);
}

/*******************************************************************
* FUNCTION DESCRIPTION: Reset S17 register
* INPUT: NONE
* OUTPUT: NONE
*******************************************************************/
int athrs17_init_switch(void)
{
	uint32_t data;
	uint32_t i = 0;

	/* Reset the switch before initialization */
	athrs17_reg_write(S17_MASK_CTRL_REG, S17_MASK_CTRL_SOFT_RET);
	do {
		udelay(10);
		data = athrs17_reg_read(S17_MASK_CTRL_REG);
		i++;
		if (i == 10){
			printf("Failed to reset S17C \n");
			return -1;
		}
	} while (data & S17_MASK_CTRL_SOFT_RET);

	i = 0;

	do {
		udelay(10);
		data = athrs17_reg_read(S17_GLOBAL_INT0_REG);
		i++;
		if (i == 10)
			return -1;
	} while ((data & S17_GLOBAL_INITIALIZED_STATUS) != S17_GLOBAL_INITIALIZED_STATUS);

	return 0;
}

/*********************************************************************
 * FUNCTION DESCRIPTION: Configure S17 register
 * INPUT : NONE
 * OUTPUT: NONE
 *********************************************************************/
void athrs17_rgmii_reg_init(void)
{
	athrs17_reg_write(S17_MAC_PWR_REG, 0); /* 疑问，这里默认传0，具体原因不详 */

	athrs17_reg_write(S17_P0STATUS_REG, (S17_SPEED_1000M |
						S17_TXMAC_EN |
						S17_RXMAC_EN |
						S17_DUPLEX_FULL));

	athrs17_reg_write(S17_GLOFW_CTRL1_REG, (S17_IGMP_JOIN_LEAVE_DPALL |
						S17_BROAD_DPALL |
						S17_MULTI_FLOOD_DPALL |
						S17_UNI_FLOOD_DPALL));

	athrs17_reg_write(S17_P5PAD_MODE_REG, S17_MAC0_RGMII_RXCLK_DELAY);

	athrs17_reg_write(S17_P0PAD_MODE_REG, (S17_MAC0_RGMII_EN |
						S17_MAC0_RGMII_TXCLK_DELAY |
						S17_MAC0_RGMII_RXCLK_DELAY |
					(0x1 << S17_MAC0_RGMII_TXCLK_SHIFT) |
					(0x2 << S17_MAC0_RGMII_RXCLK_SHIFT)));

	printf("%s: complete\n", __func__);
}

/*********************************************************************
 * FUNCTION DESCRIPTION: Configure S17 register
 * INPUT : NONE
 * OUTPUT: NONE
 *********************************************************************/
void athrs17_sgmii_reg_init(void)
{
	uint32_t reg_val;

	athrs17_reg_write(S17_P6STATUS_REG, (S17_SPEED_1000M |
						S17_TXMAC_EN |
						S17_RXMAC_EN |
						S17_DUPLEX_FULL));

	athrs17_reg_write(S17_MAC_PWR_REG, 0); /* 疑问，这里默认传0，具体原因不详 */
	reg_val = athrs17_reg_read(S17_P6PAD_MODE_REG);
	athrs17_reg_write(S17_P6PAD_MODE_REG, (reg_val | S17_MAC6_SGMII_EN));

	athrs17_reg_write(S17_PWS_REG, 0x2613a0);

	athrs17_reg_write(S17_SGMII_CTRL_REG,(S17c_SGMII_EN_PLL |
					S17c_SGMII_EN_RX |
					S17c_SGMII_EN_TX |
					S17c_SGMII_EN_SD |
					S17c_SGMII_BW_HIGH |
					S17c_SGMII_SEL_CLK125M |
					S17c_SGMII_TXDR_CTRL_600mV |
					S17c_SGMII_CDR_BW_8 |
					S17c_SGMII_DIS_AUTO_LPI_25M |
					S17c_SGMII_MODE_CTRL_SGMII_PHY |
					S17c_SGMII_PAUSE_SG_TX_EN_25M |
					S17c_SGMII_ASYM_PAUSE_25M |
					S17c_SGMII_PAUSE_25M |
					S17c_SGMII_HALF_DUPLEX_25M |
					S17c_SGMII_FULL_DUPLEX_25M));

	athrs17_reg_write(S17_MODULE_EN_REG, S17_MIB_COUNTER_ENABLE);
}

struct athrs17_regmap {
	uint32_t start;
	uint32_t end;
};

struct athrs17_regmap regmap[] = {
	{ 0x000,  0x0e4  },
	{ 0x100,  0x168  },
	{ 0x200,  0x270  },
	{ 0x400,  0x454  },
	{ 0x600,  0x718  },
	{ 0x800,  0xb70  },
	{ 0xC00,  0xC80  },
	{ 0x1100, 0x11a7 },
	{ 0x1200, 0x12a7 },
	{ 0x1300, 0x13a7 },
	{ 0x1400, 0x14a7 },
	{ 0x1600, 0x16a7 },
};

int do_ar8xxx_dump(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]) {
	int i;

	for (i = 0; i < ARRAY_SIZE(regmap); i++) {
		uint32_t reg;
		struct athrs17_regmap *section = &regmap[i];

		for (reg = section->start; reg <= section->end; reg += sizeof(uint32_t)) {
			uint32_t val = athrs17_reg_read(reg);
			printf("%03zx: %08zx\n", reg, val);
		}
	}

	return 0;
};

int athrs17_init(void)
{
	int ret = 0;

	switch_reset_gpio(GPIO_RESET_SWITCH, GPIO_RESET_TIME_GAP_MS);

	/*
	 * delay for a while after switch reset
	 * for some reason, immediately do switch init may fail after switch reset
	 */
	mdelay(20);

	athrs17_phy_linkdown_all();

	ret = athrs17_init_switch();
	if (ret != -1) {
		athrs17_sgmii_reg_init();     /* init SGMII for lan */
		printf ("S17c init  done\n");
	}

	athrs17_phy_enable();

	return ret;
}

int athrs17_switch_chip_probe(void)
{
	int result = 0;
	unsigned int value;
	value = (athrs17_reg_read(S17_MASK_CTRL_REG) & 0x0000FF00) >> 8;
	if(value == ATHRS17_DEVICE_ID)
		result = 1;
	return result;
}

