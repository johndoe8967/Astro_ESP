/*
 * SPIMove.cpp
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#include "SPIMove.h"
#include <Debug.h>

SPI_Move::SPI_Move() {
	bytes = new(unsigned char[MOVESPIBufLen]);
	Debug.println((long)bytes);
	motor_pwm[0] = 0x00;
	motor_pwm[1] = 0x00;
	increments[0] = 0;
	increments[1] = 0;
	targetPos[0]  = 0;
	targetPos[1]  = 0;
	targetPosReached[0] = false;
	targetPosReached[1] = false;
	targetPosLimit[0] = 10;
	targetPosLimit[1] = 10;
	LEDs		 = 0x00;
	offset[0]	 = 0x00;
	offset[1]	 = 0x00;
}

SPI_Move::~SPI_Move() {
	delete(bytes);
}

void SPI_Move::setSPIInBuffer(unsigned char *newData) {
	long temp=0;
	unsigned int inctemp;
	unsigned int inctempold;

	for (char i=0; i<3; i++) {
		temp <<= 8;
		temp += *newData;
		newData++;
	}
	inctemp =  temp & 0x000fff;
	inctempold = increments[1] & 0x0fff;

	if ((int)(inctemp - inctempold) < -2048) {
		Debug.println("ueberlauf+");
		increments[1] += 0x1000;
	}

	if ((int)(inctemp - inctempold) > 2048) {
		Debug.println("ueberlauf-");
		increments[1] -= 0x1000;
	}

	increments[1] &= 0xfffff000;
	increments[1] |= inctemp;


	inctemp =  (temp & 0xfff000) >> 12;
	inctempold = increments[0] & 0x0fff;

	if ((int)(inctemp - inctempold) < -2048) {
		Debug.println("ueberlauf+");
		increments[0] += 0x1000;
	}

	if ((int)(inctemp - inctempold) > 2048) {
		Debug.println("ueberlauf-");
		increments[0] -= 0x1000;
	}

	increments[0] &= 0xfffff000;
	increments[0] |= inctemp;
};

void SPI_Move::calcSPIOutBuffer() {
	if (posControlLoopEnabled) {
		calcControlLoop(0);
		calcControlLoop(1);
	}
	if (motor_pwm[0] <= 0x80) {
		bytes[2] = 0x80 - motor_pwm[0];
	} else {
		bytes[2] = motor_pwm[0];
	}

	if (motor_pwm[1] <= 0x80) {
		bytes[1] = 0x80 - motor_pwm[1];
	} else {
		bytes[1] = motor_pwm[1];
	}
	bytes[0] = LEDs;
}

void SPI_Move::calcControlLoop(unsigned char ch) {
	long error = targetPos[ch] - this->getPos(ch);
	targetPosReached[ch] = (abs(error) < targetPosLimit[ch]);
	float control =  (float)error * P[ch];
	if (ch == 0) {
	 	if (control >127) control = 127;
		if (control <-127) control = -127;

	} else {
		if (control >127) control = 127;
		if (control <-127) control = -127;
	}
	motor_pwm[ch] = control;
}
