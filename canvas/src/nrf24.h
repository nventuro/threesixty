#ifndef _NRF24_H_
#define _NRF24_H_

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    NRF24_TX,
    NRF24_RX
} nrf24_mode_t;

/**
 * @brief      Enables the nrf24 module. The module may only be initialized
 *             once, with only one mode. Only Pipe 0 is used.
 *
 * @param[in]  mode  The mode in which the module will run.
 */
void nrf24_init(nrf24_mode_t mode);

// TX functions


/*
 * @brief      A callback for tx calls.
 *
 * @param[in]  success      true if an ACK packet was received from the RX
 *                          module, false if the maximum number of
 *                          retransmissions hace occured.
 * @param[in]  ack_payload  If the RX module answers with payload, this will be
 *                          non-NULL, and point to said payload. This must be
 *                          read before calling any other nrf24 function is
 *                          called.
 * @param[in]  length       The length of ack_payload, in bytes.
 */
typedef void (*nrf24_tx_cb_t) (bool success, uint8_t *ack_payload, uint8_t length);

/**
 * @brief      Initializes a transmission to Pipe 0.
 *
 * @param[in]  data    The data to be sent.
 * @param[in]  length  The length of the data, in bytes. This must not be
 *                     greater than 32.
 * @param[in]  eot     A callback to be called once the transmission ends. At
 *                     this point, transmit can be called again.
 */
void nrf24_transmit(const uint8_t *data, uint8_t length, nrf24_tx_cb_t eot);

bool nrf24_isBusy(void);
// Informs wheter the module is busy. nrf24_.Transmit can only be called if IsBusy is false. After eot is called in
// nrf24_Transmit, IsBusy will be false.
// IsBusy can only be called on PTX mode.

// RX functions


typedef void (*nrf24_rx_cb_t) (uint8_t *data, uint8_t length);

void nrf24_receive(nrf24_rx_cb_t eot);
// Registers a callback for reception on Pipe 0. When data is received, it is stored in data, and eot is called. length
// (eot's argument) contains the number of bytes received, which are stored in data.
// data must be read before any call to any other nrf function is done.
// nrf24_Receive can only be called in PRX mode.

void nrf24_storeAckPayload(const uint8_t *data, uint8_t length);
// Uploads to the device the payload that will be sent with the next ACK, stored in data, of length bytes.

#endif
