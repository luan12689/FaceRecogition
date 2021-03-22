/*MATLAB library for interfacing with Microcontroller
ECE 4760 Spring 2011
Final Project

By Cat Jubinski

*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "uart.h"
#include "MatlabLib.h"




#define begin {
#define end }



//********************************************************
//It prints the information 66 bytes at a time for MATLAB.
//If the data isn't a multiple of 66 0's are appended on 
//to the end of the line before sending it to MATLAB.
void dumpFrame(unsigned char* frame, int frameSize )
begin
	char numLines;//number of full lines (66)
	char extra; //remainder after divide by 66
	char perLine = 66;
	if(frameSize<perLine){
		//Tell MATLAB it is receiving 1 line of 66 
		numLines = 1;
		printf("%3u ", numLines);
		printf("\n\r");
		//Send line of data
		for(int i=0; i<perLine; i++) begin
			if(i<frameSize){ printf("%3u ", frame[i]);}
			else{ printf("%3u ", 0);}
		end
		printf("\n\r");
	}else{
		//Calculate and send to MATLAB number of lines of 66 it will receive
		extra = frameSize % perLine;
		numLines = frameSize/perLine;
		if(extra==0){  printf("%3u ", numLines);}
		else{printf("%3u ", numLines+1);}
		
		printf("\n\r");
		
		//Send Lines
		for(int k=0; k<numLines; k++)begin 
			for(int i=0; i<perLine; i++)begin
				printf("%3u ", frame[(k*perLine)+i]);
			end
			printf("\n\r");
		end
		//Send leftover data
		if(extra!=0) begin
			for(int i=0; i<perLine; i++) begin
				if(i<extra){ printf("%3u ", frame[((numLines-1)*perLine)+i]);}
				else{ printf("%3u ", 0);}
			end
			printf("\n\r");
		end
	}

	
end 

//*******************************************************
//if MATLAB is going to receive multiple faces and frames
// tell them how many to expect
void setUp(char numFaces, int numFrames)
begin
	printf("%3u ", numFaces);
	printf("\n\r");
	printf("%3u ", numFrames);
	printf("\n\r");

end


//********************************
//Reads in data from MATLAB and puts it in buffer
void readFrame(unsigned char* buffer)
begin

	int dataSize;
	scanf("%d", &dataSize);

	for( int i=0; i<dataSize; i++){
		scanf("%d", &buffer[i]);
	}


end

	
