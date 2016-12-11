/*
 * SPIAI.cpp
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#include "SPIAI.h"
#include "string.h"

SPI_AI::SPI_AI() {
	bytes = new(unsigned char[AISPIBufLen]);
	filter = 10;
	Debug.println((long)bytes);
}

SPI_AI::~SPI_AI() {
	delete (bytes);
}

void SPI_AI::setSPIInBuffer(unsigned char *newData) {
	long temp=0;
	for (char i=0; i<4; i++) {
		temp <<= 8;
		temp += *newData;
		newData++;
	}
	AI[0] = temp & 0x0000ffff;
	AI[1] = (temp & 0xffff0000) >> 16;
};

void SPI_AI::calcSPIOutBuffer() {
	memset(bytes,0x00,sizeof(bytes));
	bytes[0] = filter;
	bytes[1] = filter;
	bytes[2] = filter;
	bytes[3] = filter;
}
