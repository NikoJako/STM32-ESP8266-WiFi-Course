/*
 * fifo.c
 *
 *  Created on: Jan 13, 2023
 *      Author: njacobs
 */

#include "fifo.h"

#define TX_FIFO_SIZE	1024
#define RX_FIFO_SIZE	1024
#define TX_FAIL			0
#define TX_SUCCESS		1
#define RX_FAIL			0
#define RX_SUCCESS		1


/*The actual FIFOs*/
tx_dataType static TX_FIFO[TX_FIFO_SIZE];
rx_dataType static RX_FIFO[RX_FIFO_SIZE];

/*Define Iterators to help put and get data in/out of FIFO*/
uint32_t volatile tx_put_itr;
uint32_t volatile tx_get_itr;
uint32_t volatile rx_put_itr;
uint32_t volatile rx_get_itr;

/*Initialize tx FIFO*/
void tx_fifo_init(void)
{
	tx_put_itr = 0;
	tx_get_itr = 0;

}

/*Put data into the tx FIFO*/
uint8_t tx_fifo_put(tx_dataType data)
{
	/*Check if fifo is full*/
	if((tx_put_itr - tx_get_itr) & ~(TX_FIFO_SIZE - 1))
	{
		/*FIFO is full*/
		return TX_FAIL;
	}

	/*Put data into the FIFO*/
	TX_FIFO[(tx_put_itr) & (TX_FIFO_SIZE -1)] = data;

	tx_put_itr++;

	return (TX_SUCCESS);

}

/* argument is a pointer to the data we get from the FIFO */
uint8_t tx_fifo_get(tx_dataType *pdata)
{
	/*Check if FIFO is empty*/
	if(tx_put_itr == tx_get_itr)
	{
		/*FIFO empty*/
		return(TX_FAIL);
	}

	/*get the data*/
	*pdata = TX_FIFO[tx_get_itr & (TX_FIFO_SIZE - 1)];

	/*increment the itr*/
	tx_get_itr++;

	return(TX_SUCCESS);
}

uint32_t tx_fifo_size(void)
{
	return (uint32_t)(tx_put_itr - tx_get_itr);
}

/*Initialize rx FIFO*/
void rx_fifo_init(void)
{
	rx_put_itr = 0;
	rx_get_itr = 0;

}

/*Put data into the rx FIFO*/
uint8_t rx_fifo_put(rx_dataType data)
{
	/*Check if fifo is full*/
	if((rx_put_itr - rx_get_itr) & ~(RX_FIFO_SIZE - 1))
	{
		/*FIFO is full*/
		return (RX_FAIL);
	}

	/*Put data into the FIFO*/
	RX_FIFO[rx_put_itr & (RX_FIFO_SIZE -1)] = data;

	rx_put_itr++;

	return (RX_SUCCESS);

}

/* argument is a pointer to the data we get from the FIFO */
uint8_t rx_fifo_get(rx_dataType *pdata)
{
	/*Check if FIFO is empty*/
	if(rx_put_itr == rx_get_itr)
	{
		/*FIFO empty*/
		return(RX_FAIL);
	}

	/*get the data*/
	*pdata = RX_FIFO[rx_get_itr & (RX_FIFO_SIZE - 1)];

	/*increment the itr*/
	rx_get_itr++;

	return(RX_SUCCESS);
}

uint32_t rx_fifo_size(void)
{
	return (uint32_t)(rx_put_itr - rx_get_itr);
}
