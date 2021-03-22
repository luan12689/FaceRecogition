/*	Main Face Recognition Program 
 *	ECE 4760 Spring 2011
 *	Final Project
 *
 * 	By Cat Jubinski caj65
 */


#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <math.h>
#include <string.h>

volatile unsigned char PCLK;

#include "uart.h"
#include "uart.c"
#include "MatlabLib.h"
#include "MatlabLib.c"
#include "lcd_lib.h"
#include "lcd_lib.c"
#include "flashmem.h"
#include "flashmem.c"
#include "camlib.h"
#include "camlib.c"
#include "twimaster.c"

#define begin {
#define end   } 

#define CNTS_FLOP 40


FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

void initialize(void);

void transitionA(void);
void transitionB(void);

volatile unsigned int transCnt; //timer for debouncer

// state machine constants
#define NoPushA 1 
#define MaybePushA 2
#define PushedA 3
#define MaybeNoPushA 4
#define NoPushB 5
#define MaybePushB 6
#define PushedB 7
#define MaybeNoPushB 8
#define NoPushC 9
#define MaybePushC 10
#define PushedC 11
#define MaybeNoPushC 12

//Flash Memory Stuff
#define PageSize 528
#define FaceSize 64
#define ActualFace 48

//Define buttons
#define BUT0 (~PINC & 0x08)
#define BUT1 (~PIND & 0x04)
#define BUT2 (~PINC & 0x04)

//Face Rec Stuff
#define desMean 80
#define desStd 50
#define numEig 25
#define MaxRegistration 20
#define templatePage 4032
#define corrThresh 500
#define NEWFACE 51
#define MEANFACE 37

//State Machine Stuff
char PushStateA;
char PushStateB;
char PushStateC;

//Buffers for Calculations
unsigned char newFaceBuf[528];
unsigned char oldFaceBuf[528];
signed char signedBuf[528];
signed int tempPage[528];

