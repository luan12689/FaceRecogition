/*	SPI library for reading/writing flash memory from microcontroller
 *	ECE 4760 Spring 2011
 *	Final Project
 *
 * 	By Brian Harding bjh78@cornell.edu
 */

/*	Library for reading/writing AT45DB321D 32Mbit Atmel flash memory
 *  from AVR ATmega644 
*/

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
//#include "flashmem.h"


#define begin {
#define end }

// handy macros
#define SETBIT(REG,BITNUM) ((void) (REG |= 1<<(BITNUM)))
#define CLRBIT(REG,BITNUM) ((void) (REG &= ~(1<<(BITNUM))))

//FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);


//Variables used to interface with flash

unsigned char junk; // temporary variable
unsigned char flashByte; // byte read from flash

//**********************************************************
//Sets up the SPI interface for the flash memory
void spi_init(){
	// Data direction register
	SETBIT(DDRB,7); // SCLK to output
	CLRBIT(DDRB,6); // MISO to input
	SETBIT(DDRB,5); // MOSI to output
	SETBIT(DDRB,3); // use B.3 as chip select instead of B.4
	SETBIT(DDRB,4);
	SETBIT(PORTB,3);
	SETBIT(PORTB,4);
	
	// Set up SPCR
	// SPIE lo, SPE hi, DORD lo (MSB first), MSTR hi, 
	// CPOL lo, CPHA lo (SPI Mode 0),
	// SPR1 and SPR0 low for speed
	CLRBIT(SPCR,7); // disable interrupts
	SETBIT(SPCR,6); // SPE - enable
	CLRBIT(SPCR,5); // DORD msb first
	SETBIT(SPCR,4); // MSTR - set uC as master
	CLRBIT(SPCR,3); // clock phase/polarity
	CLRBIT(SPCR,2); // Mode 0
	CLRBIT(SPCR,1); // fast speed
	SETBIT(SPCR,0); // fast speed

	
	// Set up SPSR
	// overclock bit
	SPSR = 0x00;
	//Speed: 1/16

} // spi_init

//**********************************************************
// Read status register:
// [RDY, COMP, 1, 1, 0, 1, PROTECT, PAGESIZE]
unsigned char readStatusRegister(){
	//opcode D7
	CLRBIT(PORTB,3);//select chip
	SPDR = 0xD7;
	while (!(SPSR & (1<<SPIF)));
	junk = SPDR; // read
	SPDR = junk; // write junk
	while (!(SPSR & (1<<SPIF)));
	flashByte = SPDR;
	SETBIT(PORTB,3);
	return flashByte;
} // readStatusRegister

//**********************************************************
// is flash memory busy?
// The seventh bit of the status register is "not busy"
unsigned char isBusy(){
	return !(readStatusRegister()>>7);
}//isBusy

//**********************************************************
// Write n bytes to buffer
// starting at "byte," number within buffer (0-527)
// bufferNum == 1 or 2
// length of data[] should be n
void writeBuffer(unsigned char data[], unsigned int n, unsigned int byte, char bufferNum){
	// opcode 84H (87H for buffer 2)
	// 3 addr bytes: 10 DC, 10 buffer addr bytes
	// then data

	unsigned char opcode = 0x00;
	if (bufferNum==1){
		opcode = 0x84;
	}else{
		opcode = 0x87;
	}

	unsigned char message[4] = 
			{opcode,            // opcode
			 0x00,              // DC
			 (char) (byte>>8),  // [2DC BA9-BA8]
			 (char) (byte)};   // [BA7-BA0]
	
	CLRBIT(PORTB,3); // select chip
	// start sending bytes over SPI
	for (int k=0; k<4; k++){
		SPDR = message[k];
		while (!(SPSR & (1<<SPIF))); // wait until complete
		junk = SPDR; // "read" register
	}
	// start sending data
	for (int k=0; k<n; k++){
		SPDR = data[k];
		while (!(SPSR & (1<<SPIF))); // wait until complete
		junk = SPDR; // "read" register
	}

	SETBIT(PORTB,3); // deselect chip
}//writeBuffer


//*********************************************************
// Read from buffer
// byte number within buffer (0-527)
// bufferNum == 1 or 2
void readBuffer(unsigned char* data, unsigned int n, unsigned int byte, char bufferNum){
	// opcode D1H (D3H for buffer 2)
	// 14 DC, 12 buffer address

	unsigned char opcode = 0x00;
	if (bufferNum==1){
		opcode = 0xD1;
	}else{
		opcode = 0xD3;
	}

	unsigned char message[4] = 
			 {opcode,              // opcode
			 0x00,              // DC
			 (char) (byte>>8),  // [2DC BA9-BA8]
			 (char) (byte)};     // [BA7-BA0]

	CLRBIT(PORTB,3); // select chip
	//start sending bytes over SPI
	for( int k=0; k<4; k++ ){
		SPDR = message[k];
		while (!(SPSR & (1<<SPIF))); // wait until complete
		junk = SPDR; // read register
	}
	
	// now start reading
	for( int k=0; k<n; k++ ){
		SPDR = 0x00;
		while (!(SPSR & (1<<SPIF))); // wait until complete
		data[k] = SPDR;
	}
	SETBIT(PORTB,3); //deselect chip

	// last value read is the data byte
	
}//readBuffer

