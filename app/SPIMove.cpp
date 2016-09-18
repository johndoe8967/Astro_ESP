/*
 * SPIMove.cpp
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#include "SPIMove.h"

SPI_Move::SPI_Move() {
	bytes = new(unsigned char[MOVESPIBufLen]);
	Debug.println((long)bytes);
	motor_pwm[0] = 0x00;
	motor_pwm[1] = 0x00;
	LEDs		 = 0x00;
	offset[0]	 = 0x00;
	offset[1]	 = 0x00;
}

SPI_Move::~SPI_Move() {
	delete(bytes);
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
	if (posControlLoopEnabled) {
		calcControlLoop(0);
		calcControlLoop(1);
	}
	bytes[2] = motor_pwm[0];
	bytes[1] = motor_pwm[1];
	bytes[0] = LEDs;
}

void SPI_Move::calcControlLoop(unsigned char ch) {
	long error = targetPos[ch] - increments[ch];
	float control =  (float)error * P[ch];
	if (control > 127) control = 127;
	if (control <-127) control = -127;
	motor_pwm[ch] = control;
}
