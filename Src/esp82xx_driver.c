/*
 * esp82xx_driver.c
 *
 *  Created on: Jan 4, 2023
 *      Author: njacobs
 */

#include "stm32f4xx.h"
#include "stm32f411xe.h"
#include "esp82xx_driver.h"

/*These will only work in this file*/
#define GPIOAEN				(1U<<0)

#define AF7					7
#define CR1_UE				(1U<<13)
#define SYS_FREQ			16000000	//internal clock source
#define APB1_CLK			SYS_FREQ
#define APB2_CLK			SYS_FREQ

#define UART_BAUDRATE		115200
#define CR1_TE				(1U<<3)
#define CR1_RE				(1U<<2)

#define SYSTICK_LOAD_VAL	16000	//default for the STM32F411
#define CTRL_ENABLE			(1U<<0)
#define CTRL_CLKSRC			(1U<<2) 	//internal clock source
#define CTRL_COUNT_FLAG		(1<<16)

static uint16_t compute_usart_baud(uint32_t periph_clk, uint32_t baudrate);

int __io_putchar(int ch)
{
	debug_usart2_write(ch);
	return ch;
}

//APB1
void debug_usart2_init(void)
{
	/***********Enabling Clock Access***********/
	/*Enable clock access to the USART pins on appropriate GPIO port (port A)*/
	RCC->AHB1ENR |= GPIOAEN;

	/***********Configuring GPIO Pins for Alternate Function***********/
	/*Set PA2 mode to alt_func mode-->MODER3 = 2 0b10
	 * later change to (PinMode << (2 * PinNumber))*/
	GPIOA->MODER |= (2 << GPIO_MODER_MODER2_Pos); //4th bit
	/*Set PA3 mode to alt_func mode-->MODER3 = 2*/
	GPIOA->MODER |= (2 << GPIO_MODER_MODER3_Pos); //6th bit

	/*Set PA2 mode to alt_func type to USART2_TX (AF07 = 0111)*/
	GPIOA->AFR[0] |= (AF7 << GPIO_AFRL_AFSEL2_Pos);
	/*Set PA3 mode to alt_func type to USART2_RX (AF07 = 0111)*/
	GPIOA->AFR[0] |= (AF7 << GPIO_AFRL_AFSEL3_Pos);

	/***********Configure USART Module***********/
	/*Enable clock access to USART Module*/
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN_Msk;  //(0x1UL << RCC_APB1ENR_USART2EN_Pos, where Pos = 17U) )

	/*Disable USART Module*/
	USART2->CR1 &= ~CR1_UE;

	/*Set USART baudrate - 115200*/
	USART2->BRR = compute_usart_baud(APB1_CLK,UART_BAUDRATE);

	/*Set transfer direction*/
	USART2->CR1 = (CR1_TE | CR1_RE);

	/*Enable USART2 interrupt in NVIC - this is a 1/2 steps*/
	/*Enable interrupt in the NVIC*/
	//NVIC_EnableIRQ(USART2_IRQn);

	/*Enable USART module*/
	USART2->CR1 |= CR1_UE;

}

/*Pinout :
 * UART Module     :  	UART1
 * UART Pins   	   : 	PA9 = TX, PA10 = RX
 * esp pin		    	stm32f4 pin
 * --------------------------
 * ESP82XX RS  Pin  : 	PA8
 * ESP82XX EN  Pin  :	3.3V
 * ESP82XX IO1 Pin  :	3.3V
 * ESP82XX IO2 Pin  :	3.3V
 * ESP82XX VCC Pin  :	3.3V
 * ESP82XX GND Pin  :	GND
 * ESP82XX TX  Pin  :	PA10 (RX)
 * ESP82XX RX  Pin  :	PA9 (TX)
 * */

/*Setting to PA8 to a constant 3.3v prevents the "board from being flushed
 * the workaround is to remove power to the so the MCU can be detected
 * again"  */
void esp_rs_pin_init(void)
{
	/*Enable clock access to GPIOA*/
	RCC->AHB1ENR |= GPIOAEN;

	/*Set PA8 as an output pin, 01 to bits 16 & 17*/
	GPIOA->MODER |= (1U << GPIO_MODER_MODER8_Pos);
	GPIOA->MODER &= ~(1U << 17);
}

void esp_rs_pin_enable(void)
{
	/*Set PA8 HIGH*/
	GPIOA->ODR = (1U << 8);
}

