#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_nvic.h"

#include "driverlib/sysctl.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

#include "utils/uartstdio.h"

#include <stdbool.h>
#include <stdio.h>

#include "spi.h"
#include "misc.h"

static void init_console(void);
static void spi_transfer_cb(void);

uint32_t spi_count = 0;

char write_buffer[] = "0123456789";
char read_buffer[] = "9876543210";

int main(void)
{
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    misc_init();

    init_console();
    UARTprintf("threesixty-canvas up and running!\n");

    spi_init(false, false, 100000);
    IntMasterEnable();

    spi_Transfer((uint8_t *) write_buffer, (uint8_t *) read_buffer, 10, spi_transfer_cb);

    uint32_t start = misc_getSysMS();

    while (true) {
        uint32_t now = misc_getSysMS();
        if ((now - start) >= 10000) {
            start = now;
            UARTprintf("%u transfers per second\n", spi_count / 10);
            spi_count = 0;
        }
    }
}

static void init_console(void)
{
    // Initialize the UART.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTStdioInit(0);
}

static void spi_transfer_cb(void)
{
    spi_count += 1;
    spi_Transfer((uint8_t *) write_buffer, (uint8_t *) read_buffer, 10, spi_transfer_cb);
}
