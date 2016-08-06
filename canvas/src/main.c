#include "inc/hw_types.h"

#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include <stdbool.h>
#include <stdio.h>

#include "io.h"
#include "spi.h"
#include "console.h"
#include "misc.h"
#include "assert.h"

uint32_t spi_count = 0;

char write_buffer[] = "0123456789";
char read_buffer[] = "9876543210";

static void io_butt_cb(io_butt_t pressed);
static void spi_transfer_cb(void);

int main(void)
{
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    misc_init();

    console_init();
    console_printf("threesixty-canvas up and running!\n");

    io_init();
    io_registerButtonCallback(io_butt_cb);
    io_led(IO_LED_BLUE, true);

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

static void io_butt_cb(io_butt_t pressed)
{
    if (pressed == IO_BUTT_LEFT) {
        console_printf("Left button pressed\n");
    } else if (pressed == IO_BUTT_RIGHT) {
        console_printf("Right button pressed\n");
    }
}

static void spi_transfer_cb(void)
{
    spi_count += 1;
    spi_Transfer((uint8_t *) write_buffer, (uint8_t *) read_buffer, 10, spi_transfer_cb);
}
