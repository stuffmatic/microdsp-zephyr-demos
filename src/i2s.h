#ifndef I2S_H
#define I2S_H

#include <nrfx_i2s.h>
#include "audio_callbacks.h"
#include "audio_cfg.h"

nrfx_err_t i2s_start(audio_cfg_t* audio_cfg, audio_callbacks_t* audio_callbacks);

#endif // I2S_H