#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <string.h>

#include "uart.h"
#include "uart.c"
#include "MatlabLib.h"
#include "MatlabLib.c"
#include "flashmem.h"
#include "flashmem.c"


#define begin {
#define end   } 


FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void flashLED(void);
void initialize(void);

void transitionA(void);
void transitionB(void);

volatile unsigned int transCnt; //timer for debouncer
unsigned char led;					//light states
unsigned char input;


// state machine constants
#define NoPushA 1 
#define MaybePushA 2
#define PushedA 3
#define MaybeNoPushA 4
#define NoPushB 5
#define MaybePushB 6
#define PushedB 7
#define MaybeNoPushB 8
#define CNTS_FLOP 20

#define PageSize 528
#define FaceSize 64
#define ActualFace 48

#define BUT0 (~PINC & 0x08)
#define BUT1 (~PIND & 0x04) 

#define SETBIT(REG,BITNUM) ((void) (REG |= 1<<(BITNUM)))
#define CLRBIT(REG,BITNUM) ((void) (REG &= ~(1<<(BITNUM))))

char PushStateA;
char PushStateB;
char numFrames;
unsigned char * buffer;
unsigned char MatlabPage [PageSize];


//**********************************************************
//timer 0 compare ISR
ISR (TIMER0_COMPA_vect){    
  //Decrement the times if it is not already zero
  if (transCnt>0) transCnt--;
}


//***********************************************
//Takes a "page" from MATLAB to flash
void pageMatlabFlash(int pageNum){
	//SHOULD HAVE ERASED PAGE BEFORE DOING THIS
	CLRBIT(PORTD, 7);
	int dataSize;
	scanf("%d", &dataSize);
	SETBIT(PORTD, 7);
	readFrame(dataSize, MatlabPage);
	CLRBIT(PORTD,7);
	while(isBusy());
	writeBuffer(MatlabPage, PageSize, 0, 1);
	SETBIT(PORTD,7);
	while(isBusy());
	bufferToMemory(pageNum, 1, 0);
	CLRBIT(PORTD,7);

}


//***********************************************
//Takes a "face" from MATLAB to flash
void faceMatlabFlash(char face){
	CLRBIT(PORTD,7);
	_delay_ms(100);
	SETBIT(PORTD,7);
	eraseFace(face);
	CLRBIT(PORTD,7);
	int initPage = face<<6;
	char pagesPerFace;
	scanf("%d", &pagesPerFace);
	SETBIT(PORTD,7);
	for(int i=0; i<pagesPerFace; i++){
		pageMatlabFlash(initPage+i);
	}

	SETBIT(PORTD,7);
}

//**********************************
//Reads the desired number of pages at face n
void sendFacetoMatlab(unsigned char face){
	//setUp(1, ActualFace);
	buffer = (unsigned char*) malloc(PageSize);
	for (int i=0; i<ActualFace; i++) {
		int pageNum = (face*FaceSize) + i;
		PORTD ^= 0x80;
		readFlash(buffer, PageSize, pageNum, 0);
		dumpFrame(buffer, PageSize);	
	}
	free(buffer);
}

//************************************************
//Send Faces from MATLAB to Flash
void transitionA(){    
  switch (PushStateA)
  {
     case NoPushA: 
        if (BUT0) PushStateA=MaybePushA;
        else PushStateA=NoPushA;
        break;
     case MaybePushA:
        if (BUT0){
           PushStateA=PushedA;
        }
        else PushStateA=NoPushA;
        break;
     case PushedA:  
        if (BUT0) PushStateA=PushedA; 
        else PushStateA=MaybeNoPushA;    
        break;
     case MaybeNoPushA:
        if (BUT0) PushStateA=PushedA; 
        else {
            PushStateA=NoPushA;
				CLRBIT(PORTD,7);
				char numFaces;
				scanf("%d", &numFaces);
				SETBIT(PORTD,7);
				for(int m=0; m<numFaces; m++){
					faceMatlabFlash((char)m+32);
				}
				if (numFaces==0){
					while(1){
						PORTD = PORTD ^ 0x80;
						_delay_ms(100);
					}
				}
			SETBIT(PORTD, 7);
			}
        break;			
  }	
}

//*****************************************************
//Transition to take a picture
void transitionB(){

  switch (PushStateB)
  {
     case NoPushB: 
        if (BUT1) PushStateB=MaybePushB;
        else PushStateB=NoPushB;
        break;
     case MaybePushB:
        if (BUT1){
           PushStateB=PushedB;
        }
        else PushStateB=NoPushB;
        break;
     case PushedB:  
        if (BUT1) PushStateB=PushedB; 
        else PushStateB=MaybeNoPushB;    
        break;
     case MaybeNoPushB:
        if (BUT1) PushStateB=PushedB; 
        else {
           PushStateB=NoPushB;
			CLRBIT(PORTD, 7);
			setUp(1, ActualFace);
			sendFacetoMatlab(0);
			SETBIT(PORTD, 7);
        }   
        break;
  }	
}
         

//********************************************************** 
//Set it all up
void initialize(void)
begin	 	

  //set up the ports
	DDRB=0xa8;
	DDRC=0x00;
	DDRD=0x80;  //PORT D.7 is LED

	SETBIT(PORTC, 3);//Pull Up for button
	SETBIT(PORTD, 2);//Pull Up for button

  //set up timer 0 for 1 mSec ticks
  TIMSK0 = 2;		//turn on timer 0 cmp match ISR 
  OCR0A = 250;  	//set the compare reg to 250 time ticks
  TCCR0A = 0b00000010; // turn on clear-on-match
  TCCR0B = 0b00000011;	// clock prescalar to 64

	//init the task timer
  transCnt = CNTS_FLOP;

	//init SPI for flash
		spi_init();

	//init the UART -- uart_init() is in uart.c
  uart_init();
  stdout = stdin = stderr = &uart_str;

  //crank up the ISRs
  sei();

	// flash LEDs once before starting
  CLRBIT(PORTD, 7);
  _delay_ms(1000);
  SETBIT(PORTD, 7);
  _delay_ms(500);
  
  //printf("Starting\n\r");

  PushStateA = 1;
  PushStateB = 5;
	//PORTC = isBusy() << 2;
end


//*****************************
int main(void){
	initialize();

	while(1){
  	// reset time and call task
		if (transCnt==0){transitionA(); transitionB(); transCnt = CNTS_FLOP;}

	}
}
  

	

