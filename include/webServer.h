/*
 * webServer.h
 *
 *  Created on: 10.09.2016
 *      Author: johndoe
 */

#ifndef INCLUDE_WEBSERVER_H_
#define INCLUDE_WEBSERVER_H_

#include "SPIDDS.h"
#include "SPIMove.h"
#include "SPIAI.h"
#include "application.h"

extern unsigned char bytes[11];
extern SPI_DDS  *myDDS;
extern SPI_Move *myMove;
extern SPI_AI   *myAI;
extern unsigned char enableBytesOut;

extern MODES mode;
extern unsigned char usePoti;

void startWebServer();
void sendSPIData(bool in, unsigned char bytes[11]);
void sendMessage(const char *msg, const char *value);


#endif /* INCLUDE_WEBSERVER_H_ */
