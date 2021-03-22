#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>


#include "uart.h"
#include "MatlabLib.h"
#include "flashmem.h"

#define begin {
#define end   } 

#define t1 500

#define baud 130
#define rec_n 1
#define CNTS_FLOP 100


FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void flashLED(void);
void initialize(void);

void transitionA(void);
void transitionB(void);

volatile unsigned int time1, transCnt; //timer for LED

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

char PushStateA;
char PushStateB;

//**********************************
//Reads the desired number of pages at face n
void readFace(unsigned char face)begin
	setUp(FaceSize);
	for (int i=0; i<FaceSize; i++) begin
		int pageNum = (face*FaceSize) + i;
		//printf("PAGE NUM: %u\n\r", pageNum);
		FrameRead = (unsigned char*) malloc(PageSize);
		readFlash(FrameRead, PageSize, pageNum, 0);
		dumpFrame(FrameRead, PageSize);
		PORTC = PORTC ^ 0x02;
		_delay_ms(100);
		PORTC = PORTC ^ 0x02;
	end
end

//**********************************
void writeFace(unsigned char face) begin
	for( int i=0; i<FaceSize; i++) begin
		char erase = 0;
		int pageNum = (face*FaceSize) + i;
		FrameWrite = (unsigned char*) malloc(PageSize);
		for (int k =0; k<PageSize; k++)
			FrameWrite[k] = k/(8*(i+1));

		writeBuffer(FrameWrite, PageSize, 0, 1);
		if(i%8==0){		eraseBlock(pageNum>>3);}
		while(isBusy());
		bufferToMemory(pageNum, 1, erase);
		while(isBusy());
		PORTC = PORTC ^ 0x02;
		_delay_ms(100);
		PORTC = PORTC ^ 0x02;
	end

end


//==================================
//Transition to SEND items to MATLAB Button 0
void transitionA(){    
  switch (PushStateA)
  {
     case NoPushA: 
        if (~PINA & 0x01) PushStateA=MaybePushA;
        else PushStateA=NoPushA;
        break;
     case MaybePushA:
        if (~PINA & 0x01){
           PushStateA=PushedA;
        }
        else PushStateA=NoPushA;
        break;
     case PushedA:  
        if (~PINA & 0x01) PushStateA=PushedA; 
        else PushStateA=MaybeNoPushA;    
        break;
     case MaybeNoPushA:
        if (~PINA & 0x01) PushStateA=PushedA; 
        else {
           PushStateA=NoPushA;
						readFace(0);
		   	}   
        break;
		
  }	
}


//==================================
//Transition to RECEIVE items from MATLAB Button 1
void transitionB(){

  switch (PushStateB)
  {
     case NoPushB: 
        if (~PINA & 0x02) PushStateB=MaybePushB;
        else PushStateB=NoPushB;
        break;
     case MaybePushB:
        if (~PINA & 0x02){
           PushStateB=PushedB;
        }
        else PushStateB=NoPushB;
        break;
     case PushedB:  
        if (~PINA & 0x02) PushStateB=PushedB; 
        else PushStateB=MaybeNoPushB;    
        break;
     case MaybeNoPushB:
        if (~PINA & 0x02) PushStateB=PushedB; 
        else {
           PushStateB=NoPushB;
		   			writeFace(0);
        }   
        break;
  }	
}
         
//**********************************************************
//timer 0 compare ISR
ISR (TIMER0_COMPA_vect)
begin      
  if (transCnt>0) transCnt--;
end  


//*****************************
int main(void)
begin
	spi_init();
  initialize();
	while(1) begin 
  	// reset time and call task    
	if (transCnt==0){transitionA(); transitionB(); transCnt = CNTS_FLOP;}

  end
end  

	

//********************************************************** 
//Set it all up
void initialize(void)
begin
 

  //set up the ports
  DDRC=0xff;    // PORT C is an output  
  PORTC=0;
  DDRA = 0x00;		//A.0 is dumpFrame; A.1 is readFrame

  //set up timer 0 for 1 mSec ticks
  TIMSK0 = 2;		//turn on timer 0 cmp match ISR 
  OCR0A = 250;  	//set the compare reg to 250 time ticks
  TCCR0A = 0b00000010; // turn on clear-on-match
  TCCR0B = 0b00000011;	// clock prescalar to 64
    
  //init the LED status (all )
  led=0xff; 
	PORTC = led;
  
  //init the task timers
  time1=t1;
  transCnt = CNTS_FLOP;
  

  //init the UART -- uart_init() is in uart.c
  uart_init();
  stdout = stdin = stderr = &uart_str;
  //crank up the ISRs
  sei();


  PushStateA = 1;
  PushStateB = 5;
end
