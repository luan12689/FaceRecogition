#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>

volatile unsigned char PCLK;

#include "uart.h"
#include "uart.c"
#include "MatlabLib.h"
#include "MatlabLib.c"
#include "flashmem.h"
#include "flashmem.c"
#include "camlib.h"
#include "camlib.c"
#include "twimaster.c"

#define begin {
#define end   } 

#define CNTS_FLOP 100


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

#define PageSize 528
#define FaceSize 64
#define ActualFace 48

#define BUT0 (~PINC & 0x08)
#define BUT1 (~PIND & 0x04) 


char PushStateA;
char PushStateB;
char numFrames;

unsigned char curFace;
unsigned char* FrameRead;



//**********************************************************
//timer 0 compare ISR
ISR (TIMER0_COMPA_vect){    
  //Decrement the times if it is not already zero
  if (transCnt>0) transCnt--;
}


//**********************************
//Reads the desired number of pages at face n
void sendFacetoMatlab(unsigned char face)begin
	FrameRead = (unsigned char*) malloc(PageSize);
	for (int i=0; i<ActualFace; i++) begin
		int pageNum = (face*FaceSize) + i;
		PORTD ^= 0x80;
		readFlash(FrameRead, PageSize, pageNum, 0);
		dumpFrame(FrameRead, PageSize);

	end
	free(FrameRead);
end


//************************************************
//Transition to SEND items to MATLAB
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
						setUp(1, ActualFace);
						sendFacetoMatlab(51);
						SETBIT(PORTD,7);
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
				 	if (curFace==0) eraseFace(0);
					takePicture(curFace);
					while(isBusy());
					curFace++;
					SETBIT(PORTD, 7);
					eraseFace(curFace);
        }   
        break;
  }	
}
         

//********************************************************** 
//Set it all up
void initialize(void)
begin	 	

  //set up the ports
  //DDRA=0xff;    // PORT A is LCD
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

	unsigned char f;
	f = camera_init();
	if (f){
		while(1){
			SETBIT(PORTD, 7);
			_delay_ms(100);
			CLRBIT(PORTD, 7);
		}
	}


	spi_init();
  
	//init the UART -- uart_init() is in uart.c
  uart_init();
  stdout = stdin = stderr = &uart_str;

  //crank up the ISRs
  sei();

	// flash LEDs once before starting
  CLRBIT(PORTD, 7);
  _delay_ms(500);
  SETBIT(PORTD, 7);
  _delay_ms(500);
  
  //printf("Starting\n\r");

  PushStateA = 1;
  PushStateB = 5;
  curFace = 0;
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
  

	

