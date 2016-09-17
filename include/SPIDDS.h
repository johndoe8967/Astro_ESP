/*
 * SPIDDS.h
 *
 *  Created on: 11.09.2016
 *      Author: johndoe
 */

#ifndef APP_SPIDDS_H_
#define APP_SPIDDS_H_

#include "SPIDevice.h"

#define DDSSPIBufLen 4


class SPI_DDS: public SPIDevice {
public:
	SPI_DDS();
	virtual ~SPI_DDS();

	void setSPIInBuffer(unsigned char *newData);
	inline size_t getSPIBufferLen() {return DDSSPIBufLen;};
	unsigned char* getSPIBuffer() {calcSPIOutBuffer(); return bytes;};

	void setDDSValue (unsigned long value) {if (value <= 0x7FFFFFF) {DDSvalue = value;}};
	void setMagnet() {magnet = 1;};
	void clrMagnet() {magnet = 0;};
	void setLED(unsigned char ch) {if(ch<4) { LEDs |= 1<<ch;}};
	void clrLED(unsigned char ch) {if(ch<4) { LEDs &= ~(1<<ch);}};
	char getDI(unsigned char ch) {if(ch<4) { return (DI_IN & (1<<ch)) != 0;}};


private:
	void calcSPIOutBuffer();

	unsigned char *bytes;
	char DI_IN;
	unsigned long DDSvalue;
	char LEDs;
	char magnet;

};

#endif /* APP_SPIDDS_H_ */
