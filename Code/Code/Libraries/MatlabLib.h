//********************************************************
//It prints the information 66 bytes at a time for MATLAB.
//If the data isn't a multiple of 66 0's are appended on 
//to the end of the line before sending it to MATLAB.
void dumpFrame(unsigned char*, int);

//*******************************************************
//if MATLAB is going to receive multiple faces and frames
// tell them how many to expect
void setUp(char, int);

//********************************
//Reads in data from MATLAB and puts it in buffer
void readFrame(unsigned char*);
