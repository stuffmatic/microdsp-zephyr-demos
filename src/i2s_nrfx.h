#ifndef NRF_I2S_H
#define NRF_I2S_H

#include <nrfx_i2s.h>
#include "audio_processing.h"

nrfx_err_t i2s_start(audio_processing_options_t* processing_options);

#endif