#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"
#include "inc/hw_gpio.h"

#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

#include <stdbool.h>
#include <stdio.h>

#include "spi.h"
#include "console.h"
#include "misc.h"
#include "assert.h"

#define INPUT_GPIO_PERIPH SYSCTL_PERIPH_GPIOF
#define INPUT_GPIO_PORT   GPIO_PORTF_BASE
#define INPUT_GPIO_LEFT   GPIO_PIN_4
#define INPUT_GPIO_RIGHT  GPIO_PIN_0

uint32_t spi_count = 0;

char write_buffer[] = "0123456789";
char read_buffer[] = "9876543210";

static void inputs_init(void);
static void inputs_gpioISR(void);
static void spi_transfer_cb(void);

int main(void)
{
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    misc_init();

    console_init();
    console_printf("threesixty-canvas up and running!\n");

    inputs_init();

    spi_init(false, false, 100000);

    IntMasterEnable();

    spi_Transfer((uint8_t *) write_buffer, (uint8_t *) read_buffer, 10, spi_transfer_cb);

    uint32_t start = misc_getSysMS();

    while (true) {
        uint32_t now = misc_getSysMS();
        if ((now - start) >= 10000) {
            start = now;

            console_printf("%u transfers per second\n", spi_count / 10);
            spi_count = 0;
        }
    }
}

static void spi_transfer_cb(void)
{
    spi_count += 1;
    spi_Transfer((uint8_t *) write_buffer, (uint8_t *) read_buffer, 10, spi_transfer_cb);
}

static void inputs_gpioISR(void)
{
    long gpio_pin = GPIOPinIntStatus(INPUT_GPIO_PORT, true);
    GPIOPinIntClear(INPUT_GPIO_PORT, gpio_pin);

    switch (gpio_pin) {
        case INPUT_GPIO_LEFT:
            console_printf("Left button press!\n");
            break;

        case INPUT_GPIO_RIGHT:
            console_printf("Right button press!\n");
            break;

        default:
            console_printf("Unknown gpio pressed!\n");
            break;
    }
}

static void inputs_init(void)
{
    SysCtlPeripheralEnable(INPUT_GPIO_PERIPH);

    // Unlock PF0 so we can change it to a GPIO input
    // Once we have enabled (unlocked) the commit register then re-lock it
    // to prevent further changes.  PF0 is muxed with NMI thus a special case.
    HWREG(INPUT_GPIO_PORT + GPIO_O_LOCK) = GPIO_LOCK_KEY_DD;
    HWREG(INPUT_GPIO_PORT + GPIO_O_CR) |= 0x01;
    HWREG(INPUT_GPIO_PORT + GPIO_O_LOCK) = 0;

    GPIOPinTypeGPIOInput(INPUT_GPIO_PORT, INPUT_GPIO_LEFT | INPUT_GPIO_RIGHT);
    GPIOPadConfigSet(INPUT_GPIO_PORT, INPUT_GPIO_LEFT | INPUT_GPIO_RIGHT, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // Both switches need a pullup

    GPIOIntTypeSet(INPUT_GPIO_PORT, INPUT_GPIO_LEFT | INPUT_GPIO_RIGHT, GPIO_FALLING_EDGE);
    GPIOPortIntRegister(INPUT_GPIO_PORT, inputs_gpioISR);
    GPIOPinIntEnable(INPUT_GPIO_PORT, INPUT_GPIO_LEFT | INPUT_GPIO_RIGHT);
}
