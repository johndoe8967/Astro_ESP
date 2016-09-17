/*
 * SPIDevice.h
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#ifndef APP_SPIDEVICE_H_
#define APP_SPIDEVICE_H_

#include <user_config.h>
#include <debug.h>


class SPIDevice {
public:
	SPIDevice();
	virtual ~SPIDevice();
	virtual void setSPIInBuffer(unsigned char *newData){};
	virtual size_t getSPIBufferLen() {};
private:
	virtual void calcSPIOutBuffer() {};
};

#endif /* APP_SPIDEVICE_H_ */
