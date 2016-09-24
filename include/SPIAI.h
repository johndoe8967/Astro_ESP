/*
 * SPIAI.h
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#ifndef APP_SPIAI_H_
#define APP_SPIAI_H_

#include "SPIDevice.h"

#define AISPIBufLen 4
#define NUM_CHANNELS 2

class SPI_AI: public SPIDevice {
public:
	SPI_AI();
	virtual ~SPI_AI();

	void setSPIInBuffer(unsigned char *newData);
	inline size_t getSPIBufferLen() {return AISPIBufLen;};
	unsigned char* getSPIBuffer() {calcSPIOutBuffer(); return bytes;};

	int getAI(unsigned char ch) {if (ch<2) {return AI[ch];}};
	void setFilter(unsigned char val) {if (val > 0) filter = val;};

private:
	void calcSPIOutBuffer();

	unsigned char *bytes;
	int AI[NUM_CHANNELS];
	unsigned char filter;

};

#endif /* APP_SPIAI_H_ */
