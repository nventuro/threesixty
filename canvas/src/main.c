#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"

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

static void spi_transfer_cb(void);

static void gpio_isr(void);

uint32_t spi_count = 0;

char write_buffer[] = "0123456789";
char read_buffer[] = "9876543210";

int main(void)
{
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    misc_init();

    console_init();
    console_printf("threesixty-canvas up and running!\n");

    spi_init(false, false, 100000);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);

    GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);
    GPIOPortIntRegister(GPIO_PORTF_BASE, gpio_isr);
    GPIOPinIntEnable(GPIO_PORTF_BASE, GPIO_PIN_4);

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

static void gpio_isr(void)
{
    GPIOPinIntClear(GPIO_PORTF_BASE, GPIO_PIN_4);

    if (!GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4)) {
        console_printf("Button press!\n");
    }
}