//**********************************************************
// write main memory from buffer (with or without built-in erase)
//erase=0--no built in erase
void bufferToMemory(unsigned int page, char bufferNum, char erase){
	// Built-in erase: opcode 83H (86H for buffer 2)
	// No erase: opcode 88H (89H for buffer 2)
	// 3 addr bytes: 1 DC, 13 page address, 10 DC

	unsigned char opcode = 0x00;
	if (bufferNum==1){
		if (erase){
			opcode = 0x83;
		}else{
			opcode = 0x88;
		}
	}else{
		if (erase){
			opcode = 0x86;
		}else{
			opcode = 0x89;
		}
	}

	unsigned char message[4] = 
		{opcode,                      // opcode
		(char)(page>>6), 			  // [DC, PA12-PA6]
		(char)(page<<2),              // [PA5-PA0 2DC]
		0x00};				          // DC

	//start sending bytes over SPI
	CLRBIT(PORTB,3); // select chip
	for( int k=0; k<4; k++ ){
		SPDR = message[k];
		while (!(SPSR & (1<<SPIF))); // wait until complete
		junk = SPDR; // "read" register
	}
	SETBIT(PORTB,3); // deselect chip

}

//**********************************************************
// Read n bytes from flash
// Put result in data buffer
// page number (0-8191)
// byte number within page (0-527)
void readFlash(unsigned char* data, unsigned int n, unsigned int page, unsigned int byte){
	// From datasheet section 6.4 Main Memory Page Read
	// opcode D2H
	// 3 address bytes: [DC PA12-PA6], [PA5-PA0 BA9-BA8], [BA7-BA0]
	// 4 don't care bytes
	
	unsigned char message[8] = 
			{0xD2,                        // opcode
			(char)(page>>6), 			  // [DC, PA12-PA6]
			(char)(page<<2 | byte>>8),    // [PA5-PA0 BA9-BA8]
			(char) byte,			      // [BA7-BA0]
			0x00,						  // 4 Don't care
			0x00,
			0x00,
			0x00};
	
	CLRBIT(PORTB,3); // select chip
	//start sending bytes over SPI
	for( int k=0; k<8; k++ ){
		SPDR = message[k];
		while (!(SPSR & (1<<SPIF))); // wait until complete
		junk = SPDR; // "read" register
	}
	
	// now the flash will send data
	for (int k=0; k<n; k++){
		SPDR = 0x00; // send junk
		while (!(SPSR & (1<<SPIF))); // wait until complete
		data[k] = SPDR; // read data byte
	}
	SETBIT(PORTB,3); // deselect chip
}

//**********************************************************
// Erase a block of memory (8 pages)
// block (0-1023)
// Note: page number k is in block number (k>>3)
void eraseBlock(unsigned int block){
	// opcode 50
	// 2 DC, 10 page addr, 12 DC

	unsigned char message[4] = 
		{0x50,                        // opcode
		(char)(block>>3),			  // [2DC,PA12-PA6]
		(char)(block<<5),             // [PA5-PA3 5DC]
		0x00};				          // DC

	
	//start sending bytes over SPI
	CLRBIT(PORTB,3); // select chip
	for( int k=0; k<4; k++ ){
		SPDR = message[k];
		while (!(SPSR & (1<<SPIF))); // wait until complete
		junk = SPDR; // "read" register
	}
	SETBIT(PORTB,3); // deselect chip


} //blockErase


//************************************************
void eraseFace(unsigned char k){
	unsigned int startBlock = ((int)k) << 3;
	for (int i=0; i<8; i++){
		while(isBusy());
		eraseBlock(startBlock+i);	
	}
	while(isBusy());
}

//****************************************************
// Transfer face p to face q in flash, using buffer
void facetransfer(unsigned char p, unsigned char q, unsigned char* buf){
	eraseFace(q);
	while(isBusy());
	for(int i=0; i<48; i++){
		CLRBIT(PORTD, 7);
		readFlash(buf, 528, p*64+i, 0);
		while(isBusy());
		writeBuffer(buf, 528, 0, 1);
		while(isBusy());
		bufferToMemory(q*64+i, 1, 0);
		SETBIT(PORTD,7);
		_delay_ms(20);
	}
}
