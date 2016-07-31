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
 * @brief      Informs if the SPI module is busy, and unable to initiate a new
 *             transfer.
 *
 * @return     true if the module is busy, false otherwise.
 */
bool spi_isBusy(void);

void spi_Transfer(uint8_t *input, uint8_t *output, uint8_t length, spi_cb eot);
// Initiates a SPI data transfer. length bytes from input are sent via SPI, and the data clocked in is stored in output.
// When the transmission ends, the SPI module is ready for a new data transfer, and eot is called. eot and input must not be 
// NULL, output can be NULL, in which case it is ignored. length must be non-zero. It is up to the user to ensure there's enough
// memory in both input and output.
// If spi_Transfer is called while there's already another transfer in progress, an error is thrown.

// spi_Transfer can only be called if the SPI module isn't busy. spi_Transfer ensures the module won't be busy
// by the time eot is called.

#endif