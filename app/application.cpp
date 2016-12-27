/*
Modified by: (github.com/)ADiea
Project: Sming for ESP8266 - https://github.com/anakod/Sming
License: MIT
Date: 15.08.2015
Descr: Example applicatin for radio module Si4432 aka RF22 driver
Link: http://www.electrodragon.com/w/SI4432_433M-Wireless_Transceiver_Module_%281.5KM_Range,_Shield-Protected%29
*/

#include <user_config.h>
#include "application.h"
#include <SmingCore/SmingCore.h>
#include <SmingCore/Network/TelnetServer.h>
#include <string.h>
#include "SPIDDS.h"
#include "SPIMove.h"
#include "SPIAI.h"
#include "../include/EnableDebug.h"
#include "../include/NtpClient.h"
#include "webServer.h"
#include "credentials.h"

//#define debug
//#define SerialDebug

Timer procTimer;			// cyclic Timer processing SPI loop and devices
unsigned int loopTime;
unsigned int reductionCounter;
unsigned int reduction;
bool sendIndicator = false;
SPISoft *pSoftSPI = NULL;

// Devices on the SPI loop
SPI_DDS  *myDDS = null;
SPI_Move *myMove = null;
SPI_AI   *myAI = null;
unsigned char SPIChainLen=11;
unsigned char usePoti;

// debug output instead of UART
TelnetServer telnet;
EnableDebug enableDebug;	// enable debug output command

//FTPServer myFtp;

// Callback example using defined class ntpClientDemo
ntpClient *ntp;

MODES mode = move;
MODES modePrev = move;
MODES oldMode;

unsigned char enableBytesOut=0;
unsigned char bytes[11];	// bytes received through websocket
unsigned char SPI_Buffer[11] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char *pBuffer, *pSource;


// delay by number of repeats
unsigned char delayCount;
inline void resetDelay() {
	delayCount = 0;
}
bool delayedTransition(unsigned char delay) {
	if (delayCount++ == delay) {
		resetDelay();
		return true;
	} else {
		return false;
	}
}

MODES checkManualMove(MODES actmode) {
	if (myDDS->getDI(0) || myDDS->getDI(1)) {
		oldMode = actmode;
		actmode = potiMag;
		resetDelay();
	}
	return actmode;
}

void setMode(MODES newMode) {
	if ((newMode == move) || (newMode == sync) || (newMode==track) || (newMode==slew)) {
		mode = newMode;
		resetDelay();
	}
}



// cyclic loop
/***************************************************************
 * cyclic loop / main task
 * 	debug mode without SPI device classes (only byte array IO)
 *
 */
#define magOnDelay 100
#define magOffDelay 50
#define posDelay 25

void loop() {

	switch (mode) {
	case slew:							// slew from ASCOM returns to track automatically
	case move:							// manual goto from web GUI stays in goto
		myDDS->setMagnet();
		if (delayedTransition(magOnDelay)) {
			myMove->posControlEnable(1);
			oldMode = mode;
			mode = moving;
		}
		break;


	case moving:
		if (oldMode != move) {
			if (myMove->setPositionReached(0) && myMove->setPositionReached(1)) {
				if (delayedTransition(posDelay)) {
					mode = track;
				}
			} else {
				resetDelay();
			}
		}
		break;

	case sync:
		myMove->posControlEnable(0);
		myMove->setPWM(0,0);
		myMove->setPWM(1,0);
		if (delayedTransition(magOffDelay)) {
			myDDS->clrMagnet();
			mode = syncing;
		}
		break;
	case syncing:
		myMove->setReference(0);
		myMove->setReference(1);

		mode = checkManualMove(mode);
		break;
	case track:
		myMove->posControlEnable(0);
		myMove->setPWM(0,0);
		myMove->setPWM(1,0);

		if (delayedTransition(magOffDelay)) {
			myDDS->clrMagnet();
			mode = tracking;
		}
		break;
	case tracking:
		mode = checkManualMove(mode);
		break;

	case potiMag:
		myDDS->setMagnet();
		if (delayedTransition(magOnDelay)) {
			mode = potiMove;
		}
		break;
	case potiMove:
		if (myDDS->getDI(0)) {
			myMove->setPWM(0, (myAI->getAI(0)>>8)-128);
			resetDelay();
		} else {
			myMove->setPWM(0,0);
		}
		if (myDDS->getDI(1)) {
			myMove->setPWM(1, (myAI->getAI(1)>>8)-128);
			resetDelay();
		} else {
			myMove->setPWM(1,0);
		}
		if (!myDDS->getDI(1) && !myDDS->getDI(1)) {
			if (delayedTransition(magOffDelay)) {
				mode = (MODES)(oldMode%10);
			}
		}
		break;
	default:
		break;
	}

	if (mode != modePrev) {
		char modeString[2];
		modeString[0] = '0' + mode%10;
		modeString[1] = 0;
		sendMessage("mode", modeString);
		Debug.print("mode");
		Debug.println(modeString);
		modePrev = mode;
	}

	if (enableBytesOut) {
		// copy debug data to SPI buffer
		memcpy(SPI_Buffer, bytes, sizeof(SPI_Buffer));
	}else {

		pBuffer = SPI_Buffer;

		// copy out data from devices to SPI buffer
	 	pSource = myDDS->getSPIBuffer();
		memcpy(pBuffer, pSource, myDDS->getSPIBufferLen());
		pBuffer += myDDS->getSPIBufferLen();

		pSource = myAI->getSPIBuffer();
		memcpy(pBuffer, pSource, myAI->getSPIBufferLen());
		pBuffer += myAI->getSPIBufferLen();

		pSource = myMove->getSPIBuffer();
	 	memcpy(pBuffer, pSource, myMove->getSPIBufferLen());
		pBuffer += myMove->getSPIBufferLen();
	}

	// reduction of html and debug interface to 1s cycle time
	reduction = 500 / loopTime;
	sendIndicator = 0;
	reductionCounter++;
	if (reductionCounter >= reduction) {
		sendIndicator = 1;
		reductionCounter = 0;
	}

	if (debugOpen && sendIndicator) {
		sendSPIData(false, SPI_Buffer);
	}

	// transmit SPI_Buffer
	delayMicroseconds(SPI_DELAY);
#ifndef SerialDebug
	pSoftSPI->beginTransaction(pSoftSPI->SPIDefaultSettings);
	pSoftSPI->transfer(SPI_Buffer,SPIChainLen);
	pSoftSPI->endTransaction();
#endif

	if (debugOpen && sendIndicator) {
		sendSPIData(true, SPI_Buffer);
	}

	if (usePoti) {
		myMove->setPWM(0, (myAI->getAI(0)>>8)-128);
		myMove->setPWM(1, (myAI->getAI(1)>>8)-128);
	}

	pBuffer = SPI_Buffer;

	// copy received data from SPI_Buffer to devices
	myDDS->setSPIInBuffer(pBuffer);
	pBuffer += myDDS->getSPIBufferLen();

	myAI->setSPIInBuffer(pBuffer);
	pBuffer += myAI->getSPIBufferLen();

	myMove->setSPIInBuffer(pBuffer);
	pBuffer += myMove->getSPIBufferLen();

	if (sendIndicator) {
		sendActData();
	}
}

