#include "nrf24.h"

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_nvic.h"
#include "inc/hw_gpio.h"

#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"

#include "spi.h"
#include "misc.h"
#include "assert.h"

#include <stdlib.h>
#include <string.h>

#define NRF24_IRQ_GPIO_PERIPH SYSCTL_PERIPH_GPIOF
#define NRF24_IRQ_GPIO_PORT   GPIO_PORTF_BASE
#define NRF24_IRQ_GPIO_PIN    GPIO_PIN_4

#define NRF24_CE_GPIO_PERIPH SYSCTL_PERIPH_GPIOF
#define NRF24_CE_GPIO_PORT   GPIO_PORTF_BASE
#define NRF24_CE_GPIO_PIN    GPIO_PIN_4

// SPI COMMANDS
#define R_REGISTER(x) (x)
#define W_REGISTER(x) (BIT(5) | x)

#define R_RX_PAYLOAD 0x61
#define W_TX_PAYLOAD 0xA0
#define FLUSH_TX 0xE1
#define FLUSH_RX 0xE2

#define REUSE_TX_PL 0xE3
#define R_RX_PL_WID 0x60
#define W_ACK_PAYLOAD(x) (0xA8 | x)
#define W_TX_PAYLOAD_NO_ACK 0xB0

#define ACTIVATE 0x50
#define ACTIVATE_DATA 0x73

#define NOP 0xFF

// REGISTER ADDRESSES & BITS
#define CONFIG 0x00
#define MASK_RX_DR BIT(6)
#define MASK_TX_DS BIT(5)
#define MASK_MAX_RT BIT(4)
#define EN_CRC BIT(3)
#define CRC_1_BYTE (0 << 2)
#define CRC_2_BYTE (1 << 2)
#define PWR_UP BIT(1)
#define PRIM_RX BIT(0)
#define PRIM_TX (0 << 0)

#define EN_AA 0x01 // Enable Auto Acknowledgement
#define ENAA_P(x) BIT(x)

#define EN_RXADDR 0x02 // Enable RX Addresses
#define ERX_P(x) BIT(x)

#define SETUP_AW 0x03 // Setup of Address Width
#define AW_3_BYTES 0x01
#define AW_4_BYTES 0x02
#define AW_5_BYTES 0x03

#define SETUP_RETR 0x04// Setup of Automatic Retransmission
#define ARD(x) (x<<4) // The delay is equal to (x+1) * 250us
#define ARC(x) x // x equals the maximum amount of retransmissions

#define RF_CH 0x05// RF Channel
#define SET_CH(x) x // The channel frequency is 2400 + x MHz (1 MHz resolution)

#define RF_SETUP 0x06// RF Setup
#define CONT_WAVE BIT(7)
#define RF_DR_1MBPS ((0 << 5) | (0 << 3))
#define RF_DR_2MBPS  ((0 << 5) | (1 << 3))
#define RF_DR_250KBPS  ((1 << 5) | (0 << 3))
#define PLL_LOCK BIT(4)
#define RF_PWR_m18DBM ((0x00) << 1)
#define RF_PWR_m12DBM ((0x01) << 1)
#define RF_PWR_m6DBM ((0x02) << 1)
#define RF_PWR_0DBM ((0x03) << 1)

#define STATUS 0x07
#define RX_DR BIT(6) // Data Ready RX FIFO interrupt
#define TX_DS BIT(5) // Data Sent TX FIFO interrupt
#define MAX_RT BIT(4) // Maximum number of retransmits reached interrupt
#define RX_P_NO(x) (x << 1) // Data pipe number for the payload available for reading in the RX FIFO
#define ST_TX_FULL BIT(0) // TX FIFO full

#define OBSERVE_TX 0x08
#define PLOS_CNT 0xF0 // Lost packets
#define ARC_CNT 0x0F // Retransmitted packets. Reset for each new transmission

#define RPD 0x09
#define RPD_BIT BIT(0) // Received Power Detector (Carrier Detect)

#define RX_ADDR_P(x) (0x0A + x) // Pipe 0 receive address
// Pipe 0 has a length defined by SETUP_AW. All other pipes share the most significant bytes,
// and differ in the least significant byte.
// This value must be set equal to the PTX's TX_ADDR if automatic acknowledgement is being used.

#define TX_ADDR 0x10 // Length defined by SETUP_AW

#define RX_PW_P(x) (0x11 + x) // RX payload width for pipe x

