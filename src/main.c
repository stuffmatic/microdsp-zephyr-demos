#include <zephyr/zephyr.h>
#include <zephyr/drivers/gpio.h>
#include <sys/ring_buffer.h>
#include <microdsp_demos/microdsp_demos.h>
#include <stdlib.h>

#include "audio_callbacks.h"
#include "buttons.h"
#include "i2s.h"
#include "leds.h"
#include "codecs/wm8904.h"

#ifdef CONFIG_SOC_SERIES_NRF53X
static i2s_pin_cfg_t i2s_pin_cfg = {
    .sck_pin = 25,
    .lrck_pin = 7,
    .sdin_pin = 6,
    .sdout_pin = 5,
    .mck_pin = 26
};
#endif

#ifdef CONFIG_SOC_SERIES_NRF52X
static i2s_pin_cfg_t i2s_pin_cfg = {
    .sck_pin = 30,
    .lrck_pin = 29,
    .sdin_pin = 4,
    .sdout_pin = 28,
    .mck_pin = 31,
};
#endif

#define APP_MSG_BUFFER_SIZE 64
typedef struct
{
    void *rust_app_ptr;
    uint8_t msg_rx_buffer[APP_MSG_BUFFER_SIZE];
    uint8_t msg_tx_buffer[APP_MSG_BUFFER_SIZE];
    struct ring_buf msg_rx; /* to rust app */
    struct ring_buf msg_tx; /* from rust app */
} demo_app_t;

static demo_app_t demo_app;

static void processing_cb(void *cb_data, uint32_t sample_count, float *tx, const float *rx)
{
    demo_app_t *demo_app = (demo_app_t *)cb_data;

    /* Pass incoming messages to the demo app */
    uint8_t command = 0;
    while (ring_buf_get(&demo_app->msg_rx, &command, 1) > 0)
    {
        demo_app_handle_message(demo_app->rust_app_ptr, command);
    }

    /* Process audio */
    demo_app_process(demo_app->rust_app_ptr, tx, rx, sample_count);

    /* Debug listen to mic signal */
    if (false)
    {
        for (int i = 0; i < sample_count; i++)
        {
            tx[i] = 0.5 * rx[i];
        }
    }

    /* Pass outgoing messages to the main loop */
    while (true) {
        app_message_t message = demo_app_next_outgoing_message(demo_app->rust_app_ptr);
        if (message == 0) {
            break;
        } else {
            ring_buf_put(&demo_app->msg_tx, &message, 1);
        }
    }
}

static void dropout_cb(void *data)
{
    printk("dropout!\n");
}

void button_callback(int btn_idx) {
    uint8_t msg = 0;

    switch (btn_idx) {
        case 0:
            msg = Button0Down;
            break;
        case 1:
            msg = Button1Down;
            break;
        case 2:
            msg = Button2Down;
            break;
        case 3:
            msg = Button3Down;
            break;
    }

    if (msg) {
        ring_buf_put(&demo_app.msg_rx, &msg, 1);
    }
}

void main(void)
{
    /* Determined by NRF_I2S_MCK_32MDIV... and NRF_I2S_RATIO_... */
    float sample_rate = 44444.444;

    /* LEDs and buttons  */
    init_leds();
    init_buttons(&button_callback);

    /* Create demo app */
    demo_app.rust_app_ptr = demo_app_create(sample_rate);

    /* Create ring buffers for sending messages to and from the demo app */
    ring_buf_init(&demo_app.msg_rx, ARRAY_SIZE(demo_app.msg_rx_buffer), demo_app.msg_rx_buffer);
    ring_buf_init(&demo_app.msg_tx, ARRAY_SIZE(demo_app.msg_tx_buffer), demo_app.msg_tx_buffer);

    /* Init audio codec  */
    wm8904_init();

    /* Start I2S  */
    audio_callbacks_t audio_callbacks = {
        .dropout_cb = dropout_cb,
        .processing_cb = processing_cb,
        .cb_data = &demo_app,
    };
    i2s_start(&i2s_pin_cfg, &audio_callbacks);

    /* Main loop. Poll and react to messages from the rust app. */
    int32_t poll_interval_ms = 10;
    while (1)
    {
        uint8_t command = 0;
        while (ring_buf_get(&demo_app.msg_tx, &command, 1) > 0)
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
        k_msleep(poll_interval_ms);
    }
}