/***************************************************************
 * InitSPI
 * 	fixed loop configuration mandatory
 * 		all devices will be instantiated here
 * 	fixed SPI pin configuration
 * 	cyclic loop with configurable cycletime
 */
void initSPI(unsigned int time) {
	loopTime = time;

	// initialize Soft SPI
	if (!myDDS) {
		myDDS = new(SPI_DDS);
	}
	if (!myAI) {
		myAI = new(SPI_AI);
	}
	if (!myMove) {
		myMove = new(SPI_Move);
	}

#ifdef SerialDebug
	pSoftSPI = null;
#else
	pSoftSPI = new SPISoft(SPI_MISO, SPI_MOSI, SPI_CLK, SPI_CS, SPI_DELAY, SPI_BYTE_DELAY);
#endif
	if(pSoftSPI)
	{
		pSoftSPI->begin();
//		Debug.println("SPI is initialized now.");

		SPIChainLen = myMove->getSPIBufferLen() + myAI->getSPIBufferLen() + myDDS->getSPIBufferLen();
//		Debug.printf("SPI Chainlen %d \r\n", SPIChainLen);

		if (SPIChainLen > sizeof(SPI_Buffer)) return;
	}
//	Debug.println("Start SPI Loop");
	procTimer.initializeMs(loopTime, loop).start();
}

//mDNS using ESP8266 SDK functions
void startmDNS() {
    struct mdns_info *info = (struct mdns_info *)os_zalloc(sizeof(struct mdns_info));
    info->host_name = (char *) "astro"; // You can replace test with your own host name
    info->ipAddr = WifiStation.getIP();
    info->server_name = (char *) "http";
    info->server_port = 80;
    info->txt_data[0] = (char *) "path=/";
    espconn_mdns_init(info);
}

void startAstro() {
	startWebServer();
	telnet.listen(23);
	enableDebug.initCommand();
	initSPI(20);

//	myFtp.listen(21);
//	myFtp.addUser("me", "123"); // FTP account
}

/***************************************************************
 * Connection established
 * 	start services
 */
void connectOk()
{
	ntp = new ntpClient();
	startmDNS();
}

/***************************************************************
 * Init
 * 	no Serial port usable because of 4 pins of SPI
 *	Debug through Telnet
 *	WIFI Station enabled
 *		TODO: will use WPS, has to be tested with hotspot
 *
 */
void init()
{
	spiffs_mount(); // Mount file system, in order to work with files

#ifdef SerialDebug
	Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
	Serial.systemDebugOutput(true); //Allow debug output to serial
#else
	Serial.systemDebugOutput(false); //Allow debug output to serial

	// disable UART0 -> used as GPIO for SPI
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_GPIO3);
	pinMode(SPI_MISO, INPUT_PULLUP);

	PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_GPIO1);
	pinMode(SPI_MOSI, INPUT_PULLUP);
#endif

	// start debug, will be used through telnet
	Debug.start();

//	WifiStation.config(WIFI_SSID2, WIFI_PWD2);
	WifiStation.enable(true);
	WifiStation.setHostname("astro");
	WifiStation.waitConnection(connectOk);

	WifiAccessPoint.config("astro","nomie",AUTH_OPEN,false);
	WifiAccessPoint.enable(true);

	commandHandler.registerSystemCommands();

	// Run our method when station was connected to AP
	startAstro();

}
