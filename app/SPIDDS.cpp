/*
 * SPIDDS.cpp
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#include "SPIDDS.h"

SPI_DDS::SPI_DDS() {
	bytes = new(unsigned char[DDSSPIBufLen]);
	Debug.println((long)bytes);
	DDSvalue=0x00000000;
	LEDs	=0x00;
	magnet	=0x00;
}

SPI_DDS::~SPI_DDS() {
	delete (bytes);
}


void SPI_DDS::setSPIInBuffer(unsigned char *newData) {
	DI_IN = *(newData+3);
};

void SPI_DDS::calcSPIOutBuffer() {
	unsigned long temp = DDSvalue;
	for (char i=3;i>0;i--) {
		bytes[i] = temp & 0xff;
		temp >>= 8;
	}
	bytes[0] = temp & 0x07;
	bytes[0] |= (LEDs << 3);
	bytes[0] |= (magnet << 7);
}
