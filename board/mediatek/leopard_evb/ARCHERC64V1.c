/*
 * (C) Copyright 2018 TP-LINK.Inc
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

#include <common.h>
#include <config.h>

#include <asm/arch/timer.h>
#include <asm/arch/leopard.h>
#include <asm/arch/gpio.h>
#include <asm/arch/led.h>

#define NELEMENTS(_arr) (sizeof(_arr)/sizeof(_arr[0]))
typedef struct
{
	int gpioNum;			/*!< gpio num. 
								note that this num is not the same as pin num. 
								Should reference the datasheet */
	int gpioDir;			/*!< gpio dir, GPIO_DIR_OUT or GPIO_DIR_IN */
	int gpioLevel;			/*!< enable level, default is 0 - low level */
	int isLed;				/*!< if this gpio is mapped to a LED */
	int isButton;			/*!< if this gpio is mapped to a button */
}
GPIO_INIT;

enum {
	GPIO_NUM_RESET_BUTTON = 53,
	GPIO_NUM_SWITCH_INT = 61,
	GPIO_NUM_WAN_ORANGE = 55, /*!< wan led orange: GPIO_NUM_WAN_ORANGE high, GPIO_NUM_WAN_GREEN low.
									wan led green: GPIO_NUM_WAN_ORANGE low, GPIO_NUM_WAN_GREEN high */
	GPIO_NUM_WAN_GREEN = 21,
	GPIO_NUM_LAN_LED = 56,
	GPIO_NUM_5G_LED = 57,
	GPIO_NUM_2G_LED = 58,
	GPIO_NUM_SYS_LED = 59,
	GPIO_NUM_SWITCH_RESET = 60,
};

GPIO_INIT gpioInits[] = {
	{
		.gpioNum = GPIO_NUM_RESET_BUTTON,
		.gpioDir = GPIO_DIR_IN,
		.isButton = 1,
	},
	{
		.gpioNum = GPIO_NUM_SWITCH_INT,
		.gpioDir = GPIO_DIR_IN,
	},
	{
		.gpioNum = GPIO_NUM_WAN_ORANGE,
		.gpioDir = GPIO_DIR_OUT,
	},
	{
		.gpioNum = GPIO_NUM_WAN_GREEN,
		.gpioDir = GPIO_DIR_OUT,
	},
	{
		.gpioNum = GPIO_NUM_LAN_LED,
		.gpioDir = GPIO_DIR_OUT,
		.isLed = 1,
	},
	{
		.gpioNum = GPIO_NUM_5G_LED,
		.gpioDir = GPIO_DIR_OUT,
		.isLed = 1,
	},
	{
		.gpioNum = GPIO_NUM_2G_LED,
		.gpioDir = GPIO_DIR_OUT,
		.isLed = 1,
	},
	{
		.gpioNum = GPIO_NUM_SYS_LED,
		.gpioDir = GPIO_DIR_OUT,
		.isLed = 1,
	},
	{
		.gpioNum = GPIO_NUM_SWITCH_RESET,
		.gpioDir = GPIO_DIR_OUT,
	},
};


void setLed(ulong num, ulong value)
{
	int i = 0;
	int level = 0;

	for (i = 0; i < NELEMENTS(gpioInits); i++)
	{
		if (gpioInits[i].gpioNum != num)
			continue;

		if (!gpioInits[i].isLed)
			return;
			
		level = gpioInits[i].gpioLevel;
		if (value == 0)
			level = 1 - level;
			
		mt_set_gpio_out(gpioInits[i].gpioNum, level);
		return;
	}

	return;
}

/* only for rtl8367s */
void external_switch_reset(void)
{

	/* pull low to reset */
	mt_set_gpio_out(GPIO_NUM_SWITCH_RESET, 0);
	/* sleep 50ms(datasheet demand >10ms) */
	mdelay(50);
	/* pull high */
	mt_set_gpio_out(GPIO_NUM_SWITCH_RESET, 1);

}

