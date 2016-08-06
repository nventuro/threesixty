#include "spi.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ssi.h"
#include "inc/hw_ints.h"

#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/ssi.h"
#include "driverlib/gpio.h"

#include <stdio.h>

#include "assert.h"
#include "misc.h"

// Pin FSS goes down at the beginning of every 8 bit SPI transfer, and up when it ends.
// Pin SPI_LONG_SS goes down at the same time as pin FSS, but only goes up when spi_transfer ends.
// Depending on the device, connect the SS (CSN) line to either SS0 or SPI_LONG_SS.
#define SPI_LONG_SS        GPIO_PIN_1
#define SPI_LONG_SS_PORT   GPIO_PORTE_BASE
#define SPI_LONG_SS_PERIPH SYSCTL_PERIPH_GPIOE

#define SPI_SS_START() GPIOPinWrite(SPI_LONG_SS_PORT, SPI_LONG_SS, 0)
#define SPI_SS_STOP()  GPIOPinWrite(SPI_LONG_SS_PORT, SPI_LONG_SS, SPI_LONG_SS)

static struct {
    volatile bool busy;

    const uint8_t *write;
    uint8_t *read;
    uint8_t length;
    uint8_t index;

    spi_cb eot;
} spi_data;

static void spi_sendNewData(void);
static void spi_storeReceived(void);
static void spi_ISR(void);

void spi_init(bool cpol, bool cpha, uint32_t freq)
{
    // The SSI0 peripheral must be enabled for use.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

    // For this example SSI0 is used with PortA[5:2]. GPIO port A needs to be enabled so these pins can be used.
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    // Configure the pin muxing for SSI0 functions on port A2, A3, A4, and A5.
    GPIOPinConfigure(GPIO_PA2_SSI0CLK);
    GPIOPinConfigure(GPIO_PA3_SSI0FSS);
    GPIOPinConfigure(GPIO_PA4_SSI0RX);
    GPIOPinConfigure(GPIO_PA5_SSI0TX);

    // Configure the GPIO settings for the SSI pins.  This function also gives
    // control of these pins to the SSI hardware.
    // The pins are assigned as follows:
    //      PA5 - SSI0Tx
    //      PA4 - SSI0Rx
    //      PA3 - SSI0Fss
    //      PA2 - SSI0CLK
    GPIOPinTypeSSI(GPIO_PORTA_BASE, GPIO_PIN_5 | GPIO_PIN_4 | GPIO_PIN_3 | GPIO_PIN_2);

    // Configure and enable the SSI port for SPI master mode.  Use SSI0,
    // system clock supply, idle clock level low and active low clock in
    // freescale SPI mode, master mode, 1MHz SSI frequency, and 8-bit data.

    // Polarity Phase       Mode
    //   0       0   SSI_FRF_MOTO_MODE_0
    //   0       1   SSI_FRF_MOTO_MODE_1
    //   1       0   SSI_FRF_MOTO_MODE_2
    //   1       1   SSI_FRF_MOTO_MODE_3
    uint8_t mode;
    if (!cpol) {
        if (!cpha) {
            mode = SSI_FRF_MOTO_MODE_0;
        } else {
            mode = SSI_FRF_MOTO_MODE_1;
        }
    } else {
        if (!cpha) {
            mode = SSI_FRF_MOTO_MODE_2;
        } else {
            mode = SSI_FRF_MOTO_MODE_3;
        }
    }
    SSIConfigSetExpClk(SSI0_BASE, SysCtlClockGet(), mode, SSI_MODE_MASTER, freq, 8);

    HWREG(SSI0_BASE + SSI_O_CR1) |= SSI_CR1_EOT;

    SSIEnable(SSI0_BASE);

    SSIIntRegister(SSI0_BASE, spi_ISR);

    SSIIntDisable(SSI0_BASE, SSI_TXFF);
    IntEnable(INT_SSI0);

    // Enable the LONG_SS pinS
    SysCtlPeripheralEnable(SPI_LONG_SS_PERIPH);
    GPIOPinTypeGPIOOutput(SPI_LONG_SS_PORT, SPI_LONG_SS);

    spi_data.busy = false;

    SPI_SS_STOP();
}

bool spi_isBusy(void)
{
    return spi_data.busy;
}

void spi_Transfer(const uint8_t *write, uint8_t *read, uint8_t length, spi_cb eot)
{
    assert(!spi_data.busy, "spi: attempt to initiate a transfer while another is in progress.\n");
    assert(write != NULL, "spi: received NULL write pointer.\n");
    assert(length > 0, "spi: length must be non-zero.\n");

    spi_data.busy = true;

    spi_data.write = write;
    spi_data.read = read;
    spi_data.length = length;
    spi_data.eot = eot;
    spi_data.index = 0;

    SPI_SS_START();
    spi_sendNewData();

    SSIIntEnable(SSI0_BASE, SSI_TXFF);
}

static void spi_sendNewData(void)
{
    while (SSIBusy(SSI0_BASE)) {
    }

    SSIDataPut(SSI0_BASE, spi_data.write[spi_data.index]);
}

static void spi_storeReceived(void)
{
    while (SSIBusy(SSI0_BASE)) {
    }

    unsigned long read;

    // Receive the data using the "blocking" Get function. This function
    // will wait until there is data in the receive FIFO before returning.
    SSIDataGet(SSI0_BASE, &read);

    if (spi_data.read != NULL) {
        spi_data.read[spi_data.index] = read;
    }
}

static void spi_ISR(void)
{
    spi_storeReceived();

    spi_data.index++;
    if (spi_data.index != spi_data.length) {
        spi_sendNewData();
    } else {
        SPI_SS_STOP();
        SSIIntDisable(SSI0_BASE, SSI_TXFF);

        spi_data.busy = false;

        if (spi_data.eot != NULL) {
            spi_data.eot();
        }
    }
}
