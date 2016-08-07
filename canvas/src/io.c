#include "io.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_gpio.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

#include "console.h"
#include "assert.h"

#include <stdlib.h>

#define IO_GPIO_PERIPH SYSCTL_PERIPH_GPIOF
#define IO_GPIO_PORT   GPIO_PORTF_BASE

#define IO_GPIO_BUTT_LEFT   GPIO_PIN_4
#define IO_GPIO_BUTT_RIGHT  GPIO_PIN_0

#define IO_GPIO_LED_RED     GPIO_PIN_1
#define IO_GPIO_LED_GREEN   GPIO_PIN_3
#define IO_GPIO_LED_BLUE    GPIO_PIN_2

static io_butt_cb_t io_butt_cb = NULL;

static void io_gpioISR(void);

void io_init(void)
{
    SysCtlPeripheralEnable(IO_GPIO_PERIPH);

    // Outputs
    GPIOPinTypeGPIOOutput(IO_GPIO_PORT, IO_GPIO_LED_RED | IO_GPIO_LED_GREEN | IO_GPIO_LED_BLUE);
    io_led(IO_LED_RED, false);
    io_led(IO_LED_GREEN, false);
    io_led(IO_LED_BLUE, false);

    // Inputs

    // Unlock PF0 so we can change it to a GPIO input
    // Once we have enabled (unlocked) the commit register then re-lock it
    // to prevent further changes. PF0 is muxed with NMI thus a special case.
    HWREG(IO_GPIO_PORT + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
    HWREG(IO_GPIO_PORT + GPIO_O_CR) |= 0x01;
    HWREG(IO_GPIO_PORT + GPIO_O_LOCK) = 0;

    GPIOPinTypeGPIOInput(IO_GPIO_PORT, IO_GPIO_BUTT_LEFT | IO_GPIO_BUTT_RIGHT);
    GPIOPadConfigSet(IO_GPIO_PORT, IO_GPIO_BUTT_LEFT | IO_GPIO_BUTT_RIGHT, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // Both switches need a pullup

    GPIOIntTypeSet(IO_GPIO_PORT, IO_GPIO_BUTT_LEFT | IO_GPIO_BUTT_RIGHT, GPIO_FALLING_EDGE);
    GPIOPortIntRegister(IO_GPIO_PORT, io_gpioISR);
    GPIOPinIntEnable(IO_GPIO_PORT, IO_GPIO_BUTT_LEFT | IO_GPIO_BUTT_RIGHT);
}

void io_registerButtonCallback(io_butt_cb_t cb)
{
    io_butt_cb = cb;
}

void io_led(io_led_t led, bool status)
{
    assert((led == IO_LED_RED) || (led == IO_LED_GREEN) || (led == IO_LED_BLUE), "unknown led selected\n");

    long gpio_pin;
    if (led == IO_LED_RED) {
        gpio_pin = IO_LED_RED;
    } else if (led == IO_LED_GREEN) {
        gpio_pin = IO_GPIO_LED_GREEN;
    } else {
        gpio_pin = IO_GPIO_LED_BLUE;
    }

    GPIOPinWrite(IO_GPIO_PORT, gpio_pin, status ? 0xFF : 0x00);
}

static void io_gpioISR(void)
{
    long gpio_pin = GPIOPinIntStatus(IO_GPIO_PORT, true);
    GPIOPinIntClear(IO_GPIO_PORT, gpio_pin);

    io_butt_t butt;
    switch (gpio_pin) {
        case IO_GPIO_BUTT_LEFT:
            butt = IO_BUTT_LEFT;
            break;

        case IO_GPIO_BUTT_RIGHT:
            butt = IO_BUTT_RIGHT;
            break;

        default:
            assert(false, "unknown gpio pressed!\n");
            return; // We won't ever reach this point because the assert will fail, but this avoids compiler warnings
    }

    if (io_butt_cb != NULL) {
        io_butt_cb(butt);
    }
}
