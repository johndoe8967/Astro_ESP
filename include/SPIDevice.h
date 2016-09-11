/*
 * SPIDevice.h
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#ifndef APP_SPIDEVICE_H_
#define APP_SPIDEVICE_H_
class SPIDevice {
public:
	SPIDevice();
	SPIDevice(unsigned char* pBuffer);
	virtual ~SPIDevice();
	unsigned char* getSPIOutBuffer();
	virtual void setSPIInBuffer(unsigned char *newData){};
	virtual unsigned char getSPIBufferLen() {};
private:
	virtual void calcSPIOutBuffer() {};
	unsigned char* pSPIData;
};

#endif /* APP_SPIDEVICE_H_ */
