/*
 * esp82xx_lib.c
 *
 *  Created on: Apr 17, 2023
 *      Author: njacobs
 *
 *      lastest API can be found here:
 *      https://docs.espressif.com/projects/esp-at/en/release-v2.2.0.0_esp8266/Get_Started/index.html
 */

#include "esp82xx_lib.h"
#include <stdbool.h>
#include "fifo.h"
#include <string.h>
#include <stdlib.h>
#include "stm32f4xx.h"
#include "stm32f411xe.h"		//pin & register definitions

/*buffer to hold all the chars after "+ipd"*/
#define SERVER_RESPONSE_SIZE	1024
#define BUFFER_SIZE				1024
#define MAX_NUM_OF_TRY			10

#define ESP8266_WIFI_MODE_STA			1		//station mode == "client" mode
#define ESP8266_WIFI_MODE_AP			2
#define ESP8266_WIFI_MODE_STA_AND_AP	3


/*Indicies associated with BXBuffer*/
char server_resp_buffer[SERVER_RESPONSE_SIZE];
uint32_t RXBufferIndex = 0;

/* local variables/search flags that will tell us where we're at in our search process
 * used to search for a particular string within our data stream*/
char sub_str[32]; 					//buffer created for wait_resp()
volatile bool searching = false;
volatile bool is_response = false;
volatile int search_idx = 0;

/**/
char server_resp_sub_str[16] = "+ipd,";		//internet datagram protocol, this is what we want to search for in our rx data stream
volatile bool server_search_resp_cmplt = false;
volatile int server_resp_search_idx = 0;
volatile int server_resp_searching = 0;		//state machine

int server_resp_idx = 0;

char temp_buffer[1024];

/*Prototypes - can all be "static" since they'll
 * only be used in this file, won't work if called frmo main.c*/
static void wait_resp(char *pt);

static uint8_t esp82xx_reset(void);
static uint8_t esp82xx_set_wifi_mode(uint8_t mode);
static uint8_t esp82xx_list_access_points(void);
static uint8_t esp82xx_join_wifi_access_point(const char * ssid, const char * password);
static uint8_t esp82xx_get_local_ip_addr();
static uint8_t esp82xx_dns_get_ip(char *website);

static void esp_server_resp_srch_start(void);
static void search_check(char letter);
static void esp_server_resp_srch_check(char letter);
static void copy_software_to_hardware(void);
static void esp82xx_send_cmd(const char * cmd);
static void esp_uart_callback(void);
static void esp82xx_process_data(void);
static void uart_output_char(char data);

/**/
void esp82xx_init(const char * ssid, const char * password)
{
	tx_fifo_init();
	rx_fifo_init();

	/*Enable RS pin*/
	esp_rs_pin_init();

	/*Enable esp uart*/
	esp_uart_init();

	/*Enable debug uart*/
	debug_usart2_init();

	/*Initialize Flags*/
	searching = false;
	is_response = false;
	server_resp_searching = 0;
	server_search_resp_cmplt = 0;

	printf("ESP8266 Initialization...\n\r");

	/*Enable NVIC interrupt for the WiFi module*/
	NVIC_EnableIRQ(USART1_IRQn);

	if(esp82xx_reset() == 0)
	{
		printf("Reset failure, could not reset\r\n");
	}
	else
	{
		printf("Reset was successful...\r\n");
	}

	if(esp82xx_set_wifi_mode(ESP8266_WIFI_MODE_STA) == 0)
	{
		printf("SetFiFiMode failed\r\n");
	}
	else
	{
		printf("SetWiFiMode set successfully");
	}

	esp82xx_list_access_points();

	/*Join WiFi*/
	if(esp82xx_join_wifi_access_point(ssid, password) == 0)
	{
		printf("Could NOT join WiFi\r\n");
	}
	else
	{
		printf("WiFi Joined Successfully....\r\n");
	}

	//esp82xx_get_local_ip_addr();

	/*Test getting the IP address of "google.com"*/
	//esp82xx_dns_get_ip("google.com");
}

