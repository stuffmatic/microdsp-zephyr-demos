#ifndef NRF_I2S_H
#define NRF_I2S_H

#include <nrfx_i2s.h>

/* typedef struct {
  processing_cb (float*, const float*, void* context)
  processing_cb_context
} audio_options; */

nrfx_err_t i2s_start();

#endif