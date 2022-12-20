#ifndef MICRODSP_DEMO_H
#define MICRODSP_DEMO_H

typedef enum {
    Button0Down = 1,
    Button1Down = 2,
    Button2Down = 3,
    Button3Down = 4,
    Button0Up = 5,
    Button1Up = 6,
    Button2Up = 7,
    Button3Up = 8,

    Led0On = 9,
    Led1On = 10,
    Led2On = 11,
    Led3On = 12,
    Led0Off = 13,
    Led1Off = 14,
    Led2Off = 15,
    Led3Off = 16,
} app_message_t;

void* demo_app_create(float sample_rate);

void demo_app_process(
    void* demo_app_ptr,
    float* tx_ptr,
    const float* rx_ptr,
    unsigned int sample_count
);

void demo_app_handle_message(void* demo_app_ptr, app_message_t message);
app_message_t demo_app_pump_message(void* demo_app_ptr);

#endif