/*Reset esp module*/
static uint8_t esp82xx_reset(void)
{
	uint8_t num_of_try = MAX_NUM_OF_TRY;

	/*If "ok" doesn't work, try "ready"
	 * try "OK\r\n", THIS WORKED-->"ok\r\n" */
	wait_resp("ok\r\n");

	while(num_of_try)
	{
		/*set reset pin LOW*/
		esp_rs_pin_enable();

		/*wait - call SysTick*/
		systick_delay_ms(10);

		/*set reset pin HIGH*/
		esp_rs_pin_disable();

		/*Send RST command*/
		esp82xx_send_cmd("AT+RST\r\n");

		/*wait - call SysTick*/
		systick_delay_ms(500);

		/*Check for response*/
		if(is_response)
		{
			/*success*/
			return 1;
		}
		else
		/*decrement the # of tries*/
		num_of_try--;

	}
	/*if nothing works*/
	return 0;
}


/*Set Wifi Mode*/
static uint8_t esp82xx_set_wifi_mode(uint8_t mode)
{
	uint8_t num_of_try = MAX_NUM_OF_TRY;
	wait_resp("ok\r\n");

	while(num_of_try)
	{
		/*Combine AT+MODE
		 * sprintf(char *buffer, const char *format, ... );
		 * buffer -	pointer to a character string to write to
		 * format - pointer to a null-terminated byte string specifying how to interpret the data*/
		sprintf((char *)temp_buffer, "AT+CWMODE=%d\r\n", mode);
		esp82xx_send_cmd((char *)temp_buffer);						//in the video he puts a "const" in front of "char"
		systick_delay_ms(500);
		if(is_response)
		{
			/*success*/
			return 1;
		}
		/*decrement the # of tries*/
		num_of_try--;

	}

	/*if nothing works*/
	return 0;

}

/*List access points*/
static uint8_t esp82xx_list_access_points(void)
{
	uint8_t num_of_try = MAX_NUM_OF_TRY;
	wait_resp("ok\r\n");

	while(num_of_try)
	{
		esp82xx_send_cmd("AT+CWLAP\r\n");						//in the video he puts a "const" in front of "char"
		systick_delay_ms(5000);
		if(is_response)
		{
			/*success*/
			return 1;
		}
		/*decrement the # of tries*/
		num_of_try--;

	}
	/*if nothing works*/
	return 0;
}
/*Join Access points - Execute Command*/
static uint8_t esp82xx_join_wifi_access_point(const char * ssid, const char * password)
{
	uint8_t num_of_try = MAX_NUM_OF_TRY;
	wait_resp("ok\r\n");

	while(num_of_try)
	{
		/*Combine AT+MODE
		 * sprintf(char *buffer, const char *format, ... );
		 * buffer -	pointer to a character string to write to
		 * format - pointer to a null-terminated byte string specifying how to interpret the data*/
		sprintf((char *)temp_buffer, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid,password);
		esp82xx_send_cmd((char *)temp_buffer);						//in the video he puts a "const" in front of "char"
		systick_delay_ms(3000);
		if(is_response)
		{
			/*success*/
			return 1;
		}
		/*decrement the # of tries*/
		num_of_try--;

	}

	/*if nothing works*/
	return 0;


}
/*Get Local IP address*/
static uint8_t esp82xx_get_local_ip_addr()
{
	uint8_t num_of_try = MAX_NUM_OF_TRY;
	wait_resp("ok\r\n");

	while(num_of_try)
	{
		esp82xx_send_cmd("AT+CIFSR\r\n");
		systick_delay_ms(3000);
		if(is_response)
		{
			/*success*/
			return 1;
		}
		/*decrement the # of tries*/
		num_of_try--;

	}

	/*if nothing works*/
	return 0;
}

/*Create TCP connection
 * AT+CIPSTART=<"type">,<"remote host">,<remote port>[,<keep alive>][,<"local IP">]*/
