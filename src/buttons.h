#ifndef BUTTONS_H
#define BUTTONS_H

typedef void (*button_callback_t)(int btn_idx);
void init_buttons(button_callback_t cb);

#endif