#define FIFO_STATUS 0x17
#define TX_REUSE BIT(6)
#define TX_FULL BIT(5) // TX FIFO Full
#define TX_EMPTY BIT(4) // TX FIFO Empty
#define RX_FULL BIT(1) // RX FIFO Full
#define RX_EMPTY BIT(0) // RX FIFO Empty

#define DYNPD 0x1C
#define DPL_P(x) BIT(x)  // Dynamic Payload Length for pipe x

#define FEATURE 0x1D
#define EN_DPL BIT(2) // Enable Dynamic Payload Length
#define EN_ACK_PAY BIT(1) // Enable Payload with ACK
#define EN_DYN_ACK BIT(0) // Enable the W_TX_PAYLOAD_NO_ACK command

#define NRF_SPI_CPOL 0
#define NRF_SPI_CPHA 0

#define NRF_SPI_DATA_SIZE 40

#define NRF_STARTUP_TIME_MS 150
#define NRF_CE_PULSE_DURATION_MS 2 // In reality, it is about 10us, but this is only done once so no harm done

#define NRF_ADDRESS 0xE7

static struct {
    bool init;
    uint8_t init_step;

    nrf24_mode_t mode;

    uint8_t spi_input_data[NRF_SPI_DATA_SIZE];
    uint8_t spi_output_data[NRF_SPI_DATA_SIZE];

    struct {
        bool transmitting;
        uint8_t callback_stage;
        nrf24_tx_cb_t eot;

        // Used when reading the payload in an ACK
        uint8_t read_len[2];
    } tx_data;

    struct {
        uint8_t callback_stage;
        nrf24_rx_cb_t eot;
        bool receive_request;
        bool send_request;

        // Used for temporary payload storage
        const uint8_t *data;
        uint8_t length;

        // Used when reading the payload
        uint8_t read_len[2];
    } rx_data;
} nrf_data;

static void nrf24_initSequence(void);
static void nrf24_commenceTransmission(const uint8_t *data, uint8_t length);
static void nrf24_StoreAckDone(void);
static void nrf24_spiReadStatusCallback (void);
static void nrf24_spiHandleTXCallback(void);
static void nrf24_spiHandleTXACKCallback(void);
static void nrf24_spiHandleMAXRTCallback(void);
static void nrf24_spiHandleRXCallback(void);
static void nrf24_irqISR(void);

void nrf24_init(nrf24_mode_t mode)
{
    nrf_data.init = false;
    nrf_data.init_step = 0;

    nrf_data.mode = mode;

    if (nrf_data.mode == NRF24_TX) {
        nrf_data.tx_data.transmitting = false;
    } else {
        nrf_data.rx_data.eot = NULL;
        nrf_data.rx_data.receive_request = false;
        nrf_data.rx_data.send_request = false;
    }

    SysCtlPeripheralEnable(NRF24_IRQ_GPIO_PERIPH);
    GPIOPinTypeGPIOInput(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN);

    GPIOIntTypeSet(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN, GPIO_RISING_EDGE);
    GPIOPortIntRegister(NRF24_IRQ_GPIO_PORT, nrf24_irqISR);

    SysCtlPeripheralEnable(NRF24_CE_GPIO_PERIPH);
    GPIOPinTypeGPIOOutput(NRF24_CE_GPIO_PORT, NRF24_CE_GPIO_PIN);

    GPIOPinWrite(NRF24_CE_GPIO_PORT, NRF24_CE_GPIO_PIN, 0x00);

    misc_busyWaitMS(NRF_STARTUP_TIME_MS);
    nrf24_initSequence();

    while (nrf_data.init != true) {
    }

    // Wait until the nRF goes into Standby-I (or II) mode
    misc_busyWaitMS(NRF_CE_PULSE_DURATION_MS);

    GPIOPinIntEnable(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN);
}

void nrf24_transmit(const uint8_t *data, uint8_t length, nrf24_tx_cb_t eot)
{
    assert(nrf_data.mode == NRF24_TX, "nrf: transmit can only be called in NRF24_TX mode\n");
    assert(!nrf_data.tx_data.transmitting, "nrf: attempted to initiate a transmission while another one is taking place\n");
    assert(data != NULL, "nrf: recevied NULL data pointer.n");
    assert((length > 0) && (length <= 32), "nrf: length must be between 1 and 32.\n");

    nrf_data.tx_data.transmitting = true;
    nrf_data.tx_data.eot = eot;

    nrf24_commenceTransmission(data, length);
}

bool nrf24_IsBusy(void)
{
    assert(nrf_data.mode == NRF24_TX, "nrf: isBusy can only be called in NRF24_TX mode\n");

    return nrf_data.tx_data.transmitting;
}