//Which eigenface is where in memory
unsigned char eigIdx[30] = {32, 33, 34, 35, 36, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

//For verifying eigenfaces
signed int correctCount[30] = { 457, 631, 299, 514, 560, 415, 566, 414, 425, 660, 567, 403, 374, 483, 485, 605, 535, 468, 484, 479, 503, 555, 536, 506, 400, 500, 486, 508, 451, 583};

//Registered User Stuff
unsigned char numRegistered;
signed int * tempSet;
signed int enrTemp[numEig];
unsigned char * FrameRead;

//New Person's Template
signed int newTemp[numEig];

//Demo mode flag
unsigned char demoMode;

//LCD Text
const int8_t LCD_starting[] PROGMEM = "Initializing...\0";
const int8_t LCD_initialize0[] PROGMEM = "      Face\0";
const int8_t LCD_initialize1[] PROGMEM = "     Access\0";
const int8_t LCD_noMatch[] PROGMEM = "No Match Found\0";
const int8_t LCD_tooMany[] PROGMEM = "Too Many Users\0";
const int8_t LCD_picture[] PROGMEM = " Taking Picture\0";
const int8_t LCD_noUsers0[] PROGMEM = "  No Users Are\0";
const int8_t LCD_noUsers1[] PROGMEM = "    Enrolled!\0";
const int8_t LCD_userReset1[] PROGMEM= "  Release To\0";
const int8_t LCD_userReset2[] PROGMEM=" Reset Database\0";
const int8_t LCD_userReset3[] PROGMEM=" Database Reset\0";
int8_t lcd_buffer[17];

unsigned char curFace;
char resetCntr;


//**********************************************************
//timer 0 compare ISR
ISR (TIMER0_COMPA_vect){    
  //Decrement the times if it is not already zero
  if (transCnt>0) transCnt--;
}

//**********************************
//Reads the desired number of pages at face n
void sendFacetoMatlab(unsigned char face)begin
	//setUp(1, ActualFace);
	//FrameRead = (unsigned char*) malloc(PageSize);
	for (int i=0; i<ActualFace; i++) begin
		int pageNum = (face*FaceSize) + i;
		//printf("PAGE NUM: %u\n\r", pageNum);
		PORTD ^= 0x80;
		readFlash(newFaceBuf, PageSize, pageNum, 0);
		dumpFrame(newFaceBuf, PageSize);
		

		//_delay_ms(100);
		
	end
	//free(FrameRead);


end
//**********************************************************
//Verifies the eigenfaces are not corrupted
void verifyFlash(void){
	for(int eig=0; eig<30; eig++){
		int count255=0;
		for(int page=0; page<ActualFace; page++){
			readFlash(oldFaceBuf, PageSize, eigIdx[eig]*64+page,0);
			for(int pix=0; pix<PageSize; pix++){
				if(oldFaceBuf[pix]==255) count255++;
			}
		}
		int diff = count255 - correctCount[eig];
		printf("EigFace %i, Diff: %i\n\r", eig, diff);	
	}
	// do mean face
	int count128 = 0;
	for(int page=0; page<ActualFace; page++){
		readFlash(oldFaceBuf, PageSize, MEANFACE*64+page,0);
		for(int pix=0; pix<PageSize; pix++){
			if(oldFaceBuf[pix]==128) count128++;
		}
	}
	int diff = count128 - 21;
	printf("MeanFace  Diff: %i\n\r", diff);	

}
//**********************************************************
// Load user k's template into global variable enrTemp (k is 0-based)
void loadTemplate(char k){
	unsigned int startPage = templatePage;
	unsigned int startByte = 2*k*numEig+1;
	if (k>=10){ // This user's template is on the second page
		startPage = templatePage+1;
		startByte = 2*(k-10)*numEig;
	}
	unsigned char * rawData;
	rawData = (unsigned char *) malloc(2*numEig);
	readFlash(rawData, 2*numEig, startPage, startByte);
	// Two bytes per template entry.
	// Template entry i is 256*rawData[2*i] + rawData[2*i+1]
	for (int i=0; i<numEig; i++){
		enrTemp[i] = (((int)rawData[2*i])<<8) + (int)rawData[2*i+1];
	}
	free(rawData);
}

//*********************************************************
//resets the templates of the enrolled users
void resetTemplates(){

	eraseBlock(templatePage>>3);
	while(isBusy());
	numRegistered = 0;
	LCDclr();
	CopyStringtoLCD(LCD_userReset3, 0, 0);

}

//***********************************************
//Given a template that has already been checked/created
//add the template to system in flash
char registerUser(int * temp){

	// find the correct page
	unsigned int page = templatePage;
	unsigned int byteidx = 2*numRegistered*numEig + 1;
	if (numRegistered >= 10){ // this user will go on second page
		page = templatePage + 1;
		byteidx = 2*(numRegistered-10)*numEig;
	}
	
	// bring page into uC
	readFlash(newFaceBuf, 528, page, 0);

	// append new template
	for (int i=0; i<numEig; i++){
		unsigned char b1 = (unsigned char)((temp[i]>>8) & 0x00ff);
		unsigned char b2 = (unsigned char) (temp[i] & 0x00ff);
		newFaceBuf[byteidx+2*i] = b1;
		newFaceBuf[byteidx+2*i+1] = b2;
	}

	// write back to Flash
	writeBuffer(newFaceBuf,528,0,1);
	bufferToMemory(page,1,1);
	while(isBusy());
	
	// increment numRegistered
	readFlash(newFaceBuf,528,templatePage,0);
	numRegistered++;
	newFaceBuf[0] = numRegistered;
	writeBuffer(newFaceBuf,528,0,1);
	bufferToMemory(templatePage,1,1);
	while(isBusy());

	return numRegistered-1;
}


//**********************************************************
//Calculate mean of face 
//Total number of points is (47 * 528)+352
//1/572 * (x/44 +y/44 +...) where x is sum of a page
unsigned char calcMean(char face){
	sprintf(lcd_buffer, "Calculating");
	LCDclr();
	LCDGotoXY(0,0);
	LCDstring(lcd_buffer, strlen(lcd_buffer)) ;
	sprintf(lcd_buffer, "Mean");
	LCDGotoXY(5,1);
	LCDstring(lcd_buffer, 4) ;
	unsigned int mean=0;
	unsigned int sum=0;
	int pageIndex=face*64;
	for (int i=0; i<ActualFace; i++){
		//Last Page does not have data in final 176 bytes
		//because picture is only 176x143
		if(i<ActualFace-1){
			readFlash(newFaceBuf, 528, pageIndex+i, 0);
			
			for (int k=0; k<4; k++){
				sum = 0;
				for (int j=0; j<PageSize/4; j++){
					sum += newFaceBuf[j+k*(PageSize/4)];
				}

				mean+=sum/176;
			}
		}else{
			readFlash(newFaceBuf, 352, pageIndex+i, 0);
			for (int k=0; k<2; k++){
				sum = 0;
				for (int j=0; j<176; j++){
					sum += newFaceBuf[k*176 + j];
				}
				mean+=sum/176;
			}
		}	
	}
	mean = mean/143;
	LCDclr();
	printf("Mean: %u\n\r",mean);
	return (char) mean;
}

//************************************************
//Calculate the norm of a vector. return floating point
float calcNorm(int * vec, char length){
	float sumSquares=0;
	for(int i=0; i<length; i++){
		sumSquares += (float)vec[i] * (float)vec[i];
	}
	sumSquares = sqrt(sumSquares);
	return sumSquares;
}




//************************************************
//Calculates Std Dev of Face 0 given its mean
//Total number of points is (47 * 528)+352
//sqrt(1/572 * (x/44 +y/44 +...) 
//where x is (x_i -mean)^2
signed int calcSD(char face, char mean){
	LCDclr();
	sprintf(lcd_buffer, " Calculating...");
	LCDGotoXY(0,0);
	LCDstring(lcd_buffer, strlen(lcd_buffer)) ;
	sprintf(lcd_buffer, "Stdev");
	LCDGotoXY(5,1);
	LCDstring(lcd_buffer, 5) ;
	int pageIndex=face*64;

	uint32_t sumSquares = 0;
	uint32_t stdDev = 0;
	for (int i=0; i<ActualFace; i++){
		//Last Page does not have data in final 176 bytes
		//because picture is only 176x143
		if(i<ActualFace -1){
			readFlash(newFaceBuf, 528, pageIndex+i, 0);
			for(int j=0; j<PageSize; j++){
				signed int dif = (signed int)newFaceBuf[j]-(signed int)mean;
				sumSquares +=  (uint32_t)((int32_t)dif * (int32_t)dif);
			}
		}else{
			readFlash(newFaceBuf, 352, pageIndex+i, 0);
			for(int j=0; j<352; j++){
				signed int dif = (signed int)newFaceBuf[j]-(signed int)mean;
				sumSquares +=  (uint32_t)((int32_t)dif * (int32_t)dif);
			}
		}
		stdDev +=  sumSquares;
		sumSquares = 0;
	}
	stdDev = sqrt(stdDev);
	LCDclr();
	printf("Stdev: %u\n\r",(unsigned int) stdDev);
	return (signed int)stdDev;
}

//************************************************
//Finds the correlation between two vectors
//Correlation is measured as the cosine of the angle 
//between the two vectors.
 float findCorr(signed int * vec1, signed int * vec2){
	float norm1 = calcNorm(vec1, numEig);
	float norm2 = calcNorm(vec2, numEig);
	float dot = 0;
	for(int i=0; i<numEig; i++){
		dot += (float)vec1[i] * (float)vec2[i];
	}
	float corr = dot/(norm1 * norm2);
	return corr;
}



//************************************************
//Takes Picture, creates template, 
void createTemplate(){
	// clear variable newTemp
	for (int i=0; i<numEig; i++)
		newTemp[i] = 0;

	//Calculate the mean and std dev of temp face
	signed int faceMean;
	signed int faceSD;
	faceMean = (signed int)calcMean(NEWFACE);
	faceSD = calcSD(NEWFACE, faceMean)/100;
	//Normalize Face and calculate new template page by page
	LCDclr();
	sprintf(lcd_buffer, " Calculating...");
	LCDGotoXY(0,0);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
	for(int page=0; page<ActualFace; page++){
		sprintf(lcd_buffer, "%2i/48", page);
		LCDGotoXY(5,1);
		LCDstring(lcd_buffer, 5) ;

		//Normalize page 'page'.
		//current picture
		readFlash(newFaceBuf, 528, NEWFACE*64 + page, 0);
		//mean face
		readFlash(oldFaceBuf, 528, page+(MEANFACE*64), 0);
		//Normalize each pixel based on desired mean, stddev, and mean face
		char count=0;
		for(int pix=0; pix<528; pix++){
		
			count++;
			tempPage[pix]=((((int)newFaceBuf[pix] - faceMean)*desStd) / faceSD) + desMean - (int)oldFaceBuf[pix];
		}


		
		//loop through eigenfaces and dot them with the current page
		for(int eig=0; eig<numEig; eig++){
			readFlash(signedBuf, 528, (eigIdx[eig])*64+page, 0);
			
			int32_t contribution2=0;
			for(int indx= 0; indx<3; indx++){
				int32_t contribution1=0;
				if (!(page==47 && indx==2)){
				for(int pix=0; pix<176; pix++){
					contribution1 += (((int32_t)tempPage[(indx*176)+pix]) * ((int32_t)signedBuf[(indx*176)+pix]));
				}//for
				contribution2 = contribution2 + contribution1;
				}//if
			} // for indx
			
			newTemp[eig]=newTemp[eig]+(int)(contribution2/2500);
		} // for eig
	} // for page

	LCDclr();
}



//***********************************************
//Compares the new template with saved templates
//If correlation is greater than threshold, match found
//If 0 matches: returns -1
//If more than one match found: returns -2
//If 1 match found: returns user index
signed char findMatch(signed int * newTemp){
	//find distances between other templates
	float corr;
	signed char match=-1;
	if(numRegistered==0){return -1;}
	for (int i=0; i<numRegistered; i++){ // i is user num
		loadTemplate(i);
		corr = findCorr(newTemp, enrTemp);
		if(corr < corrThresh){
			if(match!=-1){
				return -2;
			}else{match = i;}//user 1 is in index 0
		}
	}
	return match;

}


//************************************************
//Transition to REGISTER
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
			LCDclr();
			CopyStringtoLCD(LCD_picture, 0, 0);
			takePicture(NEWFACE);
			SETBIT(PORTD,7);
			
			while(isBusy());
			createTemplate();

			for (int i=0; i<numEig; i++)
				printf("%6i",newTemp[i]);
			printf("\n\r");

			
			float corr;
			signed int corr0 = 0; // holds maximum correlation
			signed char  user0 = -1;
			signed int corr1 = 0; // holds 2nd max
			signed char  user1 = -1;
			signed int corr2 = 0; // holds 3rd max
			signed char  user2 = -1;
			for (int i=0; i<numRegistered; i++){ // i is user num
				loadTemplate(i);
				corr = findCorr(newTemp, enrTemp);
				printf("User %2i Corr: %i\n\r", i, (int)(100.0*corr));

				signed int thisCorr = (int)(100.0*corr);
				if (thisCorr > 0){
					if (thisCorr > corr0){
						corr2 = corr1; user2 = user1;
						corr1 = corr0; user1 = user0;
						corr0 = thisCorr; user0 = i;
					}else if (thisCorr > corr1){
						corr2 = corr1; user2 = user1;
						corr1 = thisCorr; user1 = i;
					}else if (thisCorr > corr2){
						corr2 = thisCorr; user2 = i;
					}
				}
			}
			printf("\n\r");

			LCDclr();
			if (corr0 < 85){
				char k = registerUser(newTemp);
				sprintf(lcd_buffer, "  Enrolled as");
				LCDGotoXY(0,0);
				LCDstring(lcd_buffer,strlen(lcd_buffer));
				sprintf(lcd_buffer, "    User %i",k);
				LCDGotoXY(0,1);
				LCDstring(lcd_buffer,strlen(lcd_buffer));
			}else{
				sprintf(lcd_buffer, "FAIL: Previously");
				LCDGotoXY(0,0);
				LCDstring(lcd_buffer,strlen(lcd_buffer));
				sprintf(lcd_buffer, "Enrolled User %i",user0);
				LCDGotoXY(0,1);
				LCDstring(lcd_buffer,strlen(lcd_buffer));
			}


			eraseFace(NEWFACE);

		   	}
				
        break;			
  }	
}


