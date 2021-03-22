//**********************************************************
//Sets up the SPI interface for the flash memory
void spi_init(void);

//**********************************************************
// Read status register:[RDY, COMP, 1, 1, 0, 1, PROTECT, PAGESIZE]
unsigned char readStatusRegister(void);

//**********************************************************
// is flash memory busy? The seventh bit of the status register is "not busy"
unsigned char isBusy(void);

//**********************************************************
// Write n bytes to buffer starting at "byte," number within buffer (0-527)
// bufferNum == 1 or 2, length of data[] should be n
void writeBuffer(unsigned char [], unsigned int , unsigned int, char );

//*********************************************************
// Read from buffer byte number within buffer (0-527)
// bufferNum == 1 or 2
void readBuffer(unsigned char* , unsigned int , unsigned int , char );

//**********************************************************
// write main memory from buffer (with or without built-in erase)
//erase=0--no built in erase
void bufferToMemory(unsigned int , char , char );

//**********************************************************
// Read n bytes from flash, puts result in data buffer
// page number (0-8191), byte number within page (0-527)
void readFlash(unsigned char* , unsigned int , unsigned int , unsigned int);

//**********************************************************
// Erase a block of memory (8 pages)
// block (0-1023)
// Note: page number k is in block number (k>>3)
void eraseBlock(unsigned int );

//****************************************************
//Erases the 8 blocks associated with the inputted char
void eraseFace(unsigned char)

//****************************************************
// Transfer face p to face q in flash, using buffer
void facetransfer(unsigned char, unsigned char, unsigned char*);
