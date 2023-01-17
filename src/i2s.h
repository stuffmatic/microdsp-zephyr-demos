#ifndef I2S_H
#define I2S_H

#include <nrfx_i2s.h>
#include "audio_callbacks.h"

typedef struct {
    uint8_t sck_pin;
    uint8_t lrck_pin;
    uint8_t mck_pin;
    uint8_t sdout_pin;
    uint8_t sdin_pin;
} i2s_pin_cfg_t;

nrfx_err_t i2s_start(i2s_pin_cfg_t* pin_cfg, audio_callbacks_t* audio_callbacks);

#endif