//*****************************************************
//Transition to "LOG IN"
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
					//if(numRegistered==0){//Print to LCD NO USers
					//}
					//char tmp = readStatusRegister();
					//printf("%x\n\r",tmp);
					
					CLRBIT(PORTD,7);
					LCDclr();
					CopyStringtoLCD(LCD_picture, 0, 0);
					takePicture(NEWFACE);
					SETBIT(PORTD,7);
					
					while(isBusy());
				
					createTemplate();			
					printf("\n\r Attempted Login Template:\n\r");
					for (int i=0; i<numEig; i++)
						printf("%6i",newTemp[i]);
					printf("\n\r");

					float corr;
					signed int corr0 = 0; // holds maximum correlation
					signed char  user0 = -1;
					signed int corr1 = 0; // holds 2nd max
					signed char  user1 = -1;
					signed int corr2 = 0; // holds 3rd max
					signed char  user2 = -1;
					for (int i=0; i<numRegistered; i++){ // i is user num
						loadTemplate(i);
						corr = findCorr(newTemp, enrTemp);
						printf("User %2i Corr: %i\n\r", i, (int)(100.0*corr));

						signed int thisCorr = (int)(100.0*corr);
						if (thisCorr > 0){
							if (thisCorr > corr0){
								corr2 = corr1; user2 = user1;
								corr1 = corr0; user1 = user0;
								corr0 = thisCorr; user0 = i;
							}else if (thisCorr > corr1){
								corr2 = corr1; user2 = user1;
								corr1 = thisCorr; user1 = i;
							}else if (thisCorr > corr2){
								corr2 = thisCorr; user2 = i;
							}
						}
					}
					printf("\n\r");


					LCDclr();					
					if (demoMode){
						// print 3 highest correlations to LCD
						sprintf(lcd_buffer,"User: %2i %2i %2i",user0,user1,user2);
						LCDGotoXY(0,0);
						LCDstring(lcd_buffer,strlen(lcd_buffer));

						sprintf(lcd_buffer,"Corr: %2i %2i %2i",corr0,corr1,corr2);
						LCDGotoXY(0,1);
						LCDstring(lcd_buffer,strlen(lcd_buffer));
					}else{ // normal mode
						if (corr0>=85 && corr1<85){ // successful login
							sprintf(lcd_buffer,"  Logged in as");
							LCDGotoXY(0,0);
							LCDstring(lcd_buffer,strlen(lcd_buffer));	
							sprintf(lcd_buffer,"     User %i",user0);
							LCDGotoXY(0,1);
							LCDstring(lcd_buffer,strlen(lcd_buffer));	
													
						}else if (corr0 < 85){ // failed login -- no match
							sprintf(lcd_buffer,"  Login Failed");
							LCDGotoXY(0,0);
							LCDstring(lcd_buffer,strlen(lcd_buffer));	
							sprintf(lcd_buffer,"    No Match");
							LCDGotoXY(0,1);
							LCDstring(lcd_buffer,strlen(lcd_buffer));							
						}else{ // failed login -- too many matches
							sprintf(lcd_buffer,"  Login Failed");
							LCDGotoXY(0,0);
							LCDstring(lcd_buffer,strlen(lcd_buffer));	
							sprintf(lcd_buffer,"Too Many Matches");
							LCDGotoXY(0,1);
							LCDstring(lcd_buffer,strlen(lcd_buffer));	
						}
					}


					eraseFace(NEWFACE);
        }   
        break;
  }	
}


