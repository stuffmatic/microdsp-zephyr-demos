#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <sys/ring_buffer.h>
#include <microdsp_demo/microdsp_demo.h>
#include <stdlib.h>

#include "audio_callbacks.h"
#include "audio_cfg.h"
#include "buttons.h"
#include "i2s.h"
#include "leds.h"

#define OSC_FREQ_HZ 350.0f
#define OSC_GAIN 0.01f
static float phase_debug = 0;
static float dphase_debug = 0;

#define APP_MSG_BUFFER_SIZE 64
typedef struct
{
    void *rust_app_ptr;
    uint8_t msg_rx_buffer[APP_MSG_BUFFER_SIZE];
    uint8_t msg_tx_buffer[APP_MSG_BUFFER_SIZE];
    struct ring_buf msg_rx;
    struct ring_buf msg_tx;
} demo_app_t;

static demo_app_t demo_app;

static void processing_cb(void *cb_data, uint32_t sample_count, float *tx, const float *rx)
{
    demo_app_t *demo_app = (demo_app_t *)cb_data;
    uint8_t command = 0;
    while (ring_buf_get(&demo_app->msg_rx, &command, 1) > 0)
    {
        demo_app_handle_message(demo_app->rust_app_ptr, command);
    }

    if (true)
    {
        demo_app_process(demo_app->rust_app_ptr, tx, rx, sample_count);
    }
    else
    {
        for (int i = 0; i < sample_count; i++)
        {
            float x = phase_debug > 1.0f ? phase_debug - 2 : phase_debug;
            float sign = phase_debug > 1.0f ? -1.0f : 1.0f;

            float x_sq = x * x;
            float x_qu = x_sq * x_sq;
            float value = 0.2146f * x_qu - 1.214601836f * x_sq + 1.0f;

            phase_debug += dphase_debug;

            if (phase_debug > 3.0f)
            {
                phase_debug -= 4.f;
            }
            float out_sample = OSC_GAIN * value * sign;
            tx[i] = out_sample;
        }
    }
}

static void dropout_cb(void *data)
{
    printk("dropout!\n");
}

#define POLL_INTERVAL_MS 10

void button_callback(int btn_idx, int is_down) {
    uint8_t msg = 0;

    switch (btn_idx) {
        case 0:
            msg = is_down ? Button0Down : Button0Up;
            break;
        case 1:
            msg = is_down ? Button1Down : Button1Up;
            break;
        case 2:
            msg = is_down ? Button2Down : Button2Up;
            break;
        case 3:
            msg = is_down ? Button3Down : Button3Up;
            break;
    }

    if (msg) {
        ring_buf_put(&demo_app.msg_tx, &msg, 1);
    }
}

void main(void)
{
    // init_leds();
    // init_buttons(&button_callback);
    printk("before demo_app_create\n");
    demo_app.rust_app_ptr = demo_app_create(audio_cfg.sample_rate);
    printk("after demo_app_create\n");
    dphase_debug = 4.0f * OSC_FREQ_HZ / audio_cfg.sample_rate;
    ring_buf_init(&demo_app.msg_rx, ARRAY_SIZE(demo_app.msg_rx_buffer), demo_app.msg_rx_buffer);
    ring_buf_init(&demo_app.msg_tx, ARRAY_SIZE(demo_app.msg_tx_buffer), demo_app.msg_tx_buffer);
    audio_cfg.init_codec();

    audio_callbacks_t audio_callbacks = {
        .dropout_cb = dropout_cb,
        .processing_cb = processing_cb,
        .cb_data = &demo_app,
    };

    i2s_start(&audio_cfg, &audio_callbacks);

    while (1)
    {
        // uint8_t command = COMMAND_PLAY;
        // ring_buf_put(&oscillator_state.commands_rx, &command, 1);
        // printk("sent COMMAND_PLAY\n");
        uint8_t command = 0;
        while (ring_buf_get(&demo_app.msg_rx, &command, 1) > 0)
        {
            switch (command)
            {
            case Led0On:
                set_led_state(0, 1);
                break;
            case Led0Off:
                set_led_state(0, 0);
                break;
            case Led1On:
                set_led_state(1, 1);
                break;
            case Led1Off:
                set_led_state(1, 0);
                break;
            case Led2On:
                set_led_state(2, 1);
                break;
            case Led2Off:
                set_led_state(2, 0);
                break;
            case Led3On:
                set_led_state(3, 1);
                break;
            case Led3Off:
                set_led_state(3, 0);
                break;
            default:
                break;
            }
        }
        k_msleep(POLL_INTERVAL_MS);
    }
}
