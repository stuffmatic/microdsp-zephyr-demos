#ifndef LEDS_H
#define LEDS_H

#include "leds.h"
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

static const struct gpio_dt_spec leds[4] = {
    GPIO_DT_SPEC_GET(LED0_NODE, gpios),
    GPIO_DT_SPEC_GET(LED1_NODE, gpios),
    GPIO_DT_SPEC_GET(LED2_NODE, gpios),
    GPIO_DT_SPEC_GET(LED3_NODE, gpios),
};

int init_leds()
{
    for (uint32_t i = 0; i < 4; i++)
    {
        gpio_pin_configure_dt(&leds[i], GPIO_OUTPUT_ACTIVE);
	    gpio_pin_set_dt(&leds[i], 0);
    }
}

int set_led_state(int led_index, int is_on)
{
    if (led_index >= 0 || led_index < 4) {
        gpio_pin_set_dt(&leds[led_index], is_on);
    }
    // printk("Setting led %d to %d\n", led_index, is_on);
}

#endif