/******************************************************************************
 * Copyright (C) 2015-2018, TP-LINK TECHNOLOGIES CO., LTD.
 *
 * File name:     switch_ctrl.c
 * Version:       1.0
 * Description:   switch gpio 通用接口定义（gpio reset）
 *
 * Authors:       Lin Qi <qilin@tp-link.com.cn>
 ****************************************************************************/

#include <common.h>
#include <config.h>

#include <asm/arch/timer.h>
#include <asm/arch/leopard.h>
#include <asm/arch/gpio.h>

void switch_reset_gpio(unsigned int gpio, unsigned int time_gap_ms)
{
	mt_set_gpio_mode(gpio, GPIO_MODE_GPIO);
	mt_set_gpio_dir(gpio, GPIO_DIR_OUT);

#ifdef SWITCH_RESET_GPIO_INV /* 极性翻转，高电平reset */
	/* pull high to reset */
	mt_set_gpio_out(gpio, GPIO_OUT_ONE);

	/* sleep for a while, definite determined by hw */
	mdelay(time_gap_ms);

	/* pull low */
	mt_set_gpio_out(gpio, GPIO_OUT_ZERO);

#else
	/* pull low to reset */
	mt_set_gpio_out(gpio, GPIO_OUT_ZERO);

	/* sleep for a while, definite determined by hw */
	mdelay(time_gap_ms);

	/* pull high */
	mt_set_gpio_out(gpio, GPIO_OUT_ONE);
#endif
}