void nrf24_receive(nrf24_rx_cb_t eot)
{
    assert(nrf_data.mode == NRF24_RX, "nrf: receive can only be called in NRF24_RX mode.\n");
    assert(nrf_data.rx_data.eot == NULL, "nrf: attempted to register a second end of transmission callback.\n");
    assert(eot != NULL, "nrf: received a NULL end of transmission callback.\n");

    nrf_data.rx_data.eot = eot;
}

void nrf24_storeAckPayload(const uint8_t *data, uint8_t length)
{
    assert(nrf_data.mode == NRF24_RX, "nrf: storeAckPayload can only be called in NRF24_RX mode.\n");

    //bool intStatus = SafeSei();

    nrf_data.rx_data.send_request = true;

    if (nrf_data.rx_data.receive_request == true) {
        nrf_data.rx_data.data = data;
        nrf_data.rx_data.length = length;
    } else {
        nrf_data.spi_input_data[0] = W_ACK_PAYLOAD(0);
        for (uint8_t index = 0; index < length; ++index) {
            nrf_data.spi_input_data[index + 1] = data[index];
        }

        spi_transfer(nrf_data.spi_input_data, NULL, length + 1, nrf24_StoreAckDone);
    }

    //SafeCli(intStatus);
}

static void nrf24_initSequence(void)
{
    switch (nrf_data.init_step) {
        // Device is in the Power Down state if there was a power cycle, or in some other state if the uC was reset.
        case 0:
            nrf_data.init_step += 1;

            spi_init(false, false, 100000);

            nrf_data.spi_input_data[0] = W_REGISTER(CONFIG);
            // Enable all interrupts (all masks are set to 0), enable 2 byte CRC, power down device (PWR_UP = 0), and set as NRF24_TX/NRF24_RX.
            nrf_data.spi_input_data[1] = EN_CRC | CRC_2_BYTE | ((nrf_data.mode == NRF24_RX) ? PRIM_RX : PRIM_TX);

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 1:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(EN_AA);
            // Enable Auto Acknowledgement for Pipe 0 (and disable for all other pipes).
            nrf_data.spi_input_data[1] = ENAA_P(0);

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 2:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(EN_RXADDR);
            // Enable RX Pipe 0 (and disable for all other pipes).
            nrf_data.spi_input_data[1] = ERX_P(0);

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 3:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(SETUP_AW);
            // Set Address Width to 5 bytes
            nrf_data.spi_input_data[1] = AW_5_BYTES;

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 4:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(SETUP_RETR);
            // Set ARD to (1+1) * 250us = 500us, and ARC to 10 retransmits.
            nrf_data.spi_input_data[1] = ARD(1) | ARC(10);

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 5:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(RF_CH);
            // Set RF Channel to 24050 MHz
            nrf_data.spi_input_data[1] = SET_CH(50);

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 6:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(RF_SETUP);
            // Set Data Rate to 2Mbps and output power to 0dBm.
            nrf_data.spi_input_data[1] = RF_DR_2MBPS | RF_PWR_0DBM;

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 7:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(RX_ADDR_P(0));
            // Set RX Address
            nrf_data.spi_input_data[1] = NRF_ADDRESS;
            nrf_data.spi_input_data[2] = NRF_ADDRESS;
            nrf_data.spi_input_data[3] = NRF_ADDRESS;
            nrf_data.spi_input_data[4] = NRF_ADDRESS;
            nrf_data.spi_input_data[5] = NRF_ADDRESS;

            spi_transfer(nrf_data.spi_input_data, NULL, 6, nrf24_initSequence);

            break;

        case 8:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(TX_ADDR);
            // Set TX Address
            nrf_data.spi_input_data[1] = RF_DR_2MBPS | RF_PWR_0DBM;
            nrf_data.spi_input_data[1] = NRF_ADDRESS;
            nrf_data.spi_input_data[2] = NRF_ADDRESS;
            nrf_data.spi_input_data[3] = NRF_ADDRESS;
            nrf_data.spi_input_data[4] = NRF_ADDRESS;
            nrf_data.spi_input_data[5] = NRF_ADDRESS;

            spi_transfer(nrf_data.spi_input_data, NULL, 6, nrf24_initSequence);

            break;

        case 9:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(DYNPD);
            // Enable dynamic payload length for Pipe 0.
            nrf_data.spi_input_data[1] = DPL_P(0);

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 10:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = ACTIVATE;
            // Enable the R_RX_PL_WID, W_ACK_PAYLOAD and W_TX_PAYLOAD_NOACK commands
            nrf_data.spi_input_data[1] = ACTIVATE_DATA;

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 11:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(FEATURE);
            // Enable dynamic payload length, enable payload with ACK, disable transmissions with no ACK.
            nrf_data.spi_input_data[1] = EN_DPL | EN_ACK_PAY;

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 12:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(STATUS);
            // Turn off all interrupts, if they were on (because the nRF wasn't reset)
            nrf_data.spi_input_data[1] = MAX_RT | TX_DS | RX_DR;

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 13:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = FLUSH_TX;
            // Flush the TX FIFO, if there was something stored (because the nRF wasn't reset)

            spi_transfer(nrf_data.spi_input_data, NULL, 1, nrf24_initSequence);

            break;

        case 14:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = FLUSH_RX;
            // Flush the RX FIFO, if there was something stored (because the nRF wasn't reset)

            spi_transfer(nrf_data.spi_input_data, NULL, 1, nrf24_initSequence);

            break;

        case 15:
            nrf_data.init_step += 1;

            nrf_data.spi_input_data[0] = W_REGISTER(CONFIG);
            // Register configuration is done, power on device.
            nrf_data.spi_input_data[1] = EN_CRC | CRC_2_BYTE | PWR_UP | ((nrf_data.mode == NRF24_RX) ? PRIM_RX : PRIM_TX);

            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_initSequence);

            break;

        case 16:
            nrf_data.init_step += 1;
            GPIOPinWrite(NRF24_CE_GPIO_PORT, NRF24_CE_GPIO_PIN, 0xFF);

            break;

        case 17:
            // The nRF is now configured and properly initialized
            nrf_data.init = true;

            break;
    }
}

