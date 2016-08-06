#ifndef _IO_H_
#define _IO_H_

#include <stdbool.h>

typedef enum {
    IO_BUTT_LEFT,
    IO_BUTT_RIGHT
} io_butt_t;

typedef enum {
    IO_LED_RED,
    IO_LED_GREEN,
    IO_LED_BLUE
} io_led_t;

typedef void (*io_butt_cb_t)(io_butt_t pressed);

void io_init(void);

void io_registerButtonCallback(io_butt_cb_t cb);

void io_led(io_led_t led, bool status);

#endif
