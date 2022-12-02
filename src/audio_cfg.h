#ifndef AUDIO_CFG_H
#define AUDIO_CFG_H

#include <nrfx_i2s.h>
#include "codecs/wm8758.h"
#include "codecs/wm8904.h"

typedef struct {
    void (*init_codec)();
    float sample_rate;

    // nrfx i2s pins
    uint8_t sck_pin;
    uint8_t lrck_pin;
    uint8_t mck_pin;
    uint8_t sdout_pin;
    uint8_t sdin_pin;

    // nrfx i2s clock setup
    nrf_i2s_mck_t mck_setup;
    nrf_i2s_ratio_t ratio;
} audio_cfg_t;

static void init_codec() {
    #ifdef CONFIG_AUDIO_CODEC_WM8758
    #ifdef CONFIG_SOC_SERIES_NRF52X
    wm8758 codec is not supported on nrf52x
    #endif
    wm8758_init();
    #endif
    #ifdef CONFIG_AUDIO_CODEC_WM8904
    wm8904_init();
    #endif
}

#ifdef CONFIG_SOC_SERIES_NRF53X
static audio_cfg_t audio_cfg = {
    .init_codec = init_codec,
    .sample_rate = 0,

    .sck_pin = 0,
    .lrck_pin = 0,
    .sdin_pin = 0,
    .sdout_pin = 0,
    .mck_pin = 0,

    .mck_setup = NRF_I2S_MCK_32MDIV8,
    .ratio = NRF_I2S_RATIO_96X
};
#endif

#ifdef CONFIG_SOC_SERIES_NRF52X

static audio_cfg_t audio_cfg = {
    .init_codec = init_codec,
    .sample_rate = 44444.444,

    .sck_pin = 30,
    .lrck_pin = 29,
    .sdin_pin = 28,
    .sdout_pin = 4,
    .mck_pin = 31,

    .mck_setup = NRF_I2S_MCK_32MDIV15,
    .ratio = NRF_I2S_RATIO_48X
};
#endif

#endif // AUDIO_CFG_H