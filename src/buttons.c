#include "buttons.h"

#include <zephyr/drivers/gpio.h>

#define BUTTON_COUNT 4
static const struct gpio_dt_spec buttons[BUTTON_COUNT] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw3), gpios, {0})};
static struct gpio_callback button_cb_data;
static button_callback_t button_callback = NULL;
void button_pressed(const struct device *dev, struct gpio_callback *cb,
                    uint32_t pins)
{
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        if ((pins & BIT(buttons[i].pin)) != 0)
        {
            if (button_callback) {
                button_callback(i);
            }
        }
    }
}

void init_buttons(button_callback_t cb)
{
    for (int i = 0; i < BUTTON_COUNT; i++)
    {
        __ASSERT_NO_MSG(device_is_ready(buttons[i].port));
        int ret = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
        __ASSERT_NO_MSG(ret == 0);
        ret = gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
        __ASSERT_NO_MSG(ret == 0);
    }
    gpio_init_callback(
        &button_cb_data,
        button_pressed,
        BIT(buttons[0].pin) | BIT(buttons[1].pin) | BIT(buttons[2].pin) | BIT(buttons[3].pin));
    int ret = gpio_add_callback(buttons[0].port, &button_cb_data);
    __ASSERT_NO_MSG(ret == 0);

    button_callback = cb;
}