uint8_t esp82xx_create_tcp_connection(char *ip_address)
{
	uint8_t num_of_try = MAX_NUM_OF_TRY;
	wait_resp("ok\r\n");

	while(num_of_try)
	{
		sprintf((char *)temp_buffer, "AT+CIPSTART=\"TCP\",\"%s\",80\r\n", ip_address);
		esp82xx_send_cmd((char *)temp_buffer);											//in the video he puts a "const" in front of "char"
		systick_delay_ms(3000);
		if(is_response)
		{
			/*success*/
			return 1;
		}
		/*decrement the # of tries*/
		num_of_try--;

	}

	/*if nothing works*/
	return 0;
}

/*Send TCP Packet to remote server*/
uint8_t esp82xx_send_tcp_pckt(char * pckt)
{
	/*First combine the packet and command together in the same string (Set command)*/
	sprintf((char *)temp_buffer, "AT+CIPSEND=%d\r\n",strlen(pckt));

	/*Send "AT+CIPSEND=<pckt lenght>"*/
	esp82xx_send_cmd(temp_buffer);
	systick_delay_ms(50);

	/*send packet (the actual data)*/
	esp82xx_send_cmd(pckt);

	/*Now search for the response, initialize server response search
	 * I don't understand why we didn't have to call this is previous
	 * functions where AT commands are being sent?*/
	esp_server_resp_srch_start();

	/*wait for the response*/
	while(server_search_resp_cmplt == false)
	{
		systick_delay_ms(1);
		printf("Waiting for response to TCP packet, esp82xx_send_tcp_pckt\r\n");
	}

	if(server_search_resp_cmplt == false)
	{
		return 0;
	}
	else
	{
		return 1;
	}

}

/*Close TCP connection*/
uint8_t esp82xx_close_tcp_connection(void)
{
	uint8_t num_of_try = MAX_NUM_OF_TRY;
	wait_resp("ok\r\n");

	while(num_of_try)
	{
		esp82xx_send_cmd("AT+CIPCLOSE\r\n");
		systick_delay_ms(3000);
	}

	if(is_response)
	{
		/*success*/
		return 1;
	}
	/*decrement the # of tries*/
	num_of_try--;

	/*if nothing works*/
	return 0;
}

/*Get Domain Name IP Address*/
static uint8_t esp82xx_dns_get_ip(char *website)
{
	uint8_t num_of_try = MAX_NUM_OF_TRY;
	wait_resp("ok\r\n");

	while(num_of_try)
	{
		sprintf((char *)temp_buffer, "AT+CIPDOMAIN=\"%s\"\r\n",website);
		esp82xx_send_cmd(temp_buffer);
		systick_delay_ms(3000);
	}

	if(is_response)
	{
		/*success*/
		return 1;
	}
	/*decrement the # of tries*/
	num_of_try--;

	/*if nothing works*/
	return 0;
}


/*Initialize string search for the SERVER RESPONSE
 * whenever we want to start a search
 * we want to copy the substring that we want to search
 * and place it into our server_resp_sub_str[16]*/
static void esp_server_resp_srch_start(void)
{
	/* We don't know the state of the above local variables when this function is called. Other functions or searches
	 * could have been executed changing the default values to something else. Therefore they need to be re-initialized
	 * in this function.
	 * 00:07:00 Section 9 lecture # 50
	 */
	strcpy(server_resp_sub_str, "+ipd,");
	server_resp_search_idx = 0;
	server_resp_searching = 1;
	server_search_resp_cmplt = false;
	server_resp_idx = 0;
}

/* Initialize string search in Rx data stream
 * takes in a string that we want to search for in the data stream
 * we send the expected response to some AT command we're about to
 * send to the esp8266 chip to be searched for in the Rx data stream*/
static void wait_resp(char *pt)
{
	/*copy the substring we want to find in the data stream into the buffer(sub_str[32])
	 * strcpy(dest,source)*/
	strcpy(sub_str, pt);

	/*Initialize search flags*/
	search_idx = 0;
	is_response = false;
	searching = true;
}