static void nrf24_commenceTransmission(const uint8_t *data, uint8_t length)
{
    nrf_data.spi_input_data[0] = W_TX_PAYLOAD;
    memcpy(&(nrf_data.spi_input_data[1]), data, length);

    spi_transfer(nrf_data.spi_input_data, NULL, length + 1, NULL);
}

void nrf24_StoreAckDone(void)
{
    nrf_data.rx_data.send_request = false;
    if (nrf_data.rx_data.receive_request == true)
    {
        nrf_data.spi_input_data[0] = NOP;
        spi_transfer(nrf_data.spi_input_data, nrf_data.spi_input_data, 1, nrf24_spiReadStatusCallback);
    }
}

static void nrf24_irqISR(void)
{
    // The nRF might be issuing an IRQ if it wasn't reset
    if (nrf_data.init != true) {
        GPIOPinIntDisable(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN);
        return;
    }

    // Since the IRQ won't be serviced until a few SPI transmissions take place, it must be inhibited until then.
    GPIOPinIntDisable(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN);

    // If the NRF24_RX is currently uploading an ACK payload, it cannot service the IRQ immediately.
    if (nrf_data.mode == NRF24_RX) {
        nrf_data.rx_data.receive_request = true;

        if (nrf_data.rx_data.send_request == true) {
            return;
        }
    }

    // By writing a NOP, the nRF's status register is read, and it is stored in nrf_data.spi_input_data[0]
    nrf_data.spi_input_data[0] = NOP;
    spi_transfer(nrf_data.spi_input_data, nrf_data.spi_input_data, 1, nrf24_spiReadStatusCallback);
}

static void nrf24_spiReadStatusCallback(void)
{
    // Read the status byte, and turn off the corresponding interrupt bits

    if (nrf_data.mode == NRF24_TX) {
        nrf_data.tx_data.callback_stage = 0;

        if ((nrf_data.spi_input_data[0] & MAX_RT) != 0) { // MAX_RT interrupt
            nrf_data.spi_input_data[0] = W_REGISTER(STATUS);
            nrf_data.spi_input_data[1] = MAX_RT;
            spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_spiHandleMAXRTCallback);

        } else { // TX_DS interrupt, an RX_DR might have also ocurred if there was payload
            if ((nrf_data.spi_input_data[0] & RX_DR) != 0) { // TX_DS + RX_DR interrupts
                nrf_data.spi_input_data[0] = W_REGISTER(STATUS);
                nrf_data.spi_input_data[1] = TX_DS | RX_DR;
                spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_spiHandleTXACKCallback);

            } else { // TX_DS interrupt
                nrf_data.spi_input_data[0] = W_REGISTER(STATUS);
                nrf_data.spi_input_data[1] = TX_DS;
                spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_spiHandleTXCallback);
            }
        }
    } else { // nrf_data.mode == NRF24_RX
        nrf_data.rx_data.callback_stage = 0;

        // Either a RX_DR or both RX_DR + TX_DS interrupts have occured - clear both flags just in case
        nrf_data.spi_input_data[0] = W_REGISTER(STATUS);
        nrf_data.spi_input_data[1] = TX_DS | RX_DR;
        spi_transfer(nrf_data.spi_input_data, NULL, 2, nrf24_spiHandleRXCallback);
    }
}

