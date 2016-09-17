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


	long getPos(unsigned char ch) { if (ch<2) { return increments[ch]; } else return 0;};
	void setPWM(unsigned char ch, char pwm) {if (ch<2) { motor_pwm[ch] = pwm;}};
	void setLED(unsigned char ch) {if(ch<4) { LEDs |= 1<<ch;}};
	void clrLED(unsigned char ch) {if(ch<4) { LEDs &= ~(1<<ch);}};


private:
	void calcSPIOutBuffer();

	unsigned char *bytes;
	int increments[NUM_CHANNELS];
	char motor_pwm[NUM_CHANNELS];
	char LEDs;
};

#endif /* APP_SPIMOVE_H_ */