//*****************************************************
//Currently Verify Flash...later reset templates
//To reset the templates, Button 2 needs to be held for
//an extended period of time.
void transitionC(){

  switch (PushStateC)
  {
     case NoPushC: 
        if (BUT2) PushStateC=MaybePushC;
        else PushStateC=NoPushC;
        break;
     case MaybePushC:
        if (BUT2){
           PushStateC=PushedC;
        }
        else PushStateC=NoPushC;
        break;
     case PushedC:  
        if (BUT2){ 
				PushStateC=PushedC; 
				
				if (resetCntr == 25){
					LCDclr();
					CopyStringtoLCD(LCD_userReset1, 0, 0);
					CopyStringtoLCD(LCD_userReset2, 0, 1);
				}else{
					resetCntr++;
				}
			}
        else PushStateC=MaybeNoPushC;    
        break;
     case MaybeNoPushC:
        if (BUT2) PushStateC=PushedC; 
        else {
        	PushStateC=NoPushC;
					if(numRegistered==0){//Print to LCD NO USers
					}

					if (resetCntr==25){
						resetTemplates();
					}else{
						demoMode = !demoMode;
						if (demoMode){
							// print Demo Mode to LCD
							LCDclr();
							sprintf(lcd_buffer,"   Demo Mode");
							LCDGotoXY(0,0);
							LCDstring(lcd_buffer,strlen(lcd_buffer));
						}else{
							// print Face Access to LCD
							LCDclr();
							CopyStringtoLCD(LCD_initialize0, 0, 0);
							CopyStringtoLCD(LCD_initialize1, 0, 1);
						}

					}

							
					
					resetCntr=0;
        }   
        break;
  }	
}


