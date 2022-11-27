/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <rust_lib.h>

#include "audio_processing.h"
#include "codec.h"
#include "i2s_nrfx.h"

typedef struct
{
    float phase;
    float dphase;
} oscillator_state_t;

#define OSC_FREQ_HZ 350.0
#define OSC_GAIN 0.01
#define SAMPLE_RATE 44100.0
static void processing_cb(void *data, uint32_t frame_count, uint32_t channel_count, float *tx, const float *rx)
{
    oscillator_state_t *osc_state = (oscillator_state_t *)data;
    for (int i = 0; i < frame_count; i++)
    {
        if (true) {
            float x = osc_state->phase > 1 ? osc_state->phase - 2 : osc_state->phase;
            float sign = osc_state->phase > 1 ? -1 : 1;

            float x_sq = x * x;
            float x_qu = x_sq * x_sq;
            float value = ((float)0.2146) * x_qu - ((float)1.214601836) * x_sq + 1;

            osc_state->phase += osc_state->dphase;

            if (osc_state->phase > 3.0) {
                osc_state->phase -= 4.; // TODO: make this work for (very) high freq? Or limit d_phase?
            }

            tx[channel_count * i] = OSC_GAIN * value * sign;
            tx[channel_count * i + 1] = OSC_GAIN * value * sign;
        }
        else
        {
            const float a = osc_state->phase < 0 ? 1 : -1;
            float phase = osc_state->phase;
            float value = OSC_GAIN * (phase + a * phase * phase);
            tx[channel_count * i] = value;
            tx[channel_count * i + 1] = value;
            phase += osc_state->dphase;
            if (phase > 1)
            {
                phase -= 2;
            }
            osc_state->phase = phase;
        }
    }
}

static void dropout_cb()
{
    printk("dropout!\n");
}

static oscillator_state_t oscillator_state = {
    .phase = -1,
    .dphase = 2 * OSC_FREQ_HZ / SAMPLE_RATE
};

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

/* The devicetree node identifier for the "led0" alias. */
// #define LED0_NODE DT_ALIAS(led0)

// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
    int test = rust_test_fn();
    printk("rust_test_fn returned %d\n", test);

    init_wm8904_codec();

    audio_processing_options_t processing_options = {
        .dropout_cb = dropout_cb,
        .processing_cb = processing_cb,
        .processing_cb_data = &oscillator_state};
    i2s_start(&processing_options);

    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
