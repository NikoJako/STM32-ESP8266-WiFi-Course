/*
 * esp82xx_driver.h
 *
 *  Created on: Jan 4, 2023
 *      Author: njacobs
 */

#include <stdint.h>
#include "stm32f4xx.h"

#define SR_TXE			(1U<<7)
#define SR_RNXE			(1U<<5)		//used to check if we have new data

void debug_usart2_init();
void debug_usart2_write(int ch);
void esp_uart_init(void);
void systick_delay_ms(uint32_t delay);
void esp_rs_pin_init(void);
void esp_rs_pin_enable(void);
void esp_rs_pin_disable(void);
void esp_usart1_write_char(char ch);


#ifndef ESP82XX_DRIVER_H_
#define ESP82XX_DRIVER_H_



#endif /* ESP82XX_DRIVER_H_ */
