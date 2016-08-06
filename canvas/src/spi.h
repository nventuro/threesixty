#ifndef _SPI_H_
#define _SPI_H_

#include <stdbool.h>
#include <stdint.h>

typedef void (*spi_cb)(void);

/**
 * @brief      Initializes the SPI module in master mode
 *
 * @param[in]  cpol  The clock polarity.
 * @param[in]  cpha  The clock phase.
 * @param[in]  freq  The clock frequency, in Hertz.
 */
void spi_init(bool cpol, bool cpha, uint32_t freq);

/**
 * @brief      Initiates an interrupt-driven, multibyte SPI data transfer. This
 *             call is non-blocking: the transfer will end after the call
 *             returns, so both buffers must persist until it actually finishes.
 *
 *             This function can be safely called from an interrupt context, but
 *             it is not thread-safe, nor reentrant: only one transfer can be
 *             ongoing at a time.
 *
 * @param[in]  write   The write (TX) buffer. This must not be NULL.
 * @param[out] read    The read (RX) buffer. If this is NULL, the received data
 *                     is discarded.
 * @param[in]  length  The number of bytes to transmit.
 * @param[in]  eot     A callback to be called once the transmission finishes.
 *                     At that point, spi_transfer can be called again. eot is
 *                     executed from an interrupt context.
 */
void spi_Transfer(const uint8_t *write, uint8_t *read, uint8_t length, spi_cb eot);

/**
 * @brief      Informs if an SPI transfer is ongoing.
 *
 * @return     true if a transfer is in progress, false if the module is idle
 *             and spi_transfer can be safely called.
 */
bool spi_isBusy(void);

#endif
