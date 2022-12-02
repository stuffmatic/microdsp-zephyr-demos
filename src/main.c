#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>

#include "audio_callbacks.h"
#include "audio_cfg.h"
#include "i2s.h"

typedef struct
{
    float phase;
    float dphase;
} oscillator_state_t;

#define OSC_FREQ_HZ 350.0
#define OSC_GAIN ((float)0.01)
static void processing_cb(void *cb_data, uint32_t frame_count, uint32_t channel_count, float *tx, const float *rx)
{
    oscillator_state_t *osc_state = (oscillator_state_t *)cb_data;
    for (int i = 0; i < frame_count; i++)
    {
        float x = osc_state->phase > 1 ? osc_state->phase - 2 : osc_state->phase;
        float sign = osc_state->phase > 1 ? -1 : 1;

        float x_sq = x * x;
        float x_qu = x_sq * x_sq;
        float value = ((float)0.2146) * x_qu - 1.214601836f * x_sq + 1.0f;

        osc_state->phase += osc_state->dphase;

        if (osc_state->phase > 3.0) {
            osc_state->phase -= 4.;
        }

        tx[channel_count * i] = OSC_GAIN * value * sign;
        tx[channel_count * i + 1] = OSC_GAIN * value * sign;
    }
}

static void dropout_cb(void *data)
{
    printk("dropout!\n");
}

static oscillator_state_t oscillator_state = {
    .phase = -1,
    .dphase = 0
};

#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
// #define LED0_NODE DT_ALIAS(led0)

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
    oscillator_state.dphase = 4 * OSC_FREQ_HZ / audio_cfg.sample_rate;
    audio_cfg.init_codec();

    audio_callbacks_t audio_callbacks = {
        .dropout_cb = dropout_cb,
        .processing_cb = processing_cb,
        .cb_data = &oscillator_state,
    };

    i2s_start(&audio_cfg, &audio_callbacks);

    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