static void nrf24_spiHandleTXCallback(void)
{
    // TX_DS interrupt - the IRQ has been serviced
    GPIOPinIntEnable(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN);

    nrf_data.tx_data.transmitting = false;
    if (nrf_data.tx_data.eot != NULL) {
        nrf_data.tx_data.eot(true, NULL, 0);
    }
}

void nrf24_spiHandleTXACKCallback (void)
{
    switch (nrf_data.tx_data.callback_stage) {
        case 0:
            // TX_DS + RX_DR interrupts - the IRQ has been serviced
            GPIOPinIntEnable(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN);

            nrf_data.tx_data.callback_stage++;

            // There's data in the RX FIFO: first, it's width must be read.
            nrf_data.spi_input_data[0] = R_RX_PL_WID;
            spi_transfer(nrf_data.spi_input_data, nrf_data.tx_data.read_len, 2, nrf24_spiHandleTXACKCallback);

            break;

        case 1:
            nrf_data.tx_data.callback_stage++;

            // nrf_data.tx_data.read_len[1] contains the amount of bytes in the RX FIFO
            nrf_data.spi_input_data[0] = R_RX_PAYLOAD;
            spi_transfer(nrf_data.spi_input_data, nrf_data.spi_output_data, nrf_data.tx_data.read_len[1] + 1, nrf24_spiHandleTXACKCallback);

            break;

        case 2:
            // The data has been read
            nrf_data.tx_data.transmitting = false;
            if (nrf_data.tx_data.eot != NULL) {
                // The first byte on spi_output_data contains the nRF's status register
                nrf_data.tx_data.eot (true, nrf_data.spi_output_data + 1, nrf_data.tx_data.read_len[1]);
            }

            break;
    }
}

static void nrf24_spiHandleMAXRTCallback(void)
{
    switch (nrf_data.tx_data.callback_stage) {
        case 0:
            // MAX_RT interrupt - the IRQ has been serviced
            GPIOPinIntEnable(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN);

            nrf_data.tx_data.callback_stage += 1;

            // The payload is still in the TX FIFO: it must be flushed out.
            nrf_data.spi_input_data[0] = FLUSH_TX;
            spi_transfer(nrf_data.spi_input_data, NULL, 1, nrf24_spiHandleMAXRTCallback);

            break;

        case 1:
            // The IRQ has been serviced, and the TX FIFO flushed.
            nrf_data.tx_data.transmitting = false;
            if (nrf_data.tx_data.eot != NULL) {
                nrf_data.tx_data.eot(false, NULL, 0);
            }

            break;
    }
}

static void nrf24_spiHandleRXCallback(void)
{
    switch (nrf_data.rx_data.callback_stage) {
        case 0:
            // Either RX_DR or TX_DS + RX_DR interrupts - the IRQ has been serviced
            GPIOPinIntEnable(NRF24_IRQ_GPIO_PORT, NRF24_IRQ_GPIO_PIN);

            nrf_data.rx_data.callback_stage++;

            // There's data in the RX FIFO: first, it's width must be read.
            nrf_data.spi_input_data[0] = R_RX_PL_WID;
            spi_transfer(nrf_data.spi_input_data, nrf_data.rx_data.read_len, 2, nrf24_spiHandleRXCallback);

            break;

        case 1:
            nrf_data.rx_data.callback_stage++;

            // nrf_data.rx_data.read_len[1] contains the amount of bytes in the RX FIFO
            nrf_data.spi_input_data[0] = R_RX_PAYLOAD;
            spi_transfer(nrf_data.spi_input_data, nrf_data.spi_output_data, nrf_data.rx_data.read_len[1] + 1, nrf24_spiHandleRXCallback);

            break;

        case 2:
            // The data has been read

            // The first byte on spi_output_data contains the nRF's status register
            if (nrf_data.rx_data.eot != NULL) {
                nrf_data.rx_data.eot (nrf_data.spi_output_data + 1, nrf_data.rx_data.read_len[1]);
            }

            nrf_data.rx_data.receive_request = false;
            if (nrf_data.rx_data.send_request == true) {
                nrf24_storeAckPayload(nrf_data.rx_data.data, nrf_data.rx_data.length);
            }

            break;
    }
}
