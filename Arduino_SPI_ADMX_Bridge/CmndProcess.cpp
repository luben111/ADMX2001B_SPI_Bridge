#include "api/Common.h"
//================================================================
// ADMX2001B USB to SPI bridge
// Processing of ANSI terminal commands
// IDEX Biometrics UK
// Written by Luben Hristov
//
// 12-08-24 -- Starting the impelementation
//================================================================
#include <Arduino.h>
#include <SPI.h>            // SPI functionality
#include <stdio.h>          // print commands and other stuff
#include "SPI_cmnd.h"       // SPI commands definitions
#include "ANSI_cmnd.h"      // ANSI commands definitions
#include "CmndProcess.h"    // inlcude main functionality
#include "CalSupport.h"     // main calibration coeff/data fetching commands are located here 
#include "LIF.h"            // include debugger interface

//-------- VARIABLE definitions (allocate space )-----------------------------

        //floatResult = (*(float*)&resultA);  // this is working - convert the result straight into single precision floating 

#define SPI_CLOCK_FREQ     8000000  // 8 MHz clock (but tested and working on 24 MHz)
#define  COMMAND_STR_LEN       150  // what is the longest control string (whole line)
char commandStr[COMMAND_STR_LEN];   // here we accumulate the data from the buffer and we have some limit of max len of string per line

char sub0[SIZE_SUB_ARRAY], sub1[SIZE_SUB_ARRAY], sub2[SIZE_SUB_ARRAY], sub3[SIZE_SUB_ARRAY], sub4[SIZE_SUB_ARRAY];  // substring commands

// ----- Measuring Z global variables
int measureZ_counter;             // keeps track of the sequential samples (when count > 1)
stateMeasureZ_t stateMeasureZ = IDLE;   // this is the state machine for measuring Z

bool flag_MEASURE_DONE ;  // set flag measure done
bool flag_DONE         ;  // set flag done
bool flag_ERROR        ;  // set flag error
bool flag_WARNING      ;  // set flag warning
bool flag_FIFO_ERROR   ;  // set flag FIFO error
int  depthFIFO         ;  // how many record reside in the FIFO
int errorCodes         ;  // keep track of the error codes
int warningCodes       ;  // keep track of warning codes


//================================================================
void Bridge_SerialPrint(String myStr)  // as we define the Serial class in this file, we keep local vesrion of the Serial.print() here
{
  Serial.print(myStr); 
}

//================================================================
void Bridge_SerialPrintLn(String myStr) // as we define the Serial class in this file, we keep local vesrion of the Serial.printLn() here
{
  Serial.println(myStr); 
}

//================================================================
void Bridge_SerialPrintDelimiter() // as we define the Serial class in this file, we keep local vesrion of the printing delimiter only
{
  Serial.print(DATA_DELIMITER);  // this is a special character to separate the data blocks (equivalent of ANSI ESC sequences )
}

//================================================================
// Function for single byte SPI transfer (including writing and reading)
//================================================================
void InitialiseSPI(void)
{
  SPI.begin();                     // starts the SPI communication on the hardware serial pins
  SPI.beginTransaction(SPISettings(SPI_CLOCK_FREQ, MSBFIRST, SPI_MODE0));  // set the format of SPI - 10MHz clock, MSB first and sampling on rising edge/ shifting of falling edge (MODE3)
  Clear_ADMX_SPI_Errors();
}


//================================================================
//  Single SPI transmission of 56 bytes for read/write
//================================================================
// this is one single SPI transmission - 56 bits - we can do it for reading and writing (reading result is into dat0..dat3)
#define GAP_BETWEEN_BYTES 4   // how many microseconds to have between bytes
#define SS_CLEARANCE_TIME 4   // gap between SS going low or high and first/last pulse
#define GAP_BETWEEN_TRANSMISSIONS 40 // minimum time is 40 us

uint32_t Single_ADMX_Frame(byte command, uint16_t address, uint32_t dataOut)
{
byte dat0, dat1, dat2, dat3;  // local variables for reading the SPI data out 

  delayMicroseconds(GAP_BETWEEN_TRANSMISSIONS);    // ensure we have enough gap between the 56 bit transmissions              
  digitalWrite(SPI_SS_PIN, LOW);        
  delayMicroseconds(SS_CLEARANCE_TIME);                 

  SPI.transfer(command                       );   // command
  delayMicroseconds(GAP_BETWEEN_BYTES);  // byte gap               
  SPI.transfer((byte)((address >> 8)  & 0xFF));   // address H
  delayMicroseconds(GAP_BETWEEN_BYTES);                 
  SPI.transfer((byte)( address        & 0xFF));   // address L
  delayMicroseconds(GAP_BETWEEN_BYTES);  // byte gap                

  dat0 = SPI.transfer((byte)((dataOut >> 24) & 0xFF));   // data 31..24 into dat0
  delayMicroseconds(GAP_BETWEEN_BYTES);   // byte gap               
  dat1 = SPI.transfer((byte)((dataOut >> 16) & 0xFF));   // data 23..16 into dat1
  delayMicroseconds(GAP_BETWEEN_BYTES);   // byte gap               
  dat2 = SPI.transfer((byte)((dataOut >>  8) & 0xFF));   // data 15..8  into dat2  
  delayMicroseconds(GAP_BETWEEN_BYTES);   // byte gap               
  dat3 = SPI.transfer((byte)( dataOut        & 0xFF));   // data 7..0   into dat3
  
  delayMicroseconds(SS_CLEARANCE_TIME);    // byte gap              
  
  digitalWrite(SPI_SS_PIN, HIGH);       

  return ((uint32_t)(dat0 << 24) | (uint32_t)(dat1 << 16) | (uint32_t)(dat2 << 8) | dat3 ); // return the U32_t result
}


//================================================================
// Splitting space delimited commands up to 5 substrings (extta spaces removed)
// 12-08-2024 -- Tested and working
//================================================================
void CommandSplitter(int cmndLen)    
{  // extract data from commandStr
  int subInd = 0;      // substring index
  int jj_sub = 0;
  sub0[0] = sub1[0] = sub2[0] = sub3[0] = sub4[0] = char(0);  // set setring ends at the beginning, if not used will yield void string

  commandStr[cmndLen] = ' ';  // add artificially a space to trigger the end
  cmndLen++;                  // expand the array to include the new space char

  bool prevCharIsSpace = true;  // if we find two or more consective spaces, we throw 
  for (int ii = 0; ii < cmndLen; ii++)  { 

    char curChar = commandStr[ii];  // get the next char
    if (curChar == ' ')   // found space ?
    {
      if (prevCharIsSpace == false) 
      {  
       // we have space, but the previous character was not space, so this is the end of the substring
        switch (subInd) {  // store data into different subArrays
          case 0: sub0[jj_sub] = char(0);  break; // add end of string
          case 1: sub1[jj_sub] = char(0);  break; // add end of string
          case 2: sub2[jj_sub] = char(0);  break; // add end of string
          case 3: sub3[jj_sub] = char(0);  break; // add end of string
          case 4: sub4[jj_sub] = char(0);  break; // add end of string
          default: break;  // we shopuld not come here
        } // check in which array to store end of string \0
        subInd++;   // we'll store the data into the next substring 
        if (subInd >= 5)
          subInd = 5;    // limit the number of subscans to 6, where the only 5 are active (if subInd=5 we don't do anything)

        prevCharIsSpace = true;
        jj_sub = 0;
      }
      else // previous is also space, so we can remove this one
      {  // we have two consecutive spaces - delete this one
         // don't do anything - throw away the space
      }
    } // was space chacarter
    else 
    { // it's normal character (not space)
      switch (subInd) {  // store data into different subArrays
        case 0: sub0[jj_sub] = curChar;  break; 
        case 1: sub1[jj_sub] = curChar;  break; 
        case 2: sub2[jj_sub] = curChar;  break; 
        case 3: sub3[jj_sub] = curChar;  break; 
        case 4: sub4[jj_sub] = curChar;  break; 
        default: break;
      } // check in which array to store the strings

      jj_sub++;                  // go to next sub 
      prevCharIsSpace = false;   // it's not anymore space the previous character

      if (jj_sub >= SIZE_SUB_ARRAY) // ensure we're not going beyond the array size
        jj_sub = SIZE_SUB_ARRAY-1;  // limit the index - the line was too long

    } // was normal char   
  } // getting next character from the commandStr[]
} // end of <CommandSplitter>


