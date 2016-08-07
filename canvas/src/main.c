#include "inc/hw_types.h"

#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"

#include <stdbool.h>
#include <stdio.h>

#include "io.h"
#include "console.h"
#include "misc.h"
#include "assert.h"
#include "nrf24.h"

static void init(void);
static void io_butt_cb(io_butt_t pressed);

static void timer_init(void);
static void timer_ISR(void);

static void nrf_cb(bool success, uint8_t *ack_payload, uint8_t length);

int main(void)
{
    init();

    uint8_t tx_data[8];

    nrf24_transmit(tx_data, 8, nrf_cb);

    while (true) {
    }
}

static void init(void)
{
    // Setup the system clock to run at 50 Mhz from PLL with crystal reference
    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

    misc_init();

    console_init();
    console_printf("threesixty-canvas up and running!\n");

    io_init();
    io_registerButtonCallback(io_butt_cb);
    io_led(IO_LED_BLUE, false);

//    timer_init();

    IntMasterEnable();

    nrf24_init(NRF24_TX);
}

static void io_butt_cb(io_butt_t pressed)
{
    if (pressed == IO_BUTT_LEFT) {
        console_printf("Left button pressed\n");
    } else if (pressed == IO_BUTT_RIGHT) {
        console_printf("Right button pressed\n");
    }
}

static void nrf_cb(bool success, uint8_t *ack_payload, uint8_t length)
{
    console_printf("nrf cb! %u %u %u\n", success, ack_payload, length);
}

#include "driverlib/timer.h"

#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "inc/hw_ints.h"

static void timer_init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    TimerLoadSet(TIMER0_BASE, TIMER_A, SysCtlClockGet() / 10);

    TimerIntRegister(TIMER0_BASE, TIMER_A, timer_ISR);
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    IntEnable(INT_TIMER0A);

    TimerEnable(TIMER0_BASE, TIMER_A);
}

static void timer_ISR(void)
{
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    console_printf("timer isr\n");
}
