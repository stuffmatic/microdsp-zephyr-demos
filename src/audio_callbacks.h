#ifndef AUDIO_CALLBACKS_H
#define AUDIO_CALLBACKS_H

typedef void (*audio_processing_callback_t)(
  void* cb_data,
  unsigned int frame_count,
  unsigned int channel_count,
  float* tx,
  const float* rx
);
typedef void (*audio_dropout_callback_t)(void* cb_data);

typedef struct {
  audio_processing_callback_t processing_cb;
  audio_dropout_callback_t dropout_cb;
  void* cb_data;
} audio_callbacks_t;

#endif // AUDIO_CALLBACKS_H