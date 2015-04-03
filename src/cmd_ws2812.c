#include "ets_sys.h"
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "espconn.h"
#include "gpio.h"
#include "driver/uart.h" 
#include "microrl.h"
#include "console.h"

#include <stdlib.h>
#include <stdlib.h>
#include <generic/macros.h>

#define WSGPIO 0

void  SEND_WS_0()
{
	uint8_t time = 8;
	WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	while(time--)
	{
		WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );
	}
}

void  SEND_WS_1()
{
	uint8_t time = 9;
	while(time--)
	{
		WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 1 );
	}
	time = 3;
	while(time--)
	{
		WRITE_PERI_REG( PERIPHS_GPIO_BASEADDR + GPIO_ID_PIN(WSGPIO), 0 );
	}
}

void __attribute__((section(".iram0.text"))) WS2812OutBuffer( uint8_t * buffer, uint16_t length )
{
	uint16_t i;
	GPIO_OUTPUT_SET(GPIO_ID_PIN(WSGPIO), 0);
	for( i = 0; i < length; i++ )
	{
		uint8_t byte = buffer[i];
		if( byte & 0x80 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x40 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x20 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x10 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x08 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x04 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x02 ) SEND_WS_1(); else SEND_WS_0();
		if( byte & 0x01 ) SEND_WS_1(); else SEND_WS_0();
	}
	//reset will happen when it's low long enough.
	//(don't call this function twice within 10us)
}

static int  do_ws2812(int argc, const char* const* argv)
{
	//int gpio = atoi(argv[2]);
	
	int r,g,b;
	uint16_t repeat;
	uint8_t buf[3];
	uint16_t i;

	r = atoi(argv[1]);
	g = atoi(argv[2]);
	b = atoi(argv[3]);
	if (argc == 5)
		repeat = atoi(argv[4]);
	else
		repeat = 1;
	buf[0]=g;
	buf[1]=r;
	buf[2]=b;

	os_intr_lock();
	for( i = 0; i < repeat; i++ )
		WS2812OutBuffer((uint8_t*)buf,3);
	os_intr_unlock();

	return 0;
}

//LOCAL void ICACHE_FLASH_ATTR 
static void __attribute__((section(".iram0.text"))) udp_recv(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *pesp_conn = arg;
	/*
	console_printf("client %d.%d.%d.%d:%d -> ", pesp_conn->proto.udp->remote_ip[0],
		   pesp_conn->proto.udp->remote_ip[1],pesp_conn->proto.udp->remote_ip[2],
    		pesp_conn->proto.udp->remote_ip[3],pesp_conn->proto.udp->remote_port);
	console_printf("received %d bytes of data\n", length);
	*/
	//ets_wdt_disable();
	//wdt_feed();
	os_intr_lock();
	WS2812OutBuffer( (uint8_t*)pusrdata, length );
	os_intr_unlock();
	//ets_wdt_enable();
}

LOCAL struct espconn udpconn;

void ws2812srv_start(uint16_t port)
{
	console_printf("Listening (UDP) on port %d\n", port);
	memset(&udpconn, 0, sizeof(struct espconn));
	//espconn_create(&udpconn);
	udpconn.type = ESPCONN_UDP;
	udpconn.proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	udpconn.proto.udp->local_port = port;
	espconn_regist_recvcb(&udpconn, udp_recv);
	espconn_create(&udpconn);
}

static int  do_ws2812srv(int argc, const char* const* argv)
{
	int port = 7777;

	if (argc == 2)
		port = atoi(argv[1]);
	ws2812srv_start(port);
	return 0;
}

CONSOLE_CMD(ws2812, 4, 5, 
	    do_ws2812, NULL, NULL, 
	    "Access WS2812 connected to gpio0. ws2812 r g b [repeat] "
	    HELPSTR_NEWLINE "ws2812 255 0 0       - drive 1 led"
	    HELPSTR_NEWLINE "ws2812 124 100 55 60 - drive 60 leds"
);

CONSOLE_CMD(ws2812srv, 1, 2, 
	    do_ws2812srv, NULL, NULL, 
	    "udp to ws2812 server. ws2812srv [port]"
);