//********************************************************** 
//Set it all up
void initialize(void){
	 	
  //set up the ports
	DDRB=0xa8;
	DDRC=0x00;
	DDRD=0x80;  //PORT D.7 is LED

	SETBIT(PORTC, 3);//Pull Up for button
	SETBIT(PORTC, 2);//Pull Up for button
	SETBIT(PORTD, 2);//Pull Up for button

	//set up timer 0 for 1 mSec ticks
	TIMSK0 = 2;		//turn on timer 0 cmp match ISR 
	OCR0A = 250;  	//set the compare reg to 250 time ticks
	TCCR0A = 0b00000010; // turn on clear-on-match
	TCCR0B = 0b00000011;	// clock prescalar to 64

	//init the task timer
  	transCnt = CNTS_FLOP;

	LCDinit();
	LCDcursorOFF();
	LCDGotoXY(0,0);
	LCDclr();
	CopyStringtoLCD(LCD_starting, 0, 0);


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


	PushStateA = 1;
	PushStateB = 5;
	PushStateC = 9;
	//For reseting the enrolled users
	resetCntr=0;


	LCDclr();
	CopyStringtoLCD(LCD_initialize0, 0, 0);
	CopyStringtoLCD(LCD_initialize1, 0, 1);

}


//*****************************
int main(void){
	initialize();
	demoMode = 0;
	readFlash(&numRegistered, 1, templatePage,0);
	if (numRegistered==255)
		numRegistered = 0; // Flash was empty


	while(1){
  	// reset time and call task
		if (transCnt==0){transitionA(); transitionB(); transitionC(); transCnt = CNTS_FLOP;}
	}
}
  

	