void esp_rs_pin_disable(void)
{
	/*Set PA8 HIGH*/
	GPIOA->ODR = (1U << 8);
}

void esp_uart_init(void)
{
	/***********Enabling Clock Access***********/
	/*Enable clock access to the USART pins on appropriate GPIO port (port A)*/
	RCC->AHB1ENR |= GPIOAEN;

	/***********Configuring GPIO Pins for Alternate Function***********/
	/*Set PA9 (Tx) mode to alt_func mode-->MODER3 = 2 0b10
	 * later change to (PinMode << (2 * PinNumber))*/
	GPIOA->MODER |= (2 << GPIO_MODER_MODER9_Pos); //9th bit
	/*Set PA10 (Rx) mode to alt_func mode-->MODER3 = 2*/
	GPIOA->MODER |= (2 << GPIO_MODER_MODER10_Pos); //10th bit

	/*Set PA9 mode to alt_func type to USART1_TX (AF07 = 0111)*/
	GPIOA->AFR[1] |= (AF7 << GPIO_AFRH_AFSEL9_Pos);
	/*Set PA10 mode to alt_func type to USART1_RX (AF07 = 0111)*/
	GPIOA->AFR[1] |= (AF7 << GPIO_AFRH_AFSEL10_Pos);

	/***********Configure USART Module***********/
	/*Enable clock access to USART1 Module (APB2 max 100MHz)*/
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN_Msk;  //(0x1UL << RCC_APB1ENR_USART2EN_Pos, where Pos = 17U) )

	/*Disable USART Module*/
	USART1->CR1 &= ~CR1_UE;

	/*Set USART baudrate - 115200*/
	USART1->BRR = compute_usart_baud(APB2_CLK,UART_BAUDRATE);

	/*Set transfer direction*/
	USART1->CR1 = (CR1_TE | CR1_RE);

	/*Enable RXNEIE Interrupt*/
	USART1->CR1 |= (1U << USART_CR1_RXNEIE_Pos);

	/*Enable USART1 interrupt in NVIC - this is a 1/2 steps*/
	/*Enable interrupt in the NVIC
	 * MOved to the new library file*/
	//NVIC_EnableIRQ(USART1_IRQn);

	/*Enable USART module*/
	USART1->CR1 |= CR1_UE;
}

void systick_delay_ms(uint32_t delay)
{
	/*Reload the number of clocks per millisecond*/
	SysTick->LOAD = SYSTICK_LOAD_VAL;

	/*Clear systick current value register*/
	SysTick->VAL = 0;

	/*Enable systick and select internal clk source*/
	SysTick->CTRL = CTRL_CLKSRC | CTRL_ENABLE;

	for(int i = 0; i < delay; i++)
	{
		/*Wait for the count flag to set:
		 *this will tell us that a timeout has occured */
		while((SysTick->CTRL & CTRL_COUNT_FLAG) == 0){};
	}
	/*Disable SysTick*/
	SysTick->CTRL = 0;
}

void esp_usart1_write_char(char ch)
{
	/* Make sure the transmit data register is empty
	 * Ensure that TXE == 1 before writing new data
	 * if the TXE bit of the SR is 1,
	 * that means data has been sent to the shift register and is about to be sent out
	 * Therefore the while loop is skipped and the program writes data
	 * if TXE == 0, data is still in the transmit data register and the program hangs
	 * at the while loop until TXE == 1*/
	while(!(USART1->SR & SR_TXE));

	/* Write to transmit data register*/
	USART1->DR = (ch & 0xFF);
}

void debug_usart2_write(int ch)
{
	/* Make sure the transmit data register is empty
	 * Ensure that TXE == 1 before writing new data
	 * if the TXE bit of the SR is 1,
	 * that means data has been sent to the shift register and is about to be sent out
	 * Therefore the while loop is skipped and the program writes data
	 * if TXE == 0, data is still in the transmit data register and the program hangs
	 * at the while loop until TXE == 1*/
	while(!(USART2->SR & SR_TXE));

	/* Write to transmit data register*/
	USART2->DR = (ch & 0xFF);
}

/* No need to use this function outside of this file therefore set to static*/
static uint16_t compute_usart_baud(uint32_t periph_clk, uint32_t baudrate)
{
	return ((periph_clk + (baudrate/2U))/baudrate);

}
