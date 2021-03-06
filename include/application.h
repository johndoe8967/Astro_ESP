/*
 * application.h
 *
 *  Created on: 30.09.2016
 *      Author: johndoe
 */

#ifndef INCLUDE_APPLICATION_H_
#define INCLUDE_APPLICATION_H_

#define SPI_MISO 3	/* Master In Slave Out */
#define SPI_MOSI 1	/* Master Out Slave In */
#define SPI_CLK  0	/* Serial Clock */
#define SPI_CS  2	/* Slave Select */
#define SPI_DELAY 10	/* Clock Delay */
#define SPI_BYTE_DELAY 10 /* Delay between Bytes */


enum MODES {move=0, track=1, sync=2, potiMag=3, slew=5, moving=10, tracking=11, syncing=12,potiMove=13,};
void setMode(MODES newMode);

//#define debugSPI

#endif /* INCLUDE_APPLICATION_H_ */
