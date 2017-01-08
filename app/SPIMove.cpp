/*
 * SPIMove.cpp
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#include "SPIMove.h"
#include <Debug.h>

SPI_Move::SPI_Move() {
	bytes = new (unsigned char[MOVESPIBufLen]);
	Debug.println((long) bytes);
	motor_pwm[DECLINATION] = 0x00;
	motor_pwm[REKTASZENSION] = 0x00;
	increments[DECLINATION] = 0;
	increments[REKTASZENSION] = 0;
	targetPos[DECLINATION] = 0;
	targetPos[REKTASZENSION] = 0;
	targetPosReached[DECLINATION] = false;
	targetPosReached[REKTASZENSION] = false;
	targetPosLimit[DECLINATION] = 10;
	targetPosLimit[REKTASZENSION] = 10;
	LEDs = 0x00;
	offset[DECLINATION] = 0x00;
	offset[REKTASZENSION] = 0x00;
	rate[REKTASZENSION] = MAXREKTASZENSIONSPEED;
	rate[DECLINATION] = MAXDECLINATIONSPEED;
	minVel[REKTASZENSION] = MINREKTASZENSIONSPEED;
	minVel[DECLINATION] = MINDECLINATIONSPEED;
	accel[REKTASZENSION] = ACCREKTASZENSION;
	accel[DECLINATION] = ACCDEKLINATION;
	moveTimeoutState = stopped;
	moveTimeoutCounter = 0;
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
	inctempold = increments[REKTASZENSION] & 0x0fff;

	if ((int)(inctemp - inctempold) < -2048) {
		Debug.println("ueberlauf+");
		increments[REKTASZENSION] += 0x1000;
	}

	if ((int)(inctemp - inctempold) > 2048) {
		Debug.println("ueberlauf-");
		increments[REKTASZENSION] -= 0x1000;
	}

	increments[REKTASZENSION] &= 0xfffff000;
	increments[REKTASZENSION] |= inctemp;
	velocity[REKTASZENSION] = calcVelocity(increments[REKTASZENSION]);

	inctemp =  (temp & 0xfff000) >> 12;
	inctempold = increments[DECLINATION] & 0x0fff;

	if ((int)(inctemp - inctempold) < -2048) {
		Debug.println("ueberlauf+");
		increments[DECLINATION] += 0x1000;
	}

	if ((int)(inctemp - inctempold) > 2048) {
		Debug.println("ueberlauf-");
		increments[DECLINATION] -= 0x1000;
	}

	increments[DECLINATION] &= 0xfffff000;
	increments[DECLINATION] |= inctemp;
};
#ifdef debug
void SPI_Move::debugError(float error) {
	static unsigned char simcounter=0;
	simcounter++;
	simcounter %=50;
	if (simcounter==0) {
		char value_msg[10];
		Debug.print("Err: ");
		dtostrf_p(error,6,2,value_msg,'0');
		Debug.print(value_msg);
	}
}
void SPI_Move::debug(unsigned char ch) {
	static unsigned char simcounter[2]={0,0};
	simcounter[ch]++;
	simcounter[ch] %=50;
	if (simcounter[ch]==0) {
		if (ch == DECLINATION) {
			Debug.print("Dec:");
		} else {
			Debug.print("Rek:");
		}
		char value_msg[10];
		Debug.print("Tar: ");
		Debug.print(targetPos[ch]);

		Debug.print("pos: ");
		Debug.print(getPos(ch));

		Debug.print(" cyc: ");
		dtostrf_p(cyclicPos[ch],6,2,value_msg,'0');
		Debug.print(value_msg);

		Debug.print(" offs:");
		Debug.println(offset[ch]);
	}
}
#endif

void SPI_Move::calcSPIOutBuffer() {
#ifdef debug
	debug(0);
	debug(1);
#endif

	if (posControlLoopEnabled) {
		calcCyclicPos(0);
		calcControlLoop(0);
		calcCyclicPos(1);
		calcControlLoop(1);
	} else if (simulate) {
		static unsigned char simcounter=0;
		simcounter++;
		simcounter %=50;
		if (simcounter==0) {
			offset[REKTASZENSION]++;
		}
	}

	if (!isMovingWhenPowered(REKTASZENSION)) {
		motor_pwm[REKTASZENSION] = 0;
	}

	if (motor_pwm[DECLINATION] <= 0x80) {
		bytes[2] = 0x80 - motor_pwm[DECLINATION];
	} else {
		bytes[2] = motor_pwm[DECLINATION];
	}

	if (motor_pwm[REKTASZENSION] <= 0x80) {
		bytes[1] = 0x80 - motor_pwm[REKTASZENSION];
	} else {
		bytes[1] = motor_pwm[REKTASZENSION];
	}
	bytes[0] = LEDs;
}

void SPI_Move::checkPosReached(unsigned char ch) {
	float error = targetPos[ch] - this->getPos(ch);
	targetPosReached[ch] = (abs(error) < targetPosLimit[ch]);
}

// if error is more than half 360° then move other direction
//             error out
//           /    |   /
//          /     |  /
//         /      | /
//        /       |/   +modulo
//-------/--------/--------/------- error in
//   -modulo     /|       /
//              / |      /
//             /  |     /
//            /   |    /
//                |
float SPI_Move::calcModuloError(unsigned char ch, float error) {
	float lowerlimit = modulo[ch];
	lowerlimit *= 1.5;
	if (error < -lowerlimit) error = -lowerlimit;
	return fmod((error + lowerlimit), modulo[ch]) - modulo[ch] / 2;
}

//             cyclicPos out
//              / |       /         /
//            /   |     /         /
//          /     |   /         /
//        /       | /         /
//------/---------/---------/------- cyclicPos in
//                |
//                |
float SPI_Move::calcModuloPos(unsigned char ch, float pos) {
	float lowerlimit = (float) modulo[ch];
	if (pos < -lowerlimit) pos = -lowerlimit;
	return fmod(pos + lowerlimit,modulo[ch]);
}

void SPI_Move::calcControlLoop(unsigned char ch) {
	checkPosReached(ch);
	float error = cyclicPos[ch] - this->getPos(ch);
	error = calcModuloError(ch, error);


	float control =  error * P[ch];
 	if (control >127) control = 127;
	if (control <-127) control = -127;

	if (abs(error) > 2) {
		controlI[ch] = controlI[ch] + error * I[ch];
	} //else {
//		controlI[ch] = 0;
//	}

	if (controlI[ch] + control > 127) {
		controlI[ch] = 127 - control;
	}

	if (controlI[ch] + control < -127) {
		controlI[ch] = -127 + control;
	}
	control += controlI[ch];

	if (ch == 0) {
	 	if (control >127) control = 127;
		if (control <-127) control = -127;

	} else {
		if (control >127) control = 127;
		if (control <-127) control = -127;
	}
	motor_pwm[ch] = control;

	if (simulate) {
		offset[ch] += error;
		offset[ch] = calcModuloPos(ch,offset[ch]);
	}
}

void SPI_Move::calcCyclicPos(unsigned char ch) {

	float error = targetPos[ch] - cyclicPos[ch];
	error = calcModuloError(ch, error);

	char dir = signbit(error);
	error = abs(error);

	float pbrake = rate[ch]*rate[ch]/(2*accel[ch]);
	float ramp = min(1, error / pbrake);
	float maxVel = rate[ch]*ramp*SAMPLETIME;

	if (maxVel < minVel[ch]) {
		maxVel = minVel[ch];
	}
	if (error > maxVel) {
		error = maxVel;
	}
	if (dir == 1) { error *= -1;}
	cyclicPos[ch] += error;
	cyclicPos[ch] = calcModuloPos(ch, cyclicPos[ch]);
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

void SPI_Move::resetDelay() {
	moveTimeoutCounter = 0;
}

bool SPI_Move::delayedTransition(unsigned char delay) {
	if (moveTimeoutCounter == delay) {
		resetDelay();
		return true;
	} else {
		moveTimeoutCounter++;
		return false;
	}
}
// check is movement is detected within defined time when pwm is greater than threshold
// if detected pwm has to become 0 for the same defined time to reset the state
bool SPI_Move::isMovingWhenPowered(unsigned char ch) {
	unsigned char abs_pwm = abs(motor_pwm[ch]);
	switch (moveTimeoutState) {
	case stopped:
		if (abs_pwm >= motor_pwm_threshold) {
			resetDelay();
			moveTimeoutState = powered;
		}
		break;
	case powered:
		if (abs_pwm < motor_pwm_threshold) {
			if (delayedTransition(25)) {
				moveTimeoutState = stopped;
			}
		} else {
			float minSpeedAtPWM = (float)abs_pwm/256.0 * MAXREKTASZENSIONSPEED;
			if (abs(velocity[ch]) < minSpeedAtPWM) {
				if (delayedTransition(25)) {
					moveTimeoutState = blocked;
				}
			} else {
				resetDelay();
			}
		}
		break;
	case blocked:
		if (abs_pwm < motor_pwm_threshold) {
			if (delayedTransition(25)) {
				moveTimeoutState = stopped;
			}
		}
		break;
	}
	return (moveTimeoutState == blocked);
}
