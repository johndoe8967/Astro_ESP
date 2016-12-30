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
	rate[REKTASZENSION]	= MAXREKTASZENSIONSPEED;
	rate[DECLINATION]	= MAXDECLINATIONSPEED;
	minVel[REKTASZENSION] 	= MINREKTASZENSIONSPEED;
	minVel[DECLINATION] 	= MINDECLINATIONSPEED;
	accel[REKTASZENSION] = rate[REKTASZENSION];
	accel[DECLINATION] = rate[DECLINATION];
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
	velocity[1] = calcVelocity(increments[1]);

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
	velocity[0] = calcVelocity(increments[0]);

};

void SPI_Move::calcSPIOutBuffer() {
	if (posControlLoopEnabled) {
		calcCyclicPos(0);
		calcControlLoop(0);
		calcCyclicPos(1);
		calcControlLoop(1);
	}

	if (!isMovingWhenPowered(REKTASZENSION)) {
		motor_pwm[REKTASZENSION] = 0;
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
	long error = cyclicPos[ch] - this->getPos(ch);
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

void SPI_Move::calcCyclicPos(unsigned char ch) {
	long error = targetPos[ch] - cyclicPos[ch];
	char dir = signbit(error);
	error = abs(error);

	float pbrake = rate[ch]*rate[ch]/(2*accel[ch]);
	float ramp = min(1, error / pbrake);
	error = min(error,rate[ch]*ramp*SAMPLETIME);
	error = max(error,minVel[ch]*SAMPLETIME);

	if (dir == 1) { error *= -1;}
	cyclicPos[ch] += error;
}

inline void SPI_Move::resetDelay() {
	moveTimeoutCounter = 0;
}
bool SPI_Move::delayedTransition(unsigned char delay) {
	if (moveTimeoutCounter++ == delay) {
		resetDelay();
		return true;
	} else {
		return false;
	}
}

float SPI_Move::calcVelocity(long actPos) {
#define OLDPOSLEN 10
static long oldPos[OLDPOSLEN];
static long oldestPosIndex;

	float vel = (actPos - oldPos[oldestPosIndex]) / (OLDPOSLEN * SAMPLETIME);

	oldPos[oldestPosIndex] = actPos;
	oldestPosIndex++;
	oldestPosIndex %= OLDPOSLEN;

	return vel;
}

// check is movement is detected within defined time when pwm is greater than threshold
// if detected pwm has to become 0 for the same defined time to reset the state
bool SPI_Move::isMovingWhenPowered(unsigned char ch) {
	switch (moveTimeoutState) {
	case stopped:
		if (abs(motor_pwm[ch]) >= motor_pwm_threshold) {
			resetDelay();
			moveTimeoutState = powered;
		}
		break;
	case powered:
		if (abs(motor_pwm[ch]) < motor_pwm_threshold) {
			if (delayedTransition(25)) {
				moveTimeoutState = stopped;
			}
		} else {
			float minSpeedAtPWM = (float)motor_pwm[ch]/128.0 * MAXREKTASZENSIONSPEED / 2;
			if (velocity[ch] < minSpeedAtPWM) {
				if (delayedTransition(25)) {
					moveTimeoutState = blocked;
				}
			} else {
				resetDelay();
			}
		}
		break;
	case blocked:
		if (abs(motor_pwm[ch]) < motor_pwm_threshold) {
			if (delayedTransition(25)) {
				moveTimeoutState = stopped;
			}
		}
		break;
	}
	return (moveTimeoutState == blocked);
}
