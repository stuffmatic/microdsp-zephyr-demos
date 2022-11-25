#ifndef AUDIO_H
#define AUDIO_H

typedef void (*audio_processing_callback_t)(
  void* cb_data,
  uint32_t frame_count,
  uint32_t channel_count,
  float* tx,
  const float* rx
);
typedef void (*dropout_callback_t)();

typedef struct {
  audio_processing_callback_t processing_cb;
  void* processing_cb_data;
  dropout_callback_t dropout_cb;
} audio_processing_options_t;

#endif