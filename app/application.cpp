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

Timer procTimer;				//* cyclic Timer processing SPI loop and devices
#define sendDataTime 500
unsigned int reduction;			//* reduction cyclic procTimer to send actual data to websocket clients
unsigned int reductionCounter;	//* modulo counter for reduction of cyclic procTimer


//* Devices on the SPI loop will be initialized together with the SPI bus
SPISoft *pSoftSPI = NULL;		//* SPI bus instance
SPI_DDS  *myDDS = null;			//* DDS tracking frequency output device incl. magnet
SPI_Move *myMove = null;		//* both incremental drives for slewing the telescope
SPI_AI   *myAI = null;			//* analog inputs for joystick movement
unsigned char SPIChainLen;		//* length of SPI chain is used to transfer the buffer
unsigned char SPI_Buffer[11] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
unsigned char *pBuffer, *pSource;	//*

// debug output instead of UART
TelnetServer telnet;
EnableDebug enableDebug;	//* custom telnet commands: enable debug, show IP, (start SPI)

//FTPServer myFtp;

ntpClient *ntp;				//* ntp client used only for time display, not necessary for position calculation (client needs exact time)

unsigned char usePoti;		//* indicator from webclient to use AI (equal to pressing both joystick button
MODES mode = move;			//* main operation mode
MODES modePrev = move;		//* backup to check if a mode change happened to send new mode to all clients
MODES oldMode;				//* backup of last operation mode to return to after manual mode


#ifdef debugSPI
unsigned char enableBytesOut=0;
unsigned char bytes[11];	// bytes received through websocket
#endif

//* mode changes need some delay to set magnet and stop drive movement
#define magOnDelay 100
#define magOffDelay 50
#define posDelay 25
unsigned char delayModeChangeCount; 	//* delay for mode changes by number of loop cycles
//restart delay
inline void resetDelay() {
	delayModeChangeCount = 0;
}
// return true if delay elapsed defined time
bool delayedTransition(unsigned char delay) {
	if (delayModeChangeCount++ == delay) {
		resetDelay();
		return true;
	} else {
		return false;
	}
}

// check if manual movement is requested
// returns given mode until manual is requested and stores given mode for later restore
MODES checkManualMove(MODES actmode) {
	if (usePoti || myDDS->getDI(0) || myDDS->getDI(1)) {
		oldMode = actmode;
		actmode = potiMag;
		resetDelay();
	}
	return actmode;
}

void calcManualMove(unsigned char ch) {
	if (usePoti || myDDS->getDI(ch)) {
		myMove->setPWM(ch, (myAI->getAI(ch)>>8)-128);
		resetDelay();
	} else {
		myMove->setPWM(ch,0);
	}
}


// interface to set mode from webserver
// no direct write access to mode is allowed
// only basic modes are allowed
void setMode(MODES newMode) {
	if ((newMode == move) || (newMode == sync) || (newMode==track) || (newMode==slew)) {
		mode = newMode;
		resetDelay();				// prevent running delay counter from premature finish
	}
}

void calcModeStatemachine () {
	switch (mode) {
	case slew:							// prepare slew from ASCOM returns to track automatically
	case move:							// prepare manual goto from web GUI stays in goto
		myDDS->setMagnet();
		if (delayedTransition(magOnDelay)) {
			myMove->posControlEnable(1);
			oldMode = mode;
			mode = moving;
		}
		break;

	case moving:
		if (oldMode != move) {			// if old mode is slew check is positioning is finished to switch back to tracking
			if (myMove->setPositionReached(0) && myMove->setPositionReached(1)) {
				if (delayedTransition(posDelay)) {
					mode = track;
				}
			} else {
				resetDelay();
			}
		}
		break;

	case sync:							// prepare sync by stopping movement and wait for stopped drives
		myMove->posControlEnable(0);
		myMove->setPWM(0,0);
		myMove->setPWM(1,0);
		if (delayedTransition(magOffDelay)) {
			myDDS->clrMagnet();
			mode = syncing;
		}
		break;
	case syncing:						// set actual position without movement to target position (reference)
		myMove->setReference(0);
		myMove->setReference(1);

		mode = checkManualMove(mode);	// check for manual mode changes
		break;

	case track:							// prepare tracking by stopping movement and wait for stopped drives
		myMove->posControlEnable(0);
		myMove->setPWM(0,0);
		myMove->setPWM(1,0);

		if (delayedTransition(magOffDelay)) {
			myDDS->clrMagnet();
			mode = tracking;
		}
		break;
	case tracking:						// nothing to do in star tracking mode
		mode = checkManualMove(mode);	// check for manual mode changes
		break;

	case potiMag:						// prepare moving with poti
		myDDS->setMagnet();
		if (delayedTransition(magOnDelay)) {
			mode = potiMove;
		}
		break;
	case potiMove:						// moving with poti
		calcManualMove(0);
		calcManualMove(1);

		if (!usePoti && !myDDS->getDI(1) && !myDDS->getDI(1)) {	// stop moving with poti if no request pending
			if (delayedTransition(magOffDelay)) {
				mode = (MODES)(oldMode%10);
			}
		}
		break;
	default:
		break;
	}

	if (mode != modePrev) {				// detect mode change
		char modeString[2];
		modeString[0] = '0' + mode%10;	// calculate basic mode as string
		modeString[1] = 0;
		sendMessage("mode", modeString);// send new mode to clients
		modePrev = mode;				// store actual mode for next loop
	}
}

// cyclic loop
/***************************************************************
 * cyclic loop / main task
 * 	debug mode without SPI device classes (only byte array IO)
 *
 */
void loop() {
	calcModeStatemachine();

#ifdef debugSPI
	if (enableBytesOut) {
		// copy debug data to SPI buffer
		memcpy(SPI_Buffer, bytes, sizeof(SPI_Buffer));
	}
	else
#endif

	{
		unsigned char *pBuffer = SPI_Buffer;				// working pointer to traverse through SPI Buffer
		unsigned char *pSource;								// working pointer from the device output Data

		// copy out data from devices to SPI buffer
	 	pSource = myDDS->getSPIBuffer();					// calculate DDS output data
		memcpy(pBuffer, pSource, myDDS->getSPIBufferLen());	// copy DDS output data to SPI Output Buffer
		pBuffer += myDDS->getSPIBufferLen();				// set working pointer to next SPI device start address

		pSource = myAI->getSPIBuffer();						// calculate AI output data
		memcpy(pBuffer, pSource, myAI->getSPIBufferLen());	// copy DDS output data to SPI Output Buffer¥
		pBuffer += myAI->getSPIBufferLen();					// set working pointer to next SPI device start address

		pSource = myMove->getSPIBuffer();					// calculate drive output data
	 	memcpy(pBuffer, pSource, myMove->getSPIBufferLen());// copy DDS output data to SPI Output Buffer¥
	}


	bool sendIndicator = false;								// local indicator to send SPU out, actual values and SPI in data

	reductionCounter++;										// calculate modulo reduction counter
	reductionCounter %= reduction;
	sendIndicator = (reductionCounter==0);					// indicate send data to clients on overflow

	if (debugOpen && sendIndicator) {						// send SPI Out data if at least one client opened the debug section
		sendSPIData(false, SPI_Buffer);
	}

	// transmit SPI_Buffer if serial is not used for debugging
#ifndef SerialDebug
	pSoftSPI->beginTransaction(pSoftSPI->SPIDefaultSettings);
	pSoftSPI->transfer(SPI_Buffer,SPIChainLen);
	pSoftSPI->endTransaction();
#endif

	if (debugOpen && sendIndicator) {						// send SPI IN data if at least one client opened the debug section
		sendSPIData(true, SPI_Buffer);
	}

	{
		pBuffer = SPI_Buffer;								// working pointer to traverse through SPI Buffer

		// copy received data from SPI_Buffer to devices
		myDDS->setSPIInBuffer(pBuffer);						// use SPI In data to calculate new DDS state
		pBuffer += myDDS->getSPIBufferLen();				// set working pointer to next SPI device start address

		myAI->setSPIInBuffer(pBuffer);						// use SPI In data to calculate new AI state
		pBuffer += myAI->getSPIBufferLen();					// set working pointer to next SPI device start address

		myMove->setSPIInBuffer(pBuffer);					// use SPI In data to calculate new drive state
	}
	if (sendIndicator) {									// send actual data to clients
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
	reduction = sendDataTime / time;					// reduction of html and debug interface to 1s cycle time

	if (!myDDS) 	myDDS = new(SPI_DDS);				// initialize DDS device
	if (!myAI) 		myAI = new(SPI_AI);					// initialize AI device
	if (!myMove) 	myMove = new(SPI_Move);				// initialize drives

#ifdef SerialDebug
	pSoftSPI = null;							// no SPI bus if pins are used for serial debugging
#else
	// initialize Soft SPI
	pSoftSPI = new SPISoft(SPI_MISO, SPI_MOSI, SPI_CLK, SPI_CS, SPI_DELAY, SPI_BYTE_DELAY);
#endif

	if (pSoftSPI) {								// check if SPI bus is initialized successful
		pSoftSPI->begin();						// start SPI bus

		// calculate SPI bus chain length and check is buffer is big enough
		SPIChainLen = myMove->getSPIBufferLen() + myAI->getSPIBufferLen() + myDDS->getSPIBufferLen();
		if (SPIChainLen > sizeof(SPI_Buffer)) return;	//TODO: should be initialized dynamically
	}
	procTimer.initializeMs(time, loop).start();	// start cyclic calculation timer
}

#ifdef mDNS
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
#endif

void startAstro() {
	startWebServer();
	telnet.listen(23);
	enableDebug.initCommand();
	initSPI(10);

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
#ifdef mDNS
	startmDNS();
#endif
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
	System.setCpuFrequency(eCF_160MHz);

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
