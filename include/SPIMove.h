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

#define DECLINATION 0
#define REKTASZENSION 1

#define MAXREKTASZENSIONSPEED 235.3
#define MAXDECLINATIONSPEED 1700
#define MINREKTASZENSIONSPEED 0.1
#define MINDECLINATIONSPEED 0.1
#define ACCREKTASZENSION 700
#define ACCDEKLINATION 5000
#define MAXREKTASZENSION 48000
#define MAXDEKLINATION 16750

#define SAMPLETIME 0.020

class SPI_Move: public SPIDevice {
public:
	SPI_Move();
	virtual ~SPI_Move();

	void setSPIInBuffer(unsigned char *newData);
	inline size_t getSPIBufferLen() {return MOVESPIBufLen;};
	unsigned char* getSPIBuffer() {calcSPIOutBuffer(); return bytes;};
#ifdef debug
	void debug(unsigned char ch);
	void debugError(float error);
#endif
	long getPos(unsigned char ch) { if (ch<NUM_CHANNELS) { return calcModuloPos(ch,increments[ch]+offset[ch]); } else return 0;};
	float getVel(unsigned char ch) { if (ch<NUM_CHANNELS) { return velocity[ch]; } else return 0;};
	void setPWM(unsigned char ch, char pwm) { if (ch<NUM_CHANNELS) { motor_pwm[ch] = pwm;}};
	void setLED(unsigned char ch) { if(ch<4) { LEDs |= 1<<ch;}};
	void clrLED(unsigned char ch) { if(ch<4) { LEDs &= ~(1<<ch);}};
	void posControlEnable(bool en) { posControlLoopEnabled=en;}
	void setPosition(unsigned char ch, long pos) { if (ch<NUM_CHANNELS) {targetPos[ch] = calcModuloPos(ch,pos);cyclicPos[ch] = this->getPos(ch);}};
	bool setPositionReached(unsigned char ch) { if (ch<NUM_CHANNELS) {return targetPosReached[ch];} else return false;};
	void setPositionLimit(unsigned char ch, long limit) { if (ch<NUM_CHANNELS) {targetPosLimit[ch] = limit;}};
	void setPControl(unsigned char ch, float setP) { if ((ch<NUM_CHANNELS)&&(setP>=0)) {P[ch]=setP;}};
	void setIControl(unsigned char ch, float setI) { if ((ch<NUM_CHANNELS)&&(setI>=0)) {I[ch]=setI;}};
	void setReference(unsigned char ch) {if(ch<NUM_CHANNELS) { offset[ch] = targetPos[ch]-increments[ch]; }};
	void setRate(unsigned char ch, float setRate) {if(ch<NUM_CHANNELS) { if (setRate > 0) {rate[ch] = setRate;}}};
	float getRate(unsigned char ch) {if(ch<NUM_CHANNELS) {return rate[ch];} else {return 0;}};
	void setAccel(unsigned char ch, float setAccel)  {if(ch<NUM_CHANNELS) { if (setAccel > 0) {accel[ch] = setAccel;}}};
	float getAccel(unsigned char ch) {if(ch<NUM_CHANNELS) {return accel[ch];} else {return 0;}};

	void setSimulate(bool sim) {simulate = sim;};


private:
	void calcSPIOutBuffer();
	void calcControlLoop(unsigned char ch);
	void checkPosReached(unsigned char ch);
	void calcCyclicPos(unsigned char ch);
	float calcModuloError(unsigned char ch, float error);
	float calcModuloPos(unsigned char ch, float pos);
	float calcVelocity(long actPos);
	bool isMovingWhenPowered(unsigned char ch);
	void resetDelay();
	bool delayedTransition(unsigned char delay);

	bool posControlLoopEnabled=false;
	float P[NUM_CHANNELS]={1.6,3};
	float I[NUM_CHANNELS]={0.03,0.02};
	float controlI[NUM_CHANNELS]={0.0,0.0};

	unsigned char *bytes;

	long targetPos[NUM_CHANNELS];
	long targetPosLimit[NUM_CHANNELS];
	bool targetPosReached[NUM_CHANNELS];
	const long modulo[NUM_CHANNELS] = {MAXDEKLINATION, MAXREKTASZENSION};
	float rate[NUM_CHANNELS];
	float accel[NUM_CHANNELS];
	float minVel[NUM_CHANNELS];
	float cyclicPos[NUM_CHANNELS];

	enum MOVETIMEOUTSTATE {stopped=0, powered, blocked} moveTimeoutState;
	unsigned char moveTimeoutCounter=25;
	const char motor_pwm_threshold=10;

	long increments[NUM_CHANNELS];
	float velocity[NUM_CHANNELS];
	long offset[NUM_CHANNELS];

	char motor_pwm[NUM_CHANNELS];
	char LEDs;

	bool simulate=false;
};

#endif /* APP_SPIMOVE_H_ */
