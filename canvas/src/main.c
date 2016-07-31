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

// Define pin to LED color mapping.
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2

static void init_console(void);

int main(void)
{
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    init_console();
    UARTprintf("Hello, world!\n");

    // Enable and configure the GPIO port for the LED operation.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED | BLUE_LED);

    spi_init(false, false, 100000);
    IntMasterEnable();

    char write_buffer[] = "Hello!";
    char read_buffer[] = "Goodbye!";

    spi_Transfer(write_buffer, read_buffer, 7, NULL);

    while (spi_isBusy()) {        
    }

    UARTprintf("Received: %s\n", read_buffer);

    while (true) {
        // Turn on the LED
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED | BLUE_LED, RED_LED);

        // Delay for a bit
        SysCtlDelay(1000000);

        // Turn on the LED
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED | BLUE_LED, BLUE_LED);

        // Delay for a bit
        SysCtlDelay(1000000);
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