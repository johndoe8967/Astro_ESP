/*
 * SPIMove.cpp
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#include "SPIMove.h"

SPI_Move::SPI_Move() {
	SPIDevice(bytes);
}

SPI_Move::~SPI_Move() {
	// TODO Auto-generated destructor stub
}

void SPI_Move::setSPIInBuffer(unsigned char *newData) {
	long temp=0;
	for (char i=0; i<3; i++) {
		temp <<= 8;
		temp += *newData;
		newData++;
	}
	increments[0] = temp & 0x000fff;
	increments[1] = (temp & 0xfff000) >> 12;
};

void SPI_Move::calcSPIOutBuffer() {
	bytes[0] = motor_pwm[0];
	bytes[1] = motor_pwm[1];
	bytes[2] = LEDs;
}