/*Convert to lowercase*/
char lc(char letter)
{
	if((letter >= 'A')&&(letter <='Z'))
	{
		letter |= 0x20;
	}
	return letter;
}


/*Search for string in Rx data stream*/
static void search_check(char letter)
{
	if(searching)
	{
		/*compare letter to sub_str*/
		if(sub_str[search_idx] == lc(letter))
		{
			search_idx++;
			if(sub_str[search_idx] == 0)
			{
				is_response = true;
				searching = false;
			}
		}
		else
		{
			/*start over*/
			search_idx = 0;
		}
	}
}


/*Look for server response in Rx data stream*/
static void esp_server_resp_srch_check(char letter)
{
	if(server_resp_searching == 1)
	{
		/*check if characters match*/
		if(server_resp_sub_str[server_resp_search_idx] == lc(letter))
		{
			server_resp_search_idx++;
			/*check if strings match*/
			if(server_resp_sub_str[server_resp_search_idx] == 0)
			{
				server_resp_searching = 2;
				strcpy(server_resp_buffer, "\n\rok\r\n");
				server_resp_search_idx = 0;
			}
		}
		else
		{
			/*start over*/
			server_resp_search_idx = 0;
		}
	}
	else if (server_resp_searching == 2)
	{
		if(server_resp_idx < SERVER_RESPONSE_SIZE)
		{
			server_resp_buffer[server_resp_idx] = lc(letter);
			server_resp_idx++;
		}
		if(server_resp_sub_str[server_resp_search_idx] == lc(letter))
		{
			server_resp_search_idx++;

			/*check if strings match*/
			if(server_resp_sub_str[server_resp_search_idx] == 0)
			{
				server_search_resp_cmplt = true;
				server_resp_searching = 0;
			}
		}
		else
		{
			/*start over*/
			server_resp_search_idx = 0;
		}


	}
}

/*Copy content of tx FIFO into DEBUG UART DR*/
static void copy_software_to_hardware(void)
{
	char letter;

	/*wait for TXE of DEBUG USART (USART2)
	 * if its empty send new data*/
	while((USART2->SR & USART_SR_TXE_Msk)&& tx_fifo_size() > 0)
	{
		/*getting the data from the tx_fifo and
		 * storing in local variable...*/
		tx_fifo_get(&letter);

		/*and then send letter to the debug usart*/
		USART2->DR = letter;
	}
}

/*Output UART character */
static void uart_output_char(char data)
{
	/*attempts to put the data we want to output (data)
	 *in the tx_fifo/data stream so it can be transmitted
	 *in over WiFi*/
	if(tx_fifo_put(data) == FIFOFAIL)
	{
		return;
	}

	/*Put data in the debug fifo*/
	copy_software_to_hardware();
}

/*copy content of the data reg to the FIFO
 * something about being called when an interrupt is called*/
static void esp82xx_process_data(void)
{
	char letter;

	/*Check if there is new data in wifi uart data register*/
	if(USART1->SR & (USART_SR_RXNE_Msk))
	{
		/*Store data from wifi uart data register
		 * and store it in letter */
		letter = USART1->DR;

		/*print data from wifi uart data register
		 *to debug usart*/
		uart_output_char(letter);

		/*check for response*/
		search_check(letter);

		/*Check for server response*/
		esp_server_resp_srch_check(letter);

	}
}

/* Callback function for esp82xx uart*/
static void esp_uart_callback(void)
{
	esp82xx_process_data();

}

/*esp82xx uart IRQhandler*/
void USART1_IRQHandler(void)
{
	esp_uart_callback();
}

/*Send command to esp82xx wifi uart module*/
static void esp82xx_send_cmd(const char * cmd)
{
	int index = 0;

	while(cmd[index] != 0)
	{
		esp_usart1_write_char(cmd[index++]);
	}
}







