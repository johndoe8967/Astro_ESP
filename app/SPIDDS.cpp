/*
 * SPIDDS.cpp
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#include "SPIDDS.h"

SPI_DDS::SPI_DDS() {
	SPIDevice(SpiData.bytes);
}

SPI_DDS::~SPI_DDS() {
	// TODO Auto-generated destructor stub
}


void SPI_DDS::setSPIInBuffer(unsigned char *newData) {
	DI_IN = *(newData+3);
};

void SPI_DDS::calcSPIOutBuffer() {
	SpiData.value = DDSvalue;
	SpiData.bytes[3] |= (LEDs << 3);
	SpiData.bytes[3] |= (magnet << 7);
}
