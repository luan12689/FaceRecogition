/* Library for interfacing with C3088 camera 
*  module (OV6220 image sensor) from AVR ATmega644
*  ECE 4760 Spring 2011
*  Final Project
*
*  By Brian Harding bjh78@cornell.edu
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>

#include "i2cmaster.h"
#include "flashmem.h"

#define CAMW 0xC0
#define CAMR 0xC1

#define SETBIT(REG,BITNUM) ((void) (REG |= 1<<(BITNUM)))
#define CLRBIT(REG,BITNUM) ((void) (REG &= ~(1<<(BITNUM))))
#define HREF ((PINB>>0)&0x01)
#define VSYNC ((PINB>>1)&0x01)
#define PCLK ((PINB>>2)&0x01)
#define PIXDAT ((PINC & 0xf0)|((PIND>>3) & 0x0f))





//************************************************
// Send a picture to flash memory (at face location k)
void takePicture(unsigned char k){

	unsigned char* data;
	data = (unsigned char*) malloc(176);
	unsigned int buffidx = 0;
	unsigned int startPage = ((int)k)<<6;
	unsigned int pageOffset = 0;	
	unsigned char curBuff = 2; //initial buffer is 1

	// ignore first frame
	while(VSYNC);
	while(!VSYNC);
	// first rising edge here
	while(VSYNC);
	while(!VSYNC);
	// second rising edge here
	while(VSYNC);

	for (int line=0; line<145; line++){
		while (!HREF); // wait for HREF to go high
		for (int pix=0; pix<176; pix++){
			while(!PCLK);
			if (line>0){
				data[pix] = PIXDAT;
				buffidx++;
			}
			while (PCLK);
		}
		
		writeBuffer(data, 176, buffidx - 176, curBuff);
		if (buffidx==528){
			buffidx = 0;
			// write to buffer
			bufferToMemory(startPage+pageOffset,curBuff,0);
			pageOffset++;

			if(curBuff==1){curBuff=2;}
			else{curBuff=1;}
		}
		while(HREF);
	}
		free(data);
}

//************************************************
// Read a byte from register regNum in the camera

unsigned char camera_read(unsigned char regNum){
	unsigned char f;
	unsigned char data;
	// send start condition and SLA+W
	f = i2c_start(CAMW);
	// tell camera what register and send stop condition
	f = i2c_write(regNum);
	i2c_stop();
	// send start condition and SLA+R
	f = i2c_start(CAMR);
	// read byte and send NAK
	data = i2c_readNak();
	// stop		
	i2c_stop();
	return data;
}

//************************************************
// write data to camera register regNum
// return 1 if communication failure
// return 0 if success

unsigned char camera_write(unsigned char regNum, unsigned char data){
	unsigned char f;
	// send start condition and SLA+W
	f = i2c_start(CAMW);
	if (f) return f;
	// tell camera what register
	f = i2c_write(regNum);
	if (f) return f;
	// write data and stop
	f = i2c_write(data);
	if (f) return f;
	i2c_stop();
	return 0;
}

//************************************************
// Reset camera and initialize the settings we want
// return 0 if successful, 1 otherwise
unsigned char camera_init(void){
	
	i2c_init();
	unsigned char f;
	f = camera_write(0x12,0x80); // soft reset
	if (f) return f;
	_delay_ms(2);
	f = camera_write(0x14,0x20); // QCIF frame size (176x144)
	if (f) return f;
	_delay_ms(2);
	f = camera_write(0x12,0x24); // (default setting) AGC enable, YCrCb mode, no AWB
	if (f) return f;
	_delay_ms(2);
	f = camera_write(0x13,0x01); // (default setting) 16 bit format (see datasheet)
	if (f) return f;
	_delay_ms(2);
	f = camera_write(0x11,(0x1C)<<1); // reduce fps
	if (f) return f;
	_delay_ms(2);
	f = camera_write(0x29,0x00); // set to camera master mode
	if (f) return f;
	_delay_ms(2);

	return 0;

}
