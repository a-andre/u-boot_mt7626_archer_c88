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
#ifndef _LED_DEFINITION_H_
#define _LED_DEFINITION_H_


void mt7626_led_gpio_init(void);
void mt7626_ephy_led_init(void);
#ifdef CONFIG_GE1_SGMII_FORCE_2500
void mt7626_switch_led_init(void);
#endif
void set_sys_led(int on);
void all_leds_on(int on);
#endif