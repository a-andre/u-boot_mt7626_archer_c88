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
	GPIO_NUM_RESET_BUTTON = 21,
	GPIO_NUM_SWITCH_INT = 61,
	GPIO_NUM_SYS_ORANGE = 58, /*!< sys led orange: GPIO_NUM_SYS_ORANGE high, GPIO_NUM_SYS_GREEN low.
									sys led green: GPIO_NUM_SYS_ORANGE low, GPIO_NUM_SYS_GREEN high */
	GPIO_NUM_SYS_GREEN = 56,

	GPIO_NUM_SWITCH_RESET = 60,
};

enum {
	GPIO_LED_OFF = 0,
	GPIO_LED_ON = 1,
	GPIO_LED_ON_2 = 2,		/*!< second color, for 2-color LED */

	GPIO_LED_ON_ORANGE = GPIO_LED_ON,
	GPIO_LED_ON_GREEN = GPIO_LED_ON_2,

	GPIO_LED_ON_OK = GPIO_LED_ON_GREEN, /*!<	color which indicates OK state */
};

enum {
	GPIO_LEVEL_LOW = 0,
	GPIO_LEVEL_HIGH = 1,
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
		.gpioNum = GPIO_NUM_SYS_ORANGE,
		.gpioDir = GPIO_DIR_OUT,
	},
	{
		.gpioNum = GPIO_NUM_SYS_GREEN,
		.gpioDir = GPIO_DIR_OUT,
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

		/* the level get here indicates GPIO_LED_ON */
		level = gpioInits[i].gpioLevel;
		
		if (value == GPIO_LED_OFF)		{
			if (level == GPIO_LEVEL_LOW)
				level = GPIO_LEVEL_HIGH;
			else
				level = GPIO_LEVEL_LOW;
		}
			
		mt_set_gpio_out(gpioInits[i].gpioNum, level);
		return;
	}

	return;
}

/* only for rtl8367s */
void external_switch_reset(void)
{

	/* pull low to reset */
	mt_set_gpio_out(GPIO_NUM_SWITCH_RESET, GPIO_LEVEL_LOW);
	/* sleep 50ms(datasheet demand >10ms) */
	mdelay(50);
	/* pull high */
	mt_set_gpio_out(GPIO_NUM_SWITCH_RESET, GPIO_LEVEL_HIGH);

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

static void sys_led_on(int on)
{
	int green = GPIO_LEVEL_LOW;
	int orange = GPIO_LEVEL_LOW;
	
	if (on == GPIO_LED_OFF)
	{
		orange = GPIO_LEVEL_HIGH;
		green = GPIO_LEVEL_HIGH;
		mt_set_gpio_out(GPIO_NUM_SYS_ORANGE, orange);

		mt_set_gpio_dir(GPIO_NUM_SYS_GREEN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_NUM_SYS_GREEN, green);		
	}
	else if (on == GPIO_LED_ON_ORANGE)
	{
		/* orange */
		orange = GPIO_LEVEL_HIGH;
		mt_set_gpio_out(GPIO_NUM_SYS_ORANGE, orange);

		/* 高阻态 */
		mt_set_gpio_dir(GPIO_NUM_SYS_GREEN, GPIO_DIR_IN);
	}
	else if (on == GPIO_LED_ON_GREEN)
	{
		/* green */
		orange = GPIO_LEVEL_LOW;
		green = GPIO_LEVEL_HIGH;
		mt_set_gpio_out(GPIO_NUM_SYS_ORANGE, orange);

		mt_set_gpio_dir(GPIO_NUM_SYS_GREEN, GPIO_DIR_OUT);
		mt_set_gpio_out(GPIO_NUM_SYS_GREEN, green);
	}
	else
	{
		return;
	}

}

/*!
	@brief			Turn on or off sys LED
*/
void set_sys_led(int on)
{
	/* display 'OK' color */
	if (on)
		sys_led_on(GPIO_LED_ON_OK);
	else
		sys_led_on(GPIO_LED_OFF);
}

/*!
	@brief			turn on all LED.
					If 2-color LED exists, use its 'OK' color.
					For orange-green color led, green indicates 'OK'.
*/
void all_leds_on(int on)
{
	int i = 0;

	for (i = 0; i < NELEMENTS(gpioInits); i++)
	{
		if (gpioInits[i].isLed)
			setLed(gpioInits[i].gpioNum, GPIO_LED_ON);
	}

	/* process 2-color sys led, light OK color green */
	sys_led_on(GPIO_LED_ON_OK);


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
			setLed(gpioInits[i].gpioNum, GPIO_LED_ON);
	}

	/* process 2-color WAN led, light green */
	sys_led_on(GPIO_LED_ON_2);

	mdelay(500);

	/* process 2-color WAN led, light orange */
	sys_led_on(GPIO_LED_ON);

	mdelay(500);

	/* turn off all led */
	for (i = 0; i < NELEMENTS(gpioInits); i++)
	{
		if (gpioInits[i].isLed)
			setLed(gpioInits[i].gpioNum, GPIO_LED_OFF);
	}

	/* process 2-color WAN led, light off */
	sys_led_on(GPIO_LED_OFF);
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
