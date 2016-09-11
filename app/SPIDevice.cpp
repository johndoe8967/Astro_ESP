/*
 * SPIDevice.cpp
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#include "SPIDevice.h"

SPIDevice::SPIDevice() {
	// TODO Auto-generated constructor stub

}
SPIDevice::SPIDevice(unsigned char* buffer) {
	pSPIData = buffer;
}

SPIDevice::~SPIDevice() {
	// TODO Auto-generated destructor stub
}

unsigned char* SPIDevice::getSPIOutBuffer() {
	calcSPIOutBuffer();
	return pSPIData;
}

