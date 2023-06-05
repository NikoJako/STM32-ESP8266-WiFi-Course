/*
 * esp82xx_lib.h
 *
 *  Created on: Apr 17, 2023
 *      Author: njacobs
 */

#ifndef ESP82XX_LIB_H_
#define ESP82XX_LIB_H_

#include "esp82xx_driver.h"
#include <stdio.h>

void esp82xx_init(const char * ssid, const char * password);
uint8_t esp82xx_create_tcp_connection(char *ip_address);
uint8_t esp82xx_send_tcp_pckt(char * pckt);
uint8_t esp82xx_close_tcp_connection(void);

#endif /* ESP82XX_LIB_H_ */
