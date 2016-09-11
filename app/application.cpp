/*
Modified by: (github.com/)ADiea
Project: Sming for ESP8266 - https://github.com/anakod/Sming
License: MIT
Date: 15.08.2015
Descr: Example applicatin for radio module Si4432 aka RF22 driver
Link: http://www.electrodragon.com/w/SI4432_433M-Wireless_Transceiver_Module_%281.5KM_Range,_Shield-Protected%29
*/

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Network/TelnetServer.h>
#include <string.h>

#include "../include/EnableDebug.h"
#include "webServer.h"

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID_Daheim
	#define WIFI_SSID_Daheim "daham2" // Put you SSID and Password here
	#define WIFI_PWD_Daheim "47110815"
#endif

/*(!) Warning on some hardware versions (ESP07, maybe ESP12)
 * 		pins GPIO4 and GPIO5 are swapped !*/
#define SPI_MISO 3	/* Master In Slave Out */
#define SPI_MOSI 1	/* Master Out Slave In */
#define SPI_CLK  0	/* Serial Clock */
#define SPI_CS  2	/* Slave Select */
#define SPI_DELAY 10	/* Clock Delay */
#define SPI_BYTE_DELAY 10 /* Delay between Bytes */

Timer procTimer;
SPISoft *pSoftSPI = NULL;
TelnetServer telnet;
EnableDebug enableDebug;

//#define PING_PERIOD_MS 2000
//#define PING_WAIT_PONG_MS 100
//unsigned long lastPingTime;

unsigned char bytes[16];
unsigned char buffer[16] = {0x00,0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

void loop() {
	memcpy(buffer, bytes, sizeof(buffer));

	Debug.printf("SPI OUT:");
	for (int i=0;i<sizeof(buffer);i++) {
		Debug.printf("%x ",buffer[i]);
	}
	Debug.printf("\n");

	pSoftSPI->beginTransaction(pSoftSPI->SPIDefaultSettings);
	delayMicroseconds(1);
	pSoftSPI->transfer(buffer,sizeof(buffer));
	pSoftSPI->endTransaction();

	Debug.printf("SPI IN:");
	for (int i=0;i<sizeof(buffer);i++) {
		Debug.printf("%x ",buffer[i]);
	}
	Debug.printf("\n");
}

// Will be called when WiFi station was connected to AP
void connectOk()
{
	Serial.println("I'm CONNECTED");
	startWebServer();

	telnet.listen(23);

	enableDebug.initCommand();
}


void init()
{
	spiffs_mount(); // Mount file system, in order to work with files

	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(false); //Allow debug output to serial

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
	pinMode(SPI_MISO, INPUT_PULLUP);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
	pinMode(SPI_MOSI, INPUT_PULLUP);


	Debug.start();

	pSoftSPI = new SPISoft(SPI_MISO, SPI_MOSI, SPI_CLK, SPI_CS, SPI_DELAY, SPI_BYTE_DELAY);
	if(pSoftSPI)
	{
		delay(100);

		//initialise radio with default settings
		pSoftSPI->begin();
		Debug.printf("SPI is initialized now.");

		//start listen loop
		procTimer.initializeMs(100, loop).start();
	}
	else Serial.print("Error not enough heap\n");

	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID_Daheim, WIFI_PWD_Daheim);
	WifiAccessPoint.enable(false);

	commandHandler.registerSystemCommands();
	Debug.setDebug(Serial);

	// Run our method when station was connected to AP
	WifiStation.waitConnection(connectOk);

}
