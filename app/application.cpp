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
#include "SPIDDS.h"
#include "SPIMove.h"
#include "SPIAI.h"

#include "../include/EnableDebug.h"
#include "webServer.h"

#define debug

// If you want, you can define WiFi settings globally in Eclipse Environment Variables
#ifndef WIFI_SSID_Daheim
	#define WIFI_SSID_Daheim "daham2" // Put you SSID and Password here
	#define WIFI_PWD_Daheim "47110815"
#endif

#define SPI_MISO 3	/* Master In Slave Out */
#define SPI_MOSI 1	/* Master Out Slave In */
#define SPI_CLK  0	/* Serial Clock */
#define SPI_CS  2	/* Slave Select */
#define SPI_DELAY 10	/* Clock Delay */
#define SPI_BYTE_DELAY 10 /* Delay between Bytes */


Timer procTimer;			// cyclic Timer processing SPI loop and devices
SPISoft *pSoftSPI = NULL;

// Devices on the SPI loop
SPI_DDS  myDDS;
SPI_Move myMove;
SPI_AI   myAI;
unsigned char SPIChainLen;

// debug output instead of UART
TelnetServer telnet;
EnableDebug enableDebug;	// enable debug output command


unsigned char bytes[16];	// bytes received through websocket
unsigned char SPI_Buffer[16] = {0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char *pBuffer, *pSource;

// cyclic loop
void loop() {
	pBuffer = SPI_Buffer;

	// copy out data from devices to SPI buffer
	// inverse order of chain
	pSource = myDDS.getSPIOutBuffer();
	memcpy(pBuffer, pSource, myMove.getSPIBufferLen());
	pBuffer += myMove.getSPIBufferLen();

	pSource = myAI.getSPIOutBuffer();
	memcpy(pBuffer, pSource, myAI.getSPIBufferLen());
	pBuffer += myAI.getSPIBufferLen();

	pSource = myMove.getSPIOutBuffer();
	memcpy(pBuffer, pSource, myMove.getSPIBufferLen());
	pBuffer += myMove.getSPIBufferLen();

#ifdef debug	// use bytes from websocked instead of SPI devices
	memcpy(SPI_Buffer, bytes, sizeof(SPI_Buffer));
#endif

	// show SPI Output before sending
	Debug.printf("SPI OUT:");
	for (int i=0;i<SPIChainLen;i++) {
		Debug.printf("%x ",SPI_Buffer[i]);
	}
	Debug.printf("\n\r");

	// transmit SPI_Buffer
	pSoftSPI->beginTransaction(pSoftSPI->SPIDefaultSettings);
	pSoftSPI->transfer(SPI_Buffer,SPIChainLen);
	pSoftSPI->endTransaction();

	// show SPI Input after receiving
	Debug.printf("SPI IN:");
	for (int i=0;i<SPIChainLen;i++) {
		Debug.printf("%x ",SPI_Buffer[i]);
	}
	Debug.printf("\n\r");

	pBuffer = SPI_Buffer;

	// copy received data from SPI_Buffer to devices
	myMove.setSPIInBuffer(pBuffer);
	pBuffer += myMove.getSPIBufferLen();

	myAI.setSPIInBuffer(pBuffer);
	pBuffer += myAI.getSPIBufferLen();

	myDDS.setSPIInBuffer(pBuffer);
	pBuffer += myDDS.getSPIBufferLen();
}

// Will be called when WiFi station was connected to AP
void connectOk()
{
	startWebServer();

	telnet.listen(23);
	enableDebug.initCommand();
}


void init()
{
	spiffs_mount(); // Mount file system, in order to work with files

	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(false); //Allow debug output to serial

	// disable UART0 -> used as GPIO for SPI
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
	pinMode(SPI_MISO, INPUT_PULLUP);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
	pinMode(SPI_MOSI, INPUT_PULLUP);

	// start debug, will be used through telnet
	Debug.start();

	// initialize Soft SPI
	pSoftSPI = new SPISoft(SPI_MISO, SPI_MOSI, SPI_CLK, SPI_CS, SPI_DELAY, SPI_BYTE_DELAY);
	if(pSoftSPI)
	{
		delay(100);

		pSoftSPI->begin();
		Debug.printf("SPI is initialized now.");

		SPIChainLen = myMove.getSPIBufferLen() + myAI.getSPIBufferLen() + myDDS.getSPIBufferLen();
		if (SPIChainLen > sizeof(SPI_Buffer)) return;

		//start listen loop
		procTimer.initializeMs(100, loop).start();
	}
	else Serial.print("Error not enough heap\n");

	WifiStation.enable(true);
	WifiStation.config(WIFI_SSID_Daheim, WIFI_PWD_Daheim);
	WifiAccessPoint.enable(false);

	commandHandler.registerSystemCommands();

	// Run our method when station was connected to AP
	WifiStation.waitConnection(connectOk);
}
