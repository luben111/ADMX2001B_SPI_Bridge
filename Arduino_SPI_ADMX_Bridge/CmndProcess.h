//================================================================
// ADMX2001B USB to SPI bridge
// Processing of ANSI terminal commandsANSI terminal commands
// IDEX Biometrics UK
// Written by Luben Hristov
//
// 12-08-24 -- Creating the file with ANSI command definitions
//
//================================================================
#ifndef _CMND_PROCESS_H
#define _CMND_PROCESS_H

#include <Arduino.h>     // when we use definitions like byte, etc. we need to include it

//-------- SPI pin definitions for Arduino Minima are here
#define SPI_SS_PIN   D10       // this is the SPI SS pin (10) 
#define SPI_MISO_PIN D11       // this is the SPI MISO pin (11) 
#define SPI_MOSI_PIN D12       // this is the SPI MOSI pin (12) 
#define SPI_CLK_PIN  D13       // this is the SPI CLK pin (13) 

#define DATA_DELIMITER  (char(0x0C))     // this is invisible character we can add at the end of the data packet 

//-------- STATE machine Z measure
// when IDLE - there is no active Z measurement, don't do anything
// when ACTIVE_Z - will read data from FIFO if 4 words were available, convert the 4 words into 2 double and report. Append counter value at front
enum stateMeasureZ_t {IDLE, ACTIVE_Z, ACTIVE_CAL, ACTIVE_COMMIT_CAL, ACTIVE_CALIBRATE_ERASE, ACTIVE_RELOAD_CAL};  
enum readWrite_t {READ_MODE, WRITE_MODE}; // used in read/write attributes
enum errorWarn_t {ERROR_MSG, WARN_MSG};   // error or warning message type
#define  SIZE_SUB_ARRAY   20     // what is the longest string we can process (like sweep_type - 10chr or error_check - 11chr)


//--------- Function prototypes -----------------------------------------------------------
void CommandSplitter(int cmndLen);
void Command_Processor();
uint32_t Single_ADMX_Frame(byte command, uint16_t address, uint32_t dataOut);
bool IsOK_Report_Err_Warn(const char custMessage[], byte custCommand);  // we can accept the warnings or not 


// You should only specify the default argument in the function prototype. For example, here is the function prototype for your displayNumber function:
// void displayNumber(int, bool=false); // enter digit
// ^^^ That's fine. Now here is the first line of the definition of your displayNumber function:
// void displayNumber(int number, bool showLeadZeros=false)
// ^^^ That's incorrect. It should be:
// void displayNumber(int number, bool showLeadZeros)

#define DEFAULT_MAX_NUMBER_WAIT   40   // we can wait up to 1ms 
uint32_t SingleParamReadWrite_waitDone(byte command, uint16_t address, uint32_t dataOut, readWrite_t flagWrite, int maxWait = DEFAULT_MAX_NUMBER_WAIT);  // Deals with parameter read/write - see the flowchart
uint32_t WaitForDoneAndGetStatus(int max_number_wait = DEFAULT_MAX_NUMBER_WAIT); // wait for status for some max amount of time, can't be inlined because of delayMicrosecond()
void Clear_ADMX_SPI_Errors(void);  // clear the errors of SPI and reset the SPI engine - notice that using too often this function can cause problems
void PrintErrWarnMessage(byte forCommand, const char custMsgStr[], const char msgString[], errorWarn_t msgType);
void CheckStatus_and_Warnings(void); 

void InitialiseSPI(void);

void Bridge_SerialPrint(String myStr);
void Bridge_SerialPrintLn(String myStr);
void Bridge_SerialPrintDelimiter(void) ;

#define STATUS_POLLING_TIME_uS   25   // we poll the status on regular intervals to clam down the communication
 

#define ConvInt32ToFloat(  int32Val)    (*(float*)&int32Val)       // macros for converting integer32 to float
#define ConvFloatToInt32(  floatVal)    (*(uint32_t*)&floatVal)    // macros for converting float to integer32
#define ConvInt64ToDouble( int64Val)    (*(double*)&int64Val)      // macros for converting integer64 to doable
#define ConvDoubleToInt64(doubleVal)    (*(uint64_t*)&doubleVal)   // macros for converting doable to integer64


extern char commandStr[];    // here we accumulate the data from the buffer and we have some limit of max len of string per line
extern char sub0[], sub1[], sub2[], sub3[], sub4[];  // substring commands



#endif //end _CMND_PROCESS_H
