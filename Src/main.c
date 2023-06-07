
#include <stdio.h>
#include "stm32f4xx.h"
#include "esp82xx_driver.h"
#include "esp82xx_lib.h"
#include "fifo.h"

#define SSID_NAME	"A_Hills"
#define PASSKEY		"1991GMCSyclone@@"

/*Create the packet, get request*/
char packet_to_send[] = "GET /data/2.5/weather?q=Anaheim&APPID=05d09ab68a862017a503d9c1668239e5 HTTP/1.1\r\nHost:api.openweathermap.org\r\n\r\n";
//char packet_to_send[] = "GET http://api.openweathermap.org/geo/1.0/direct?q=London&limit=5&appid=a7e82190046c49902ae15d5121197a24";
//char packet_to_send[] = "GET data/2.5/weather?lat=44.34&lon=10.99&appid=05d09ab68a862017a503d9c1668239e5";


		/* Default weather api key - 05d09ab68a862017a503d9c1668239e5
		 * New weather api key - a7e82190046c49902ae15d5121197a24*/

int main (void)
{
	esp82xx_init(SSID_NAME, PASSKEY);

	while(1)
	{
		/*Create TCP connection to openWeather.org*/
		if(esp82xx_create_tcp_connection("api.openweathermap.org"))
		{
			esp82xx_send_tcp_pckt(packet_to_send);
		}

		esp82xx_close_tcp_connection();

		return 0;
	}

}



