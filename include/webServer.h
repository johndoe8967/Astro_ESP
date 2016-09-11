/*
 * webServer.h
 *
 *  Created on: 10.09.2016
 *      Author: johndoe
 */

#ifndef INCLUDE_WEBSERVER_H_
#define INCLUDE_WEBSERVER_H_

extern unsigned char bytes[16];

void startWebServer();
void sendSPIData(unsigned char bytes[16]);



#endif /* INCLUDE_WEBSERVER_H_ */
