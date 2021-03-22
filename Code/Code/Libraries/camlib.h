/* Library for interfacing with C3088 camera 
*  module (OV6220 image sensor) from AVR ATmega644
*  ECE 4760 Spring 2011
*  Final Project
*
*  By Brian Harding bjh78@cornell.edu
*/

//************************************************
void eraseFace(unsigned char);

//************************************************
// Send a picture to flash memory (at face location k)
void takePicture(unsigned char);

//************************************************
// Read a byte from register regNum in the camera

unsigned char camera_read(unsigned char);

//************************************************
// write data to camera register regNum
// return 1 if communication failure
// return 0 if success

unsigned char camera_write(unsigned char, unsigned char);

//************************************************
// Reset camera and initialize the settings we want
// return 0 if successful, 1 otherwise
unsigned char camera_init(void);

