/*
 * SPIMove.h
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#ifndef APP_SPIMOVE_H_
#define APP_SPIMOVE_H_

#include "SPIDevice.h"

#define MOVESPIBufLen 3
#define NUM_CHANNELS 2

#define REKTASZENSION 0
#define DECLINATION 1

#define MAXREKTASZENSIONSPEED 235.3

#define SAMPLETIME 0.020

class SPI_Move: public SPIDevice {
public:
	SPI_Move();
	virtual ~SPI_Move();

	void setSPIInBuffer(unsigned char *newData);
	inline size_t getSPIBufferLen() {return MOVESPIBufLen;};
	unsigned char* getSPIBuffer() {calcSPIOutBuffer(); return bytes;};


	long getPos(unsigned char ch) { if (ch<NUM_CHANNELS) { return increments[ch]+offset[ch]; } else return 0;};
	float getVel(unsigned char ch) { if (ch<NUM_CHANNELS) { return velocity[ch]; } else return 0;};
	void setPWM(unsigned char ch, char pwm) { if (ch<NUM_CHANNELS) { motor_pwm[ch] = pwm;}};
	void setLED(unsigned char ch) { if(ch<4) { LEDs |= 1<<ch;}};
	void clrLED(unsigned char ch) { if(ch<4) { LEDs &= ~(1<<ch);}};
	void posControlEnable(bool en) { posControlLoopEnabled=en;}
	void setPosition(unsigned char ch, long pos) { if (ch<NUM_CHANNELS) {targetPos[ch] = pos;}};
	bool setPositionReached(unsigned char ch) { if (ch<NUM_CHANNELS) {return targetPosReached[ch];} else return false;};
	void setPositionLimit(unsigned char ch, long limit) { if (ch<NUM_CHANNELS) {targetPosLimit[ch] = limit;}};
	void setPControl(unsigned char ch, float setP) { if ((ch<NUM_CHANNELS)&&(setP>0)) {P[ch]=setP;}};
	void setReference(unsigned char ch) {if(ch<NUM_CHANNELS) { offset[ch] = targetPos[ch]-increments[ch]; }};


private:
	void calcSPIOutBuffer();
	void calcControlLoop(unsigned char ch);
	float calcVelocity(long actPos);
	bool isMovingWhenPowered(unsigned char ch);
	void resetDelay();
	bool delayedTransition(unsigned char delay);

	bool posControlLoopEnabled=false;
	float P[NUM_CHANNELS]={1,2};

	unsigned char *bytes;

	long targetPos[NUM_CHANNELS];
	long targetPosLimit[NUM_CHANNELS];
	bool targetPosReached[NUM_CHANNELS];

	enum MOVETIMEOUTSTATE {stopped=0, powered, blocked} moveTimeoutState;
	unsigned char moveTimeoutCounter=25;
	char motor_pwm_threshold;

	long increments[NUM_CHANNELS];
	float velocity[NUM_CHANNELS];
	long offset[NUM_CHANNELS];

	char motor_pwm[NUM_CHANNELS];
	char LEDs;
};

#endif /* APP_SPIMOVE_H_ */
