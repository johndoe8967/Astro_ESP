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

class SPI_Move: public SPIDevice {
public:
	SPI_Move();
	virtual ~SPI_Move();

	void setSPIInBuffer(unsigned char *newData);
	inline size_t getSPIBufferLen() {return MOVESPIBufLen;};
	unsigned char* getSPIBuffer() {calcSPIOutBuffer(); return bytes;};


	long getPos(unsigned char ch) { if (ch<2) { return increments[ch]+offset[ch]; } else return 0;};
	void setPWM(unsigned char ch, char pwm) { if (ch<2) { motor_pwm[ch] = pwm;}};
	void setLED(unsigned char ch) { if(ch<4) { LEDs |= 1<<ch;}};
	void clrLED(unsigned char ch) { if(ch<4) { LEDs &= ~(1<<ch);}};
	void posControlEnable(bool en) { posControlLoopEnabled=en;}
	void setPosition(unsigned char ch, long pos) { targetPos[ch] = pos;};
	void setPControl(unsigned char ch, float setP) { if (setP>0) {P[ch]=setP;}};
	void setReference(unsigned char ch, int ref) {offset[ch] = ref;};


private:
	void calcSPIOutBuffer();
	void calcControlLoop(unsigned char ch);
	bool posControlLoopEnabled=false;
	float P[NUM_CHANNELS]={1,1};

	unsigned char *bytes;
	long targetPos[NUM_CHANNELS];
	int increments[NUM_CHANNELS];
	int offset[NUM_CHANNELS];

	char motor_pwm[NUM_CHANNELS];
	char LEDs;
};

#endif /* APP_SPIMOVE_H_ */