//================================================================
// Print error or warning messages on serial
//================================================================
void PrintErrWarnMessage(byte forCommand, const char custMsgStr[], const char msgString[], errorWarn_t msgType)
{
  if (msgType == ERROR_MSG)
  {
    Bridge_SerialPrint("Error : ");       // report the error string
    Bridge_SerialPrint(custMsgStr);       // report the custom error
    Bridge_SerialPrint(" / 0x");
    Bridge_SerialPrint(String(forCommand, HEX));
    Bridge_SerialPrint(" / ");            // end message - ad new line
    Bridge_SerialPrintLn(msgString);
   }
  else if (msgType == WARN_MSG)
  {
    Bridge_SerialPrint("Warn : ");        // report the error string
    Bridge_SerialPrint(custMsgStr);       // report the custom error
    Bridge_SerialPrint(" / 0x");
    Bridge_SerialPrint(String(forCommand, HEX));
    Bridge_SerialPrint(" / ");
    Bridge_SerialPrintLn(msgString);      // end message - add new line

  }
 
}

//================================================================
// Check do we have errors and clear the error messages
//================================================================
bool IsOK_Report_Err_Warn(const char custMessage[], byte custCommand)  // we can accept the warnings or not 
{

#define MAX_SIZE_WARNING_ERROR_MSG  50
  char messageStr[MAX_SIZE_WARNING_ERROR_MSG+1] = ""; 
 
  if (flag_ERROR) {   // error?
    int errorCode_0F = errorCodes & 0x0F;
    if (errorCode_0F == ADMX_STATUS_FAILED ) {  // command failed
      strcpy(messageStr, "Command failed");     
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
   else if (errorCode_0F  == ADMX_STATUS_TIMEOUT ) {  // command failed
      strcpy(messageStr, "Timeout");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_ATTRIBUTE ) {  // command failed
      strcpy(messageStr, "Invalid attribute");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_ATTR_OUT_OF_RANGE ) {  // command failed
      strcpy(messageStr, "Attribute value out of range");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_ADDRESS ) {  // command failed
      strcpy(messageStr, "Invalid address of command");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_UNCOMMITED_CAL ) {  // command failed
      strcpy(messageStr, "Uncommitted calibration coeffs");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_CURRENT_GAIN ) {  // command failed
      strcpy(messageStr, "Invalid volt/current gain");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_DISPLAY_MODE ) {  // command failed
      strcpy(messageStr, "Invalid display mode for DC res mode");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_SWEEP_TYPE ) {  // command failed
      strcpy(messageStr, "Invalid sweep type for DC mode");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_SWEEP_RANGE ) {  // command failed
      strcpy(messageStr, "Invalid sweep range");   
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_CAL_COEFF_TYPE ) {  // command failed
      strcpy(messageStr, "Invalid AC calibration coefficient type");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_TRIGGER_OVERFLOW ) {  // command failed
      strcpy(messageStr, "System is not ready to take trigger");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_CAL_TYPE ) {  // command failed
      strcpy(messageStr, "Invalid calibration type");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_INVALID_GAIN ) {  // command failed
      strcpy(messageStr, "Invalid calibration gains");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
    else if (errorCode_0F  == ADMX_STATUS_COMP_FAILED ) {  // command failed
      strcpy(messageStr, "Calibration or compensation failed");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
      
    if (errorCodes & ADMX_STATUS_INVALID_COMMAND_STATE ) {  // command failed
      strcpy(messageStr, "Invalid command for the state");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }
      
    if (errorCodes & ADMX_STATUS_LOG_ZERO_ERROR ) {  // command failed
      strcpy(messageStr, "Sweep value is zero for log scale");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }

    if (errorCodes & ADMX_STATUS_LOG_SIGN_ERROR ) {  // command failed
      strcpy(messageStr, "Sign change for log scale error");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }

    if (errorCodes & ADMX_STATUS_VOLT_ADC_ERROR ) {  // command failed
      strcpy(messageStr, "Voltage ADC saturated error");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }

    if (errorCodes & ADMX_STATUS_CURR_ADC_ERROR ) {  // command failed
      strcpy(messageStr, "Current ADC saturated error");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }

    if (errorCodes & ADMX_STATUS_FIFO_ERROR ) {  // command failed
      strcpy(messageStr, "FIFO over/under flow error");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }

    if (errorCodes & ADMX_STATUS_COUNT_EXCEEDED ) {  // command failed
      strcpy(messageStr, "Sweep count maximum value exceeded");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, ERROR_MSG); }

   } // flag error was set

  if (flag_WARNING) {  // warning? We can have multiple warnings at once

    if (warningCodes & DDS_NCO_FREQ_WARN ) {  // command failed
      strcpy(messageStr, "DDS & NCO Frequency are not equal warning");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

    if (warningCodes & CAL_LOAD_FAIL_WARN ) {  // command failed
      strcpy(messageStr, "Calibration failed warning");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

    if (warningCodes & AUTORANGE_DISABLE_WARN ) {  // command failed
      strcpy(messageStr, "Autorange disabled warning");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

    if (warningCodes & AUTORANGE_FAIL_WARN ) {  // command failed
      strcpy(messageStr, "Autorange failed warning");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

    if (warningCodes & SWEEPCOUNT_WARN ) {  // command failed
      strcpy(messageStr, "Sweep count warning");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

    if (warningCodes & MAG_EXCEED_WARN ) {  // command failed
      strcpy(messageStr, "Measurement magnitude is set to 1 V");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

    if (warningCodes & OFFSET_LIMITED_WARN ) {  // command failed
      strcpy(messageStr, "Measurement offset is set to 0 V");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

    if (warningCodes & OFFSET_POS_EXCEED_WARN ) {  // command failed
      strcpy(messageStr, "Positive offset exceed warning");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

    if (warningCodes & OFFSET_NEG_EXCEED_WARN ) {  // command failed
      strcpy(messageStr, "Negative offset exceed warning");    
      PrintErrWarnMessage(custCommand, custMessage, messageStr, WARN_MSG); }

  } // we have one or more warnings - let's process them

  return (!flag_ERROR);  // if false we have errors, TRUE - is OK

}

//================================================================
// Clear SPI errors and reset the SPI engine
//================================================================
void Clear_ADMX_SPI_Errors(void)
{
  Single_ADMX_Frame(CMD_CLEAR_ERROR, 0, 0);         // reset all error codes (if any)
  WaitForDoneAndGetStatus();                        // check if clear flag is OK
} 

//================================================================
// Poll the status, especially we're interesetd in DONE flag
//================================================================
void CheckStatus_and_Warnings(void)
{
  WaitForDoneAndGetStatus(1);  // read the status but don't wait - return immediately

  if (flag_WARNING)  // let's pull the data from warning register, flagWarning was set before this
  {
    Single_ADMX_Frame(CMD_WARNING_READ, 0, 0);  // initiate reading of the warning messages from status register
    WaitForDoneAndGetStatus();  // wait as long as needed to poll the status
    warningCodes = Single_ADMX_Frame(CMD_RESULT_READ, 0, 0) & MASK_ALL_WARNING_MSG;
  }

} // end of CheckStatus_and_Warnings(void)

//================================================================
// SingleParamReadWrite_waitDone - read or write parameters from registers
//================================================================
uint32_t SingleParamReadWrite_waitDone(byte command, uint16_t address, uint32_t dataOut, readWrite_t flagWrite, int maxWait)  // Deals with parameter read/write - see the flowchart
{
  flag_ERROR = false; // we also clear the flag here - we start new command

  Single_ADMX_Frame(command, address, dataOut);            // this is the initiating of the command execution, no need to check the sttaus
  uint32_t currStatus = WaitForDoneAndGetStatus(maxWait);  // check if DONE was asserted, we can vary the max wait time here

  uint32_t paramVal = 0;  
//---------- READING RESULT register and the attributes
  if (flagWrite == READ_MODE)   // do we have READING of attributes in order to access the CMD_RESULT register
  {
    paramVal = Single_ADMX_Frame(CMD_RESULT_READ, 0, 0);     // get the parameter data and thus complete the parameter reading
  } // end of reading

  warningCodes = 0;  // in case there are no warnings - we'll clear the flags
  if (flag_WARNING)  // let's pull the data from warning register
  {
    Single_ADMX_Frame(CMD_WARNING_READ, 0, 0);  // initiate reading of the warning messages from status register
    currStatus = WaitForDoneAndGetStatus();     // we should not limit and wait till warnings are ready 
    warningCodes = Single_ADMX_Frame(CMD_RESULT_READ, 0, 0) & MASK_ALL_WARNING_MSG;
  }
  
  // there is no need to generate the error/warning events on this place, this can be dome later by using IsOK_Report_Err_Warn()
  return (paramVal);   // return the parameter value
} // end of SingleParamReadWrite_waitDone()

//================================================================
// Wait for Done status
//================================================================
uint32_t WaitForDoneAndGetStatus(int max_number_wait)  // wait for status for some max amount of time
{
  uint32_t currStatusVal = 0;  // var where we keep the last read status 
  int ii;   // keep it external, so we can track how many times we wait in the loop
  for (ii = 0; ii < max_number_wait; ii++)
  {                            // notice that the SingleCommandFrame is taking ~20us and in total the loop period goes to 82us
 
                            // moving this delay after the Single_ADMX_Frame results is wrong data comming out!!!
    currStatusVal = Single_ADMX_Frame(CMD_STATUS_READ, 0, 0);   // only reads the status register
    
    if (currStatusVal & ADMX200X_STATUS_DONE_BITM)  // is it done?
    {      
      break;  // abort the for loop
    }  // we finally got the condition - flag DONE was set

    delayMicroseconds(25);  // we need to bring some microseconds delay to avoid too offten polling of the status

  }

  flag_DONE         = (currStatusVal  & ADMX200X_STATUS_DONE_BITM)?           true : false;  // set flag done
  
  if (flag_DONE) {
    flag_ERROR        = (currStatusVal  & ADMX200X_STATUS_ERROR_BITM)?        true : false;  // set flag error
    flag_WARNING      = (currStatusVal  & ADMX200X_STATUS_WARN_BITM)?         true : false;  // set flag warning
    errorCodes        = (currStatusVal  & MASK_ALL_ERROR_MSG);                               // extract the error codes for getting better understanding of what's going on
  } // DONE asserted
  else {  // not done - reset these flags as there is no info
    flag_ERROR        = false;  // no info
    flag_WARNING      = false;  // no info
    errorCodes        = 0; // no info   
  } // DONE = FALSE

  flag_MEASURE_DONE = (currStatusVal  & ADMX200X_STATUS_MEASURE_DONE_BITM)? true : false;  // set flag measure done
  flag_FIFO_ERROR   = (currStatusVal  & ADMX200X_STATUS_FIFO_ERROR_BITM)?   true : false;  // set flag FIFO error
  depthFIFO         = ((currStatusVal & ADMX200X_STATUS_FIFO_DEPTH_BITM) >> 16) & 0xFF;    // set number of records in FIFO

  return (currStatusVal);   // the last value of the status register is held here

}

//================================================================
// Main processing of commands - we send SPI commands according to the ANSI commands strings in sub0..sub5
//================================================================
void Command_Processor(void)
{
    float floatResult;                        // temp variable for float data
    float arg1, arg2;                         // keeps track of command arguments sent by the user
    uint32_t resultA, resultB, resultC;       // temp data 
    uint32_t statusA;                         // vars ysed in the code

    bool flagGroupProcess = true;             // TRUE if we process group of commands which have something in common (like frequency, count, sweep_scale)
    byte grCommand;                           // the command we send to SPI
    enum argum_t { FLOAT_T, INT_T, ENUM_T };  // the type of conversion
    argum_t argumentTypeRd = INT_T;          // what is the exact type of conversion for reading
    argum_t argumentTypeWr = INT_T;          // what is the exact type of conversion for reading
    
    uint32_t enumValue;                       // some commands require conversion of char[] arguments to int, here we keep the value to write
    bool enumError = false;                   // TRUE if it was not found a corresponding string in sub1 for the enum argument 
    char pos_String[SIZE_SUB_ARRAY] = "";     // pos_string the response may inlcude some string like kHz, V, etc.   
    float scaling = 1;                        // Frequency requires some scaling factor when reporting the value
    char enumStr0[SIZE_SUB_ARRAY] = "";       // string for VAL=0 some sommands retrun verbose response like sweep_scale: linear/log
    char enumStr1[SIZE_SUB_ARRAY] = "";       // string for VAL=0 some sommands retrun verbose response like sweep_scale: linear/log
    char enumStr2[SIZE_SUB_ARRAY] = "";       // string for VAL=0 some sommands retrun verbose response like sweep_scale: linear/log
    char enumStr3[SIZE_SUB_ARRAY] = "";       // string for VAL=0 some sommands retrun verbose response like sweep_scale: linear/log

//--------- the following commands will be processed in the same way, we need to extract arguments and select float/INT32/enum type of data and enumVal
  if(strcmp(sub0, FREQUENCY0) == 0) {
    argumentTypeRd = FLOAT_T;
    argumentTypeWr = FLOAT_T;
    grCommand = CMD_FREQUENCY;  // this is the short byte SPI command
    strcpy(pos_String, FREQUENCY_POS_STR);  // what to display after the value
    scaling = 1000;
  } 
  else if(strcmp(sub0, MAGNITUDE0) == 0) {
    argumentTypeRd = FLOAT_T;
    argumentTypeWr = FLOAT_T;
    grCommand = CMD_MAGNITUDE;  // this is the short byte SPI command
  //  strcpy(pos_String, MAGNITUDE_POS_STR);  // don't show the volts
  }
  else if(strcmp(sub0, OFFSET0) == 0) {
    argumentTypeRd = FLOAT_T;
    argumentTypeWr = FLOAT_T;
    grCommand = CMD_OFFSET;  // this is the short byte SPI command
  //  strcpy(pos_String, OFFSET_POS_STR);  // don't show the volts
  }
  else if(strcmp(sub0, AVERAGE0) == 0) {
    grCommand = CMD_AVERAGE;  // this is the short byte SPI command, default is int
  }
  else if(strcmp(sub0, DISPLAY0) == 0) {
    grCommand = CMD_DISPLAY;  // this is the short byte SPI command, default is int
  }
  else if(strcmp(sub0, MDELAY0) == 0) {
    argumentTypeRd = FLOAT_T;
    argumentTypeWr = FLOAT_T;
    grCommand = CMD_MDELAY;  // this is the short byte SPI command, default is int
    strcpy(pos_String, MDELAY_POS_STR);  // what to display after the value
  }
  else if(strcmp(sub0, TDELAY0) == 0) {
    argumentTypeRd = FLOAT_T;
    argumentTypeWr = FLOAT_T;
    grCommand = CMD_TDELAY;  // this is the short byte SPI command, default is int
    strcpy(pos_String, TDELAY_POS_STR);  // what to display after the value
  }
  else if(strcmp(sub0, COUNT0) == 0) {
    grCommand = CMD_COUNT;  // this is the short byte SPI command, default is int
  }
  else if(strcmp(sub0, TCOUNT0) == 0) {
    grCommand = CMD_TCOUNT;  // this is the short byte SPI command, default is int
  }
  else if(strcmp(sub0, TEMPERAT0) == 0) {
    //-------- TEMPERATURE ENUM CONVERSION ---------------
    argumentTypeRd = FLOAT_T;
    argumentTypeWr = ENUM_T;
    strcpy(enumStr0, TEMP_CELSIUS);    // verbose reponses for 0
    strcpy(enumStr1, TEMP_FARENHEIT);  // verbose reponses for 1

    if(strcmp(sub1, TEMP_CELSIUS) == 0) {
      enumValue = TEMP_CELSIUS_VAL;
      grCommand = CMD_CELSIUS;       // set units   
    } // celsius
    else if (strcmp(sub1, TEMP_FARENHEIT) == 0) {
      enumValue = TEMP_FARENHEIT_VAL; 
      grCommand = CMD_CELSIUS;       // set units   
    } // farenheit
    else if (strcmp(sub1, VOID_STR) == 0) {
        grCommand = CMD_TEMPERATURE;       // this is the short byte SPI command for reading temperature
       }// READ command - avoid setting the error
    else {
      enumError = true;  // we can't find the corresponding enum and we rise the error    
    }
  }
  else if(strcmp(sub0, TRIG_MODE0) == 0) {
    //-------- TRIGGER MODE ENUM CONVERSION ---------------
    argumentTypeRd = ENUM_T;
    argumentTypeWr = ENUM_T;
    grCommand = CMD_TRIGGER_MODE;     // this is the short byte SPI command
    strcpy(enumStr0, TRIG_MODE_INT);  // verbose reponses for 0
    strcpy(enumStr1, TRIG_MODE_EXT);  // verbose reponses for 1

     if(strcmp(sub1, TRIG_MODE_INT) == 0) {
      enumValue = TRIG_MODE_INTER_VAL;
    } // internal trigger mode
    else if (strcmp(sub1, TRIG_MODE_EXT) == 0) {
      enumValue = TRIG_MODE_EXTERN_VAL;    
    } // external trigger mode
    else if (strcmp(sub1, VOID_STR) == 0) {   }// READ command - avoid setting the error
    else {
      enumError = true;  // we can't find the corresponding enum and we rise the error    
    }
  }
//-------- SET GPIO ---------------
  else if(strcmp(sub0, GPIO_CTRL0) == 0) {
    argumentTypeRd = INT_T;
    argumentTypeWr = INT_T;
    grCommand = CMD_SET_GPIO;  // this is the short byte SPI command
  //  strcpy(pos_String, OFFSET_POS_STR);  // don't show the volts
  }

  else if(strcmp(sub0, SWEEP_SCALE0) == 0) {
    //-------- SWEEP SCALE ENUM CONVERSION ---------------
    argumentTypeRd = ENUM_T;
    argumentTypeWr = ENUM_T;
    grCommand = CMD_SWEEP_SCALE;  // this is the short byte SPI command
    strcpy(enumStr0, SWEEP_SCALE_LIN1);  // verbose reponses for 0
    strcpy(enumStr1, SWEEP_SCALE_LOG1);  // verbose reponses for 1

   if(strcmp(sub1, SWEEP_SCALE_LIN1) == 0) {
      enumValue = SWEEP_SCALE_LIN_VAL;
    } // linear scale
    else if (strcmp(sub1, SWEEP_SCALE_LOG1) == 0) {
      enumValue = SWEEP_SCALE_LOG_VAL;    
    } // log scale
    else if (strcmp(sub1, VOID_STR) == 0) {   }// READ command - avoid setting the error
    else {
      enumError = true;  // we can't find the corresponding enum and we rise the error    
    } // was wrong
  } // end of sweep scale
  else if(strcmp(sub0, SWEEPTYPE0) == 0) {
    //-------- SWEEP TYPE ENUM CONVERSION ---------------
    argumentTypeRd = ENUM_T;  // read will require enum
    argumentTypeWr = ENUM_T;  // write will require enum
    grCommand = CMD_SWEEP_TYPE;           // this is the short byte SPI command
    strcpy(enumStr0, SWEEPTYPE_OFF1);     // verbose reponses for 0
    strcpy(enumStr1, SWEEPTYPE_FREQ1);    // verbose reponses for 1
    strcpy(enumStr2, SWEEPTYPE_MAGN1);    // verbose reponses for 2
    strcpy(enumStr3, SWEEPTYPE_OFFSET1);  // verbose reponses for 3
  
    if (strcmp(sub1, SWEEPTYPE_OFF1) == 0) {  // enum = 0
      enumValue = 0;    
    } // off
    else if(strcmp(sub1, SWEEPTYPE_FREQ1) == 0) {  // case 1
      enumValue = 1;
    } // freq
    else if (strcmp(sub1, SWEEPTYPE_MAGN1) == 0) {  // case 2
      enumValue = 2;    
    } // magnitude
    else if (strcmp(sub1, SWEEPTYPE_OFFSET1) == 0) {  // case 3
      enumValue = 3;     
    } // offset
    else if (strcmp(sub1, VOID_STR) == 0) {   }// READ command - avoid setting the error
    else {
      enumError = true;  // we can't find the corresponding enum and we rise the error    
    }
  }
  else
  { // it's not group command - we'll proceed with what's left
    flagGroupProcess = false;
  }


//-------- GROUP PROCESSING OF COMMANDS like frequency/magnitude/count/sweep_type
  if (flagGroupProcess)
  {
    //===================================================================
    //  READ MULTIPLE COMMANDS (like freq/magn/offset/count... etc.)
    //===================================================================
    if (strcmp(sub1, VOID_STR) == 0) // READ command 
    {  
      resultA = SingleParamReadWrite_waitDone(grCommand | CMND_READ_MASK, 0, 0, READ_MODE); // read parameter 
 
      if (IsOK_Report_Err_Warn("Hardware error1", grCommand | CMND_READ_MASK))   // check if no warnings and errors
      { 
        Bridge_SerialPrint(sub0);     // the command name
        Bridge_SerialPrint(" = ");    // the command name

        switch(argumentTypeRd) {
          case FLOAT_T : {
            floatResult = ConvInt32ToFloat(resultA) / scaling;        // convert the result straight into single precision floating  and divide by 1000 (kHz)
            Bridge_SerialPrint(String(floatResult, 4));                     // report the response as floating point
            Bridge_SerialPrintLn(pos_String);                               // add the pos string at the end
            break;
          } // end FLOAT_T

          case INT_T   : {
            Bridge_SerialPrint(String(resultA));                            // report the response as integer
            Bridge_SerialPrintLn(pos_String);                               // add the pos string at the end
            break;
          } // end INT_T

          case ENUM_T  : {
            switch (resultA) {  // switch on different enum responses (up to 4)
              case 0: Bridge_SerialPrintLn(enumStr0); break;                // report the response as verbose
              case 1: Bridge_SerialPrintLn(enumStr1); break;                // report the response as verbose
              case 2: Bridge_SerialPrintLn(enumStr2); break;                // report the response as verbose
              case 3: Bridge_SerialPrintLn(enumStr3); break;                // report the response as verbose
              default: Bridge_SerialPrintLn("Error : Can't find enum"); break;  // we should never come here
            } // end of switch
          } // end ENUM_T
        }

        Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as delimiter for the commands

      }  // was no error and DONE OK  
    } // end of read multiple commands (like freq/magnit/offset,)
    else
    {  
    //===================================================================
    // WRITE MULTIPLE COMMANDS (like freq/magn/offset/count... etc.)
    //===================================================================
      switch(argumentTypeWr) {
        case FLOAT_T : {
          arg1 = atof(sub1);  // this is the extracted value as float from argument1 
          arg2 = arg1 * scaling;  // in case of frequency registers accept Hz, not kHz and we need scaling
          resultA = ConvFloatToInt32(arg2);
          break;
        } // end FLOAT_T

        case INT_T   : {
          resultA = atoi(sub1);  // this is the extracted value as integer from argument1 
          break;
        } // end INT_T

        case ENUM_T  : {
          resultA = enumValue;  // this is the extracted value as integer from enum
          break;
        } // end ENUM_T
      }  // end of switch statement

      if (enumError == false)  { // no error 
        SingleParamReadWrite_waitDone( grCommand, 0, resultA, WRITE_MODE);  // write the value
        if (IsOK_Report_Err_Warn("Wrong arguments", grCommand))   // check if no warnings and errors
        { 
          Bridge_SerialPrint(sub0);     // the command name
          Bridge_SerialPrint(" = ");    // equals
          if (argumentTypeWr == FLOAT_T) {
            Bridge_SerialPrint(String(arg1, 4));   // report the float response
            Bridge_SerialPrintLn(pos_String);      // add pos string if any
          }
          else {
            Bridge_SerialPrint(String(resultA));  // report the integer response
            Bridge_SerialPrintLn(pos_String);      // add pos string if any
          }
        }
      }
      else { // there is enum erro!
        Bridge_SerialPrintLn("Error : Wrong enum argument");
      }

      Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands        
    }  // was write multiple
  } // was group processing

//===================================================================
//
//*******************************************************************
// SINGLE INDIVIDUAL COMMANDS which need special treatment
//*******************************************************************
//
//===================================================================
  else { // not group processing but ordinary commands
    
 //===================================================================
  // GET DEVICE IDN  (*idn? command)
  //===================================================================
    if(strcmp(sub0, IDN0) == 0)  // get device IDN command
    { 
    char outputString[9];  // temp string for HEX values

  //--------- Read the firmware revision
      resultA = SingleParamReadWrite_waitDone(CMD_FW_VERSION, 0, 0, READ_MODE);
      if (IsOK_Report_Err_Warn("Hardware error2", CMD_FW_VERSION)) {
        Bridge_SerialPrint("ADMX2001 - Precision Impedance Analyzer Measurement Module ");     // add pos string if any
        Bridge_SerialPrint(String((resultA >> 24) & 0xFF));     // add pos string if any
        Bridge_SerialPrint(".");     // add pos string if any
        Bridge_SerialPrint(String((resultA >> 16) & 0xFF));     // add pos string if any
        Bridge_SerialPrint(".");     // add pos string if any
        Bridge_SerialPrintLn(String((resultA >> 8) & 0xFF));      // add pos string if any
      }   // check if no warnings and errors

  //--------- Read the unique ID number of the board
      resultA = SingleParamReadWrite_waitDone(CMD_UNIQUE_ID, 1, 0, READ_MODE);  // MSB
      if (IsOK_Report_Err_Warn("Hardware error3", CMD_UNIQUE_ID)) {  
        Bridge_SerialPrint("Board ID - 0x");     // add pos string if any
        sprintf(outputString,"%08X", resultA);    
        Bridge_SerialPrint(outputString);     // add pos string if any
      }   // check if no warnings and errors

      resultA = SingleParamReadWrite_waitDone(CMD_UNIQUE_ID, 0, 0, READ_MODE);  // LSB
      if (IsOK_Report_Err_Warn("Hardware error4", CMD_UNIQUE_ID)) {
        sprintf(outputString,"%08X", resultA);    
        Bridge_SerialPrintLn(outputString);     // add pos string if any
      }   // check if no warnings and errors

      Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as delimiter for the commands

    } // was *idn? command
  //===================================================================
  // RESET BOARD  (reset command)
  //===================================================================
    else if(strcmp(sub0, RESET0) == 0)  // send reset command
    { 

  //--------- Reset
      SingleParamReadWrite_waitDone(CMD_RESET, 0, 0, WRITE_MODE);
      delay(80);   // for time <50ms the DONE flag is not set, need longer time
      WaitForDoneAndGetStatus();  // wait till DONE was set
      if (IsOK_Report_Err_Warn("Hardware error5", CMD_RESET)) {
        Bridge_SerialPrintLn("Reset : success");     // reset was successful
        Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as delimiter for the commands
      }   // check if no warnings and errors
    }
  //===================================================================
  // VOID COMMAND  - used for debugging the protocol
  //===================================================================
    else if(strcmp(sub0, CMND_VOID) == 0)  // send reset command
    { 
      // we just send back the response, no errors, we can accept up to 4 parameters   
      Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as delimiter for the commands
    }  //was void command
 
  //===================================================================
  // ABORT MEASUREMENTS  (abort command)
  //===================================================================
    else if(strcmp(sub0, ABORT0) == 0)  // abort command
    { 

  //--------- Abort 
      SingleParamReadWrite_waitDone(CMD_ABORT, 0, 0, WRITE_MODE);    // abort

      if (IsOK_Report_Err_Warn("Hardware error6", CMD_ABORT)) {
      }   // check if no warnings and errors
    }

  //===================================================================
  // MEASURE IMPEDANCE Z  (z command)
  //===================================================================
    else if(strcmp(sub0, MEAS_Z0) == 0)  // measure impedance Z
    { 

      SingleParamReadWrite_waitDone(CMD_Z, 0, 0, WRITE_MODE, 1);    // start Z measurement, don't wait too long, just ones
      // we're not waiting for DONE here!
      measureZ_counter = 0;                   // counter for sequential measurements (if count > 1)
      stateMeasureZ    = ACTIVE_Z;            // changing the state to active will trigger a chain of events to poll multiple times the Z result

    } // was Z measure impedance

  //===================================================================
  // CALIBRATE command tug (calibrate splits into multiple branches) 
  //===================================================================
    else if(strcmp(sub0, CALIBRATE0) == 0)  // calibrate main tug is here
    { 
      measureZ_counter = 0;                   // counter for sequential measurements - in all cases we need to reset it, let's do it here
      bool flagCalError = true;               // error flag to report if something went wrong

      // first task is to read the current Vgain and Igain as FREQ/GVAIN/IGAIN is the calibration "trinity" defining the CAL_INDEX

      //------------ CALIBRATE SHORT ---------------------------------------
      if (strcmp(sub1, CALIBRATE_SHORT1 ) == 0) {   // we start calibrate short
        SingleParamReadWrite_waitDone(CMD_CALIBRATE, ADDRESS_SHORT_CAL, 0, WRITE_MODE);  // SHORT calibration is initiated
        stateMeasureZ    = ACTIVE_CAL;    // changing the state to ACTIVE_CAL will trigger a chain of events to poll multiple times the CAL results
        flagCalError = false; // no error
        measureZ_counter = 0; // to display the result

      }
      //------------ CALIBRATE OPEN ---------------------------------------
      else if (strcmp(sub1, CALIBRATE_OPEN1 ) == 0) {   // we start calibrate open
        SingleParamReadWrite_waitDone(CMD_CALIBRATE, ADDRESS_OPEN_CAL, 0, WRITE_MODE);  // OPEN calibration is initiated
        stateMeasureZ    = ACTIVE_CAL;    // changing the state to ACTIVE_CAL will trigger a chain of events to poll multiple times the CAL results
        flagCalError = false; // no error
        measureZ_counter = 0; // to display the result

      }
      //------------ CALIBRATE LOAD ---------------------------------------  @@@WORK_LOAD
      else if (strcmp(sub1, CALIBRATE_RT_XT1 ) == 0) {   // detect RT        
 
        if (strcmp(sub2, VOID_STR) != 0) { // we have some data in argument 1

          float floatVal = atof(sub2);  // this is the extracted value as float from argument1 
          uint32_t val_rt = ConvFloatToInt32(floatVal);
               

          if (strcmp(sub3, CALIBRATE_XT_XT3 ) == 0) {  // here is the XT and we'll expect the argumen2 (xt value)

            if (strcmp(sub4, VOID_STR) != 0) { // we have some data in argument 2
              floatVal = atof(sub4);  // this is the extracted value as float from argument1 
              uint32_t val_xt = ConvFloatToInt32(floatVal);

              SingleParamReadWrite_waitDone(CMD_CALIBRATE, ADDRESS_LOAD_CAL_SET_RT, val_rt, WRITE_MODE);  // set RT value for load calibration

              if (IsOK_Report_Err_Warn("Call_LOAD coeff1 error", CMD_CALIBRATE)) {  // check argument 1

                SingleParamReadWrite_waitDone(CMD_CALIBRATE, ADDRESS_LOAD_CAL_SET_XT, val_xt, WRITE_MODE);  // set XT value for load calibration
                if (IsOK_Report_Err_Warn("Call_LOAD coeff2 error", CMD_CALIBRATE)) {  // check argument 1

                  SingleParamReadWrite_waitDone(CMD_CALIBRATE, ADDRESS_LOAD_CAL, 0, WRITE_MODE, 1);  // LOAD calibration is initiated - DON'T WAIT too long (1)

                  stateMeasureZ    = ACTIVE_CAL;    // changing the state to ACTIVE_CAL will trigger a chain of events to poll multiple times the CAL results
                  flagCalError = false; // no error
                  measureZ_counter = 0; // to display the result

                } // was OK the argument2            
              } // was OK the argument1    
            }  // // we had existing argument2 
          }  // we also had XT parameter
        } // there was data in argument1
      } // was detected LOAD >RT<

      //------------ CALIBRATE COMMIT ---------------------------------------
      else if (strcmp(sub1, CALIBRATE_COMMIT1 ) == 0) {   // calibrate commit command       

        if (strcmp(sub2, VOID_STR) != 0) { // we have some data in argument 1 (the password)

          if (strcmp(sub3, VOID_STR) != 0) { // we have some data in argument 2 (timestamp)       
            resultA = atoi(sub3);  // this is timestamp
          }
          else {
            resultA = 0x12345;   // no timestamp added - set it to some easy to recognize value
          } // no timestamp - but let's tollerate it

          for (int ii = 0; ii < ADDRESS_TIMESTAMP; ii++) {  // fill the password spaces
            uint8_t nextChar = sub2[ii];     // get next char into nextChar var

            if (nextChar == 0)  {            // this is the end of the password?
              break;                         // abort the loop - this is the last char in the password
            }

            SingleParamReadWrite_waitDone(CMD_CAL_COMMIT, ii, nextChar, WRITE_MODE, DEFAULT_MAX_NUMBER_WAIT);  // Fill the password locations, no reset SPI
            IsOK_Report_Err_Warn("Commit calibration password", CMD_CAL_COMMIT);
          } // fill all 12 locations with different password characters

          // there is a problem with TS functionality - it accepts only values <127 like it's password
          // SingleParamReadWrite_waitDone(CMD_CAL_COMMIT, ADDRESS_TIMESTAMP, resultA, WRITE_MODE, DEFAULT_MAX_NUMBER_WAIT, true);  // set timestamp
          // IsOK_Report_Err_Warn("Commit calibration TS", CMD_CAL_COMMIT);

          SingleParamReadWrite_waitDone(CMD_CAL_COMMIT, ADDRESS_CAL_COMMIT, 0, WRITE_MODE, DEFAULT_MAX_NUMBER_WAIT);  // trigger the password comparision and data commit

          stateMeasureZ    = ACTIVE_COMMIT_CAL;    // changing the state to ACTIVE_CAL will trigger a chain of events to poll multiple times the CAL results
          flagCalError = false; // no error

        } // we have the password
        else
        {
          Bridge_SerialPrintLn("Error : Calibrate commit password missing!");
          Bridge_SerialPrintDelimiter() ;  // end of the task
        }          
      }  // commit calibration  

      //------------ CALIBRATE LIST ---------------------------------------
      else if (strcmp(sub1, CALIBRATE_LIST1 ) == 0) {   // @@@CAL_LIST calibrate LIST command       

        if (strcmp(sub2, VOID_STR) != 0) { // we have some data in argument 1 (frequency) and we need to get all calibrations for this frequency

          // SingleParamReadWrite_waitDone(CMD_CAL_COMMIT, ADDRESS_CAL_COMMIT, 0, WRITE_MODE, DEFAULT_MAX_NUMBER_WAIT);  // trigger the password comparision and data commit

          // stateMeasureZ    = ACTIVE_COMMIT_CAL;    // changing the state to ACTIVE_CAL will trigger a chain of events to poll multiple times the CAL results

        } // we have to list all calibrations for particular frequency
        else
        { // list all frequencies with calibrations

          // stateMeasureZ    = ACTIVE_COMMIT_CAL;    // changing the state to ACTIVE_CAL will trigger a chain of events to poll multiple times the CAL results

        }  // was no freq parameter    

          flagCalError = false; // no error
          Bridge_SerialPrintLn("Error : Calibrate list command is under construction!");
          Bridge_SerialPrintDelimiter() ;  // end of the task

      }  // commit LIST  



      //------------ CALIBRATE ERASE ---------------------------------------
      else if (strcmp(sub1, CALIBRATE_ERASE1 ) == 0) {   // calibrate erase command  @@@WORK     

        if (strcmp(sub2, VOID_STR) != 0) { // we have some data in argument 1 (the password)

          for (int ii = 0; ii < ADDRESS_TIMESTAMP; ii++) {  // fill the password cells
            uint8_t nextChar = sub2[ii];     // get next char into nextChar var

            if (nextChar == 0)  {            // this is the end of the password?
              break;                         // abort the loop - this is the last char in the password
            }

            SingleParamReadWrite_waitDone(CMD_ERASE_CALIBRATION, ii, nextChar, WRITE_MODE, DEFAULT_MAX_NUMBER_WAIT);  // Fill the password locations, no reset SPI
            IsOK_Report_Err_Warn("Calibrate erase password", CMD_ERASE_CALIBRATION);
          } // fill up to 12 locations with different password characters

          SingleParamReadWrite_waitDone(CMD_ERASE_CALIBRATION, ADDRESS_CAL_ERASE, 0, WRITE_MODE, 1);  // trigger the password comparision and data commit, wait very short

          stateMeasureZ    = ACTIVE_CALIBRATE_ERASE;    // changing the state to ACTIVE_CAL will trigger a chain of events to poll multiple times the CAL results
          flagCalError = false; // no error

        } // we have the password
        else
        {
          Bridge_SerialPrintLn("Error : Calibrate erase password missing!");
          Bridge_SerialPrintDelimiter() ;  // end of the task
        }          
      }  // calibrate erase  

      //------------ CALIBRATE ON or OFF ---------------------------------------
      else if ((strcmp(sub1, CALIBRATE_ON1 ) == 0) ||  (strcmp(sub1, CALIBRATE_OFF1 ) == 0))  {   // detect calibrate ON or calibrate off        
        int calSetupVal = 0;
        if (strcmp(sub1, CALIBRATE_ON1 ) == 0) {
          calSetupVal = 1;  // we enable the calibration
        } // enable calibration

        flagCalError = false; // no error
        SingleParamReadWrite_waitDone(CMD_CORRECTION_MODE, 0, calSetupVal, WRITE_MODE);  // set the calibration on/off

        if (calSetupVal) {  // output the string
          Bridge_SerialPrintLn("Calibration is enabled");
        } 
        else {
          Bridge_SerialPrintLn("Calibration is disabled");
        } 

        Bridge_SerialPrintDelimiter() ;  // end of the task
      }   // detected calibrate on / off

      //------------ CALIBRATE RELOAD ---------------------------------------
      else if (strcmp(sub1, CALIBRATE_RELOAD1 ) == 0)  {   // detect calibrate reload        
        
        flagCalError = false; // suppress the error message
        SingleParamReadWrite_waitDone(CMD_CALIBRATE, ADDRESS_RELOAD_CAL, 0, WRITE_MODE,1);  // calibrate reload - we don't wait as it may take longer

        stateMeasureZ = ACTIVE_RELOAD_CAL;   // @@@CALIBR go into long wait mode for calibrate reload


      } // end of Calibrate reload

      //------------ NOT RECOGNIZED CAL IDENTIFIER ------------------------------
      else {  // not recognized identifier
        Bridge_SerialPrintLn("Error : Non supported cal parameter!");  // report the integer response
        Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands

      }

      if (flagCalError) { // something went wrong - show CAL ERROR message
        Bridge_SerialPrintLn("Error : Cal parameters mismatched!");  // report the integer response
      } // something was not OK 

    } // was CALIBRATE task (a tug with multiple commands)

  //===================================================================
  // RDCAL command - read calibration coefficients 
  //===================================================================
    else if(strcmp(sub0, READCAL0) == 0)  // rdcal detected?  @@@WORK2
    { 
      bool flagNoArg = true;
      int vgain = 0; 
      int igain = 0;
      if (strcmp(sub1, VOID_STR) != 0) { // we have some data in argument 1
        vgain = atoi(sub1);  // this is the extracted value as integer from the argument1 (VGAIN)

        if (strcmp(sub2, VOID_STR) != 0) { // we have some data in argument 2
          igain = atoi(sub2);  // this is the extracted value as integer from the argument2 (IGAIN)
          flagNoArg = false;     // no error report
                    
          // make test write for Ro to see if there is an error - if we get error - it's because there is nothing stored there and we need to show defaults
          SingleParamReadWrite_waitDone(CMD_CAL_READ, MASK_LSB_COEFFICIENT | ((igain & 0x03) << 2) | (vgain & 0x03), 0, READ_MODE);  // Request reading Ro coeff LSB
          
          if (flag_ERROR == false) {  // we have valid coeff in memory - it's easy to tell from Ro - if pure zero, we need to show defaults 
//--------- OUTPUT all 12 calibration coefficients
            ReadCalibrationDouble(CALL_ADDR_Ro, vgain, igain, "Ro = ");   // get Ro
            ReadCalibrationDouble(CALL_ADDR_Xo, vgain, igain, "Xo = ");   // get Xo
            ReadCalibrationDouble(CALL_ADDR_Go, vgain, igain, "Go = ");   // get Go
            ReadCalibrationDouble(CALL_ADDR_Bo, vgain, igain, "Bo = ");   // get Bo

            ReadCalibrationDouble(CALL_ADDR_Rs, vgain, igain, "Rs = ");   // get Rs
            ReadCalibrationDouble(CALL_ADDR_Xs, vgain, igain, "Xs = ");   // get Xs
            ReadCalibrationDouble(CALL_ADDR_Gs, vgain, igain, "Gs = ");   // get Gs
            ReadCalibrationDouble(CALL_ADDR_Bs, vgain, igain, "Bs = ");   // get Bs

            ReadCalibrationDouble(CALL_ADDR_Rg, vgain, igain, "Rg = ");   // get Rg
            ReadCalibrationDouble(CALL_ADDR_Xg, vgain, igain, "Xg = ");   // get Xg
            ReadCalibrationDouble(CALL_ADDR_Gg, vgain, igain, "Gg = ");   // get Gg
            ReadCalibrationDouble(CALL_ADDR_Bg, vgain, igain, "Bg = ");   // get Bg

          } // there was no error for this set of Vgain and Igain
          else
          { // both int32 for Ro were zero - we need to show defaults!
            Bridge_SerialPrintLn("Warn : No cal coefficients found for V_gain and I_gain. Defaults values:");
            Bridge_SerialPrintLn("Ro = 1.0e+06"); 
            Bridge_SerialPrintLn("Xo = 1.0e+06"); 
            Bridge_SerialPrintLn("Go = 0.0e+00"); 
            Bridge_SerialPrintLn("Bo = 0.0e+00"); 

            Bridge_SerialPrintLn("Rs = 0.0e+00"); 
            Bridge_SerialPrintLn("Xs = 0.0e+00"); 
            Bridge_SerialPrintLn("Gs = 1.0e+06"); 
            Bridge_SerialPrintLn("Bs = 1.0e+06"); 

            Bridge_SerialPrintLn("Rg = -1.0e+06"); 
            Bridge_SerialPrintLn("Xg = -1.0e+06"); 
            Bridge_SerialPrintLn("Gg = -1.0e+06"); 
            Bridge_SerialPrintLn("Bg = -1.0e+06"); 
          } // was error - no data for these Vgain/Igain - show default coefficients!

          // output calibration status - which CAL were done
          resultA = SingleParamReadWrite_waitDone(CMD_CAL_READ, (CALL_ADDR_AC_STATUS << SHIFT_ADDR_READ_CAL) | \
                                                        ((igain & 0x03) << 2) | (vgain & 0x03), 0, READ_MODE);  // Request reading Ro coeff LSB

          const char calDone[] = "done";
          const char calNotDone[] = "not_done";

          Bridge_SerialPrint("Short = ");
          if (resultA & MASK_SHORT_DONE) { Bridge_SerialPrint(calDone); }
            else {Bridge_SerialPrint(calNotDone);}         
          
          Bridge_SerialPrint(", Open = ");
          if (resultA & MASK_OPEN_DONE) { Bridge_SerialPrint(calDone); }
            else {Bridge_SerialPrint(calNotDone);} 

          Bridge_SerialPrint(", Load = ");
          if (resultA & MASK_LOAD_DONE) { Bridge_SerialPrintLn(calDone); }
            else {Bridge_SerialPrintLn(calNotDone);} 

        } // second argument was not void string
      } // we have first argumanet (VGAIN)

      if (flagNoArg)  { // missing arguments
        Bridge_SerialPrintLn("Error : rdcal missing arguments!");  // report the for missing arguments
      }

      Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands

    } // was RDCAL command
  //===================================================================
  // RESETCAL command - resets all or some calibration coefficients
  //===================================================================
    else if(strcmp(sub0, RESETCAL0) == 0)  // resetcal detected?
    { 
      bool flagResetCalErr = true;
      int mask_RESETCAL = 0;
      if (strcmp(sub1, VOID_STR) != 0) { // we have some data in argument 1
        resultA = atoi(sub1) & 0x03;  // this is V gain

        if (strcmp(sub2, VOID_STR) != 0) { // we have some data in argument 2
          resultB = atoi(sub2) & 0x03;  // this is I gain
          mask_RESETCAL = resultA | (resultB << 2); 
          flagResetCalErr = false;
          SingleParamReadWrite_waitDone(CMD_RESET_CAL, mask_RESETCAL, 0, WRITE_MODE);  // Request reading Ro coeff LSB
          Bridge_SerialPrintLn("Reset : success");
        } // we have two arguments, and we need to set the mask
      }  // argument 1 is non void
      else
      {
        flagResetCalErr = false; // there is no error, just no arguments - set mask=FF
          SingleParamReadWrite_waitDone(CMD_RESET_CAL, MASK_RESET_ALL_CAL, 0, WRITE_MODE);  // Request reading Ro coeff LSB
          Bridge_SerialPrintLn("Resetting all : success");
      } // no arguents - erase all

      if (flagResetCalErr)  { // no error - complete operation
        Bridge_SerialPrintLn("Error : resetcal missing arguments!");  // report the for missing arguments
      } // was error 

      Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands

    } // was resetcal command

  //===================================================================
  // STORECAL command - stores individual coefficients into RAM for vgain/igain/freq trinity @@@WORK3
  //===================================================================
    else if(strcmp(sub0, STORECAL0) == 0)  // STORECAL detected?
    { 
      bool flagWrongArguments = true;  // if true - we'll not proceed sending commands
      int v_gain, i_gain;
      double dfloatValue;
      uint64_t valToWrite;

      if (strcmp(sub1, VOID_STR) != 0) { // we have some data in argument 1
        v_gain = atoi(sub1) & 0x03;  // this is V gain

        if (strcmp(sub2, VOID_STR) != 0) { // we have some data in argument 2
          i_gain = atoi(sub2) & 0x03;  // this is I gain
        
          if (strcmp(sub3, VOID_STR) != 0) { // we have some data in argument 3
            int tokenVal = -1;   // negative value - no tocken
            for (int ii = 0; ii < 12; ii++) {   // scan for tockens
               
              if (strcmp(sub3, STORE_CAL_FIELDS1[ii]) == 0) { // we have tocken match
                tokenVal = ii;
                break; // break the loop
              } // token match - abort and get the index
            }  // compare with 12 tokens

            if ((tokenVal >= 0) && (strcmp(sub4, VOID_STR) != 0)) { // we have some data in argument 4 - this is the coefficient double floating value
              flagWrongArguments = false;  // suppress error message

              dfloatValue = atof(sub4);  // convert to double  
              valToWrite = ConvDoubleToInt64(dfloatValue);

              SingleParamReadWrite_waitDone(CMD_STORE_CAL, (tokenVal << SHIFT_ADDR_STORE_CAL) | MASK_LSB_COEFFICIENT | (i_gain << 2) | v_gain, \
                                            (uint32_t)(valToWrite & 0xFFFFFFFF), WRITE_MODE, DEFAULT_MAX_NUMBER_WAIT);  // Fill the password locations, no reset SPI
              IsOK_Report_Err_Warn("StoreCal failure LSB", CMD_STORE_CAL);
 
              SingleParamReadWrite_waitDone(CMD_STORE_CAL, (tokenVal << SHIFT_ADDR_STORE_CAL) | MASK_MSB_COEFFICIENT | (i_gain << 2) | v_gain, \
                                            (uint32_t)(valToWrite >> 32), WRITE_MODE, DEFAULT_MAX_NUMBER_WAIT);  // Fill the password locations, no reset SPI
              IsOK_Report_Err_Warn("StoreCal failure MSB", CMD_STORE_CAL);
              
            } // we have the coefficient value and found the token - ready to proceed
          } // name of parameter exists
        } // igain was existing
      } // vgain was existing

      if (flagWrongArguments) { // we have to repot an error
        Bridge_SerialPrintLn("Error : StoreCal invalid parameters");
      }  
      
      Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands

    } // was storcal command

  //===================================================================
  // SET VGAIN and IGAIN COMMANDs
  //===================================================================
    else if(strcmp(sub0, SETGAIN0) == 0) {  //this is setgain command

      bool flag_V_I_gain = false;
      char reportStr[20];  // keep response here

  //---------- AUTO GAIN ----------------------    
      if(strcmp(sub1, SETGAIN_AUTO1) == 0) { // check if second argument is auto ?
        SingleParamReadWrite_waitDone(CMD_ENABLE_AUTORANGE, 0, 1, WRITE_MODE);  // enable autorange 

        if (IsOK_Report_Err_Warn("Wrong arguments", CMD_ENABLE_AUTORANGE))   // check if no warnings and errors
        {
        Bridge_SerialPrintLn("Autorange enabled");  // report autorange was enabled
        Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands
        } // everthing was OK

      } // was auto gen

  //---------- WRITE VOLTAGE GAIN (stageA) ----------------------    
      else if ((strcmp(sub1, SETGAIN_VGAIN1) == 0) && \
              (strcmp(sub2, VOID_STR) != 0)) {  // setup VGAIN values - extract from sub2 (which is not void)
        grCommand = CMD_VOLTAGE_GAIN;
        strcpy(reportStr, SETGAIN_RESP_V1);
        flag_V_I_gain = true;    // Vgain is needed      
      } // was vgain

  //---------- WRITE VOLTAGE GAIN (stageA)----------------------    
      else if ((strcmp(sub1, SETGAIN_IGAIN1) == 0) && \
              (strcmp(sub2, VOID_STR) != 0)) {  // setup VGAIN values - extract from sub2 (which is not void)
        grCommand = CMD_CURRENT_GAIN;
        strcpy(reportStr, SETGAIN_RESP_I1);
        flag_V_I_gain = true;    // Igain is needed      
      } // was igain

  //---------- READ GAIN SETTINGS ----------------------    
      else if(strcmp(sub1, VOID_STR) == 0) // read gain command
      {      
  //--------- READ SETGAIN setttings
        resultA = SingleParamReadWrite_waitDone(CMD_VOLTAGE_GAIN     | CMND_READ_MASK, 0, 0, READ_MODE);
        resultB = SingleParamReadWrite_waitDone(CMD_CURRENT_GAIN     | CMND_READ_MASK, 0, 0, READ_MODE);
        resultC = SingleParamReadWrite_waitDone(CMD_ENABLE_AUTORANGE | CMND_READ_MASK, 0, 0, READ_MODE);

        if (resultC & 0x01) { // AUTORANGE DETECTED
          Bridge_SerialPrintLn("Autorange enabled"); } // report autorange was enabled
        else {
           Bridge_SerialPrintLn("Autorange disabled"); } // report autorange was disabled

        Bridge_SerialPrint(SETGAIN_RESP_V1);  // report autorange was enabled
        Bridge_SerialPrintLn(String(resultA));
        Bridge_SerialPrint(SETGAIN_RESP_I1);  // report autorange was enabled
        Bridge_SerialPrintLn(String(resultB));
        Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands

      } // was read gain command

      if (flag_V_I_gain)   // check is we already detected vgain or igain for writing settings
      {
  //--------- WRITE SETGAIN setttings (which disables the autogain)
        resultA = atoi(sub2);  // this is the extracted value of gain as integer from argument2 
        SingleParamReadWrite_waitDone(grCommand, 0, resultA, WRITE_MODE); // write the value
        if (IsOK_Report_Err_Warn("SetGain", grCommand))   // check if no warnings and errors
        {
          SingleParamReadWrite_waitDone(CMD_ENABLE_AUTORANGE, 0, 0, WRITE_MODE); // disable autorange
          if (IsOK_Report_Err_Warn("Autorange update", grCommand))   // check if no warnings and errors
          {
            Bridge_SerialPrint(reportStr);     // the command name
            Bridge_SerialPrintLn(String(resultA));  // report the integer response

            Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands
          } // was OK disable autorange
        } // was OK when setting the range
      }  // we needed some extra work on V/I gains      
    }  // was setgain command

  //===========================================================================
  //---------- CAN'T FIND COMMND - THIS IS NOT SUPPORTED ----------------------    
  //===========================================================================
  else  {  //can't find command
      Bridge_SerialPrintLn("Error : Non supported command!");  // report the integer response
      Bridge_SerialPrintDelimiter();   // here we print the special character 0x0C which works as LabView delimiter for the commands 
    } // command not recognized
  } // non group processing commands
} // end of the command processor


