/*
 * fifo.h
 *
 *  Created on: Jan 13, 2023
 *      Author: njacobs
 */

#include <stdint.h>

#ifndef FIFO_H_
#define FIFO_H_

#define FIFOFAIL		0

typedef char tx_dataType;
typedef char rx_dataType;

void tx_fifo_init(void);
uint8_t tx_fifo_put(tx_dataType data);
uint8_t tx_fifo_get(tx_dataType *pdata);
uint32_t tx_fifo_size(void);

void rx_fifo_init(void);
uint8_t rx_fifo_put(rx_dataType data);
uint8_t rx_fifo_get(rx_dataType *pdata);
uint32_t rx_fifo_size(void);


#endif /* FIFO_H_ */