void board_gpio_init(void)
{
	int i = 0;

	for (i = 0; i < NELEMENTS(gpioInits); i++)
	{
		mt_set_gpio_mode(gpioInits[i].gpioNum, GPIO_MODE_GPIO);
		mt_set_gpio_dir(gpioInits[i].gpioNum, gpioInits[i].gpioDir);
	}
}


/*!
	@brief			Turn on or off sys LED
*/
void set_sys_led(int on)
{
	
	setLed(GPIO_NUM_SYS_LED, on);
	
}

/*!
	@brief			turn on all LED.
					If 2-color LED exists, use its 'OK' color.
*/
void all_leds_on(int on)
{
	int i = 0;

	for (i = 0; i < NELEMENTS(gpioInits); i++)
	{
		if (gpioInits[i].isLed)
			setLed(gpioInits[i].gpioNum, 1);
	}

	/* process 2-color WAN led, light green */
	mt_set_gpio_out(GPIO_NUM_WAN_ORANGE, 0);
	mt_set_gpio_out(GPIO_NUM_WAN_GREEN, 1);

}

static void wan_led_on(int on)
{
	/*
	 *  绿灯：
	 *      GPIO21_DOUT9 = 1;
	 *      GPIO55_DOUT2 = 0;
	 *  橙灯：
	 *      GPIO21_DOUT9_DIR = INPUT;
	 *      GPIO55_DOUT2 = 1;
	 *  熄灭：
	 *      GPIO21_DOUT9 = 0;
	 *      GPIO55_DOUT2 = 0; 
	 */

	if (on == 0)
	{
		mt_set_gpio_dir(GPIO_NUM_WAN_GREEN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_NUM_WAN_ORANGE, 0);
		mt_set_gpio_out(GPIO_NUM_WAN_GREEN, 0);
	}
	else if (on == 1)
	{
		mt_set_gpio_dir(GPIO_NUM_WAN_GREEN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_NUM_WAN_ORANGE, 0);
		mt_set_gpio_out(GPIO_NUM_WAN_GREEN, 1);
	}
	else if (on == 2)
	{
		mt_set_gpio_dir(GPIO_NUM_WAN_GREEN, GPIO_DIR_IN);
		mt_set_gpio_out(GPIO_NUM_WAN_ORANGE, 1);
	}
	else
	{
		return;
	}
}
/*!
	@brief			Turn on all LEDs for 1s and turn them off to check LED lighting in sys init.
					If 2-color LED(s) exist(s), this LED(s) should light all colors in sequence.
*/
void all_leds_check(void)
{
	int i = 0;

	for (i = 0; i < NELEMENTS(gpioInits); i++)
	{
		if (gpioInits[i].isLed)
			setLed(gpioInits[i].gpioNum, 1);
	}

	/* process 2-color WAN led, light green */
	wan_led_on(1);

	mdelay(500);

	/* process 2-color WAN led, light orange */
	wan_led_on(2);

	mdelay(500);

	/* turn off all led */
	for (i = 0; i < NELEMENTS(gpioInits); i++)
	{
		if (gpioInits[i].isLed)
			setLed(gpioInits[i].gpioNum, 0);
	}

	/* process 2-color WAN led, light off */
	wan_led_on(0);
}


static int is_button_pressed(int num)
{
	int i = 0;
	int level = 0;
	for (i = 0; i < NELEMENTS(gpioInits); i++)
	{
		if (gpioInits[i].gpioNum != num)
			continue;
			
		if (!gpioInits[i].isButton)
			return 0;
	
		if ((level = mt_get_gpio_in(num)) < 0)
			return 0;

		if (gpioInits[i].gpioLevel == level)
			return 1;

		return 0;
	}

	return 0;
}


/*!
	@brief		check if reset button is pressed
*/
int is_reset_button_pressed(void)
{
	return is_button_pressed(GPIO_NUM_RESET_BUTTON);

}
