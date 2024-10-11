//================================================================
// ADMX2001B USB to SPI bridge for Arduino UNO R4 Minima
// Executing tasks made on slower pace (like wait for DONE)
// IDEX Biometrics UK
// Written by Luben Hristov
//
// Arduino UNO R4 Minima for attachInterrupt() useable pins are: 2, 3 like:  attachInterrupt(digitalPinToInterrupt(pin_size_t pin)....)
// 07-08-24 -- Starting the project, testing the serial, setting DTR to receive data
//
//================================================================
#include "SlowTask.h"                   // inlcude Slow task header
#include "CmndProcess.h"                // inlcude functionality from command processor
#include "SPI_cmnd.h"                   // SPI commands definitions
#include "LIF.h"                        // inlcude debugger

#define DONE_POLLING_TIME_MS    5       // prevents to often calling of the task, 5..20ms is a good balance between performace and responsivness 
unsigned long lastPollTime = millis();  // here we keep track of the last moment we polled the status and if it exceeds the limit - poll it again 

//================================================================
// Extract one record from FIFO and report it over serial
//================================================================
void ReportZ_fromFIFO(void)
{
uint32_t resultFIFO_0, resultFIFO_1;  // we accumulate the reading here
uint64_t mergedVal64;                 // here we collect the data for conversion U64->double
double_t Rm, Xm;                      // this is the measured impedance
char floatBuffer[SIZE_SUB_ARRAY]; // here we store the float shar

        //--------- Pull one Z record from the FIFO (four 32bit words)                          
  resultFIFO_0 = Single_ADMX_Frame(CMD_FIFO_READ, 0, 0);  // read the data, before resultFIFO_0 = Single_ADMX_Frame(CMD_FIFO_READ, 0, 0);
  resultFIFO_1 = Single_ADMX_Frame(CMD_FIFO_READ, 0, 0);  // read the data
  mergedVal64 = (uint64_t)(resultFIFO_1)<<32 | resultFIFO_0;  // merge the two U32 words into U64
  Rm = ConvInt64ToDouble( mergedVal64);    // this is the first result as double

  Bridge_SerialPrint(String(measureZ_counter));
  Bridge_SerialPrint(","); // delimiter

  sprintf(floatBuffer, "%.7e", Rm); // here we store the float shar
  Bridge_SerialPrint(floatBuffer);  // Output real

  Bridge_SerialPrint(","); // delimiter

  resultFIFO_0 = Single_ADMX_Frame(CMD_FIFO_READ, 0, 0);  // read the data, before: resultFIFO_0 = Single_ADMX_Frame(CMD_FIFO_READ, 0, 0);
  resultFIFO_1 = Single_ADMX_Frame(CMD_FIFO_READ, 0, 0);  // read the data
  mergedVal64 = (uint64_t)(resultFIFO_1)<<32 | resultFIFO_0;  // merge the two U32 words into U64
  Xm = ConvInt64ToDouble( mergedVal64);    // this is the Second result as double

  sprintf(floatBuffer, "%.7e", Xm); // here we store the float shar
  Bridge_SerialPrintLn(floatBuffer);  // Output real

  measureZ_counter++;  // ready for the next sample

} // end of eportZ_fromFIFO

//================================================================
// Extract one record from FIFO and report it over serial
//================================================================
void Flush_FIFO(void) // flush all data from FIFO  
{
  for (int ii = 0; ii < 256; ii++)  // avoid hanging here - limit the max number of fifo records to 256 (if by some reason we get huge number)
  {
    WaitForDoneAndGetStatus(1);  // update the status of depthFIFO

    if (depthFIFO == 0) {  // in case the FIFO depth is zero - we abort the loop
      break;
    } 

    Single_ADMX_Frame(CMD_FIFO_READ, 0, 0);  // we poll the data from the FIFO only when there is nothing inside
  }

} // end of FLUSH_FIFO

//================================================================
// Main processing of commands - we send SPI commands according to the ANSI commands strings in sub0..sub5
//================================================================
void ExecuteSlowTask(void)   // here we execute commands on regular intervals like wait for DONE and wait for MEASURE_DONE
{

unsigned long elapsedTimeMS;  // here we calculate the difference
char reportStr[SIZE_SUB_ARRAY] = "";

double floatResult;    // other floating temp 

unsigned long currMillisecond = millis();

  if (currMillisecond >= lastPollTime) { // normal operation - timer has no rollower 
    elapsedTimeMS = currMillisecond - lastPollTime;  // calc elapsed time in ms
  }
  else {  // we hit rollover - it happens rarely, after 50 days and is recognizable because lastPollTime becomes greater than millis()
    elapsedTimeMS = (LARGEST_UNIGNED_LONG - lastPollTime) + currMillisecond; // compensation of the elapsed time
  }
  
  if (elapsedTimeMS >= DONE_POLLING_TIME_MS) // time to make new polling for DONE?
  {
    lastPollTime = currMillisecond;   // start new measurement 

//---------------- HERE WE PASS EACH 5 ms -------------------------------------------------
//=========================================================================================

    if (stateMeasureZ == ACTIVE_Z) {  // there is ACTIVE_Z measurement tak running, let's do some work
      // time to see if we can poll out some data   
      CheckStatus_and_Warnings();  // check the status and set the flags 

//      SingleParamReadWrite_waitDone(CMD_STATUS_READ, 0, 0, READ_MODE, 1, true); // read once the status register to see the FIFO length
      IsOK_Report_Err_Warn("Z measure", CMD_Z);

      if (depthFIFO > 0) // there is some data
      {
        if (depthFIFO >= 4) // there is data to poll out and it has 4*N pending values we can take in one shot
        {
          ReportZ_fromFIFO();  // reports real/imaginary and count
        } // we had at least 4 records and we polled one data 
        // Flush_FIFO(); // flush all data from FIFO  

      }   // the current depth is > 0 

      //--------- If flag MEASURE_DONE was set - we can simply end the task
      else if (flag_DONE) { // it's ACTIVE_Z, no Z pending commands and MEASURE DONE  - move the state to IDLE and release the task for new processing
                            // flag_MEASURE_DONE should not be checked here, as this may result in skipping the error/warning messages
 
        stateMeasureZ   = IDLE;    // set the measuring state to IDLE
        Bridge_SerialPrintDelimiter() ;  // at the end of the task we pint a delimiter to extract the data from the PC FIFO
      } // we just hit the end of the ACTIVE_Z task! Status forced to IDLE

    }  // was active ACTIVE_Z measuring task
//------------------------------------------------------
//  CALIBRATION IS ACTIVE    
//------------------------------------------------------
    else if (stateMeasureZ == ACTIVE_CAL) {  // there is ACTIVE_CAL task running, let's do some work
      CheckStatus_and_Warnings();  // check the status and set the flags 

//      SingleParamReadWrite_waitDone(CMD_STATUS_READ, 0, 0, READ_MODE, 1); // read ONCE the status register to see the FIFO length
      IsOK_Report_Err_Warn("Calibrate", CMD_CALIBRATE);  // run calibrate command
      
      if (depthFIFO > 0) // there is some data
      {
        if (depthFIFO >= 4) // there is data to poll out and it has 4*N pending values we can take in one shot
        {
          ReportZ_fromFIFO();  // reports real/imaginary and count
        } // we had at least 4 records and we polled one data 
        
      }   // the current depth is > 0  - prefent the task from checking DONE - justr poll the data out
      else if (flag_DONE)      // it's ACTIVE_CAL and we just got DONE flag to move to next stage
      {                        // flag_MEASURE_DONE should not be checked here, as this may result in skipping the error/warning messages
        // report from CAL command (as in ANSI terminal interface)

        //---- FREQUENCY report
        uint32_t resultTemp = SingleParamReadWrite_waitDone(CMD_FREQUENCY | CMND_READ_MASK, 0, 0, READ_MODE); // read parameter 
        IsOK_Report_Err_Warn("Hardware Error21", CMD_FREQUENCY | CMND_READ_MASK) ;  // check if no warnings and errors

        Bridge_SerialPrint("Cal Freq = ");    
        floatResult = ConvInt32ToFloat(resultTemp) / 1000;            // convert the result straight into single precision floating  and divide by 1000 (kHz)
        Bridge_SerialPrint(String(floatResult, 4));                     // report the response as floating point
        Bridge_SerialPrintLn("kHz");                                    // add the pos string at the end

        //---- TIME report
        Bridge_SerialPrintLn("Cal Time: 0");

        //---- TEMPERATURE report        
        resultTemp = SingleParamReadWrite_waitDone(CMD_TEMPERATURE | CMND_READ_MASK, 0, 0, READ_MODE); // read temperature 
        if (IsOK_Report_Err_Warn("Hardware error1", CMD_TEMPERATURE | CMND_READ_MASK))   // check if no warnings and errors
        { 
          Bridge_SerialPrint("Cal Temp: ");                   // the cal temperature is
          floatResult = ConvInt32ToFloat(resultTemp);         // convert the result straight into single precision floating  and divide by 1000 (kHz)
          Bridge_SerialPrintLn(String(floatResult, 1));         // report the response as floating point with single precision
        }


        // output calibration status - which CAL were done
        uint32_t current_V_GAIN = SingleParamReadWrite_waitDone(CMD_VOLTAGE_GAIN     | CMND_READ_MASK, 0, 0, READ_MODE) & 0x03;  // store the V gain
        uint32_t current_I_GAIN = SingleParamReadWrite_waitDone(CMD_CURRENT_GAIN     | CMND_READ_MASK, 0, 0, READ_MODE) & 0x03;  // store the I gain
                                                                          //@@@WORK_AND_FIX - shift is 9 but needs 10
        resultTemp = SingleParamReadWrite_waitDone(CMD_CAL_READ, (CALL_ADDR_AC_STATUS << SHIFT_ADDR_READ_CAL) | \
                                                      ((current_I_GAIN & 0x03) << 2) | (current_V_GAIN & 0x03), 0, READ_MODE);  // Request reading Ro coeff LSB

        const char calDone[] = "Done";
        const char calNotDone[] = "Not Done";

        Bridge_SerialPrint("open: ");
        if (resultTemp & MASK_OPEN_DONE) { Bridge_SerialPrintLn(calDone); }
          else {Bridge_SerialPrintLn(calNotDone);} 

        Bridge_SerialPrint("short: ");
        if (resultTemp & MASK_SHORT_DONE) { Bridge_SerialPrintLn(calDone); }
          else {Bridge_SerialPrintLn(calNotDone);}         
        
        Bridge_SerialPrint("load: ");
        if (resultTemp & MASK_LOAD_DONE) { Bridge_SerialPrintLn(calDone); }
          else {Bridge_SerialPrintLn(calNotDone);} 

        stateMeasureZ   = IDLE;    // set the measuring state to IDLE
        Bridge_SerialPrintDelimiter() ;  // at the end of the task we pint a delimiter to extract the data from the PC FIFO
      } // we just hit the end of the ACTIVE_Z task! Status forced to IDLE

    } // was active ACTIVE_CAL calibration task
//------------------------------------------------------
//  COMMIT CALIBRATION (storing coefficients in FLASH) IS ACTIVE    
//------------------------------------------------------
    else if (stateMeasureZ == ACTIVE_COMMIT_CAL) {  // there is ACTIVE_COMMIT task running, let's do some work
      CheckStatus_and_Warnings();  // check the status and set the flags 
      IsOK_Report_Err_Warn("Commit calibration coeff", CMD_CAL_COMMIT);  // running commit command
      
      if (flag_DONE) { // it's ACTIVE_CAL and we just got DONE flag to move to next stage
        if (flag_ERROR == false) {
          Bridge_SerialPrintLn("Commit : success");  
        } // no error
        else {
          Bridge_SerialPrintLn("Error : Calibration not done");  
        } // no error

        stateMeasureZ   = IDLE;    // set the measuring state to IDLE
        Bridge_SerialPrintDelimiter() ;  // at the end of the task we pint a delimiter to extract the data from the PC FIFO

      }  // done was OK

    } // was active ACTIVE_COMMIT task

//------------------------------------------------------
//  CALIBRATION ERASE - delete all calibrations from memory
//------------------------------------------------------
    else if (stateMeasureZ == ACTIVE_CALIBRATE_ERASE) {  // there is ACTIVE_COMMIT task running, let's do some work
      CheckStatus_and_Warnings();  // check the status and set the flags 
      IsOK_Report_Err_Warn("Calibrate erase", CMD_ERASE_CALIBRATION);  // running calibrate erase
      
      if (flag_DONE) { // it's ACTIVE_CALIBRATE_ERASE and we just got DONE flag to move to next stage
        if (flag_ERROR == false) {
          Bridge_SerialPrintLn("Erase : success");  
        } // no error
        else {
          Bridge_SerialPrintLn("Error : Calibrate Erase not done");  
        } // no error

        stateMeasureZ   = IDLE;    // set the measuring state to IDLE
        Bridge_SerialPrintDelimiter() ;  // at the end of the task we pint a delimiter to extract the data from the PC FIFO

      }  // done was OK

    } // was active ACTIVE_COMMIT task
//------------------------------------------------------
//  CALIBRATION RELOAD - reload calibration before each Z 
//------------------------------------------------------
    else if (stateMeasureZ == ACTIVE_RELOAD_CAL) {  // there is ACTIVE_RELOAD_CAL task running, let's do some work
      CheckStatus_and_Warnings();  // check the status and set the flags 
      IsOK_Report_Err_Warn("Calibrate reload", CMD_CALIBRATE);  // running calibrate reload
      
      if (flag_DONE) { // it's ACTIVE_CALIBRATE_RELOAD and we just got DONE flag to move to next stage
        if (flag_ERROR == false) {
          Bridge_SerialPrintLn("Reload : success");  // reloading was successful
        } // no error
        else {
          Bridge_SerialPrintLn("Error : Calibrate reloading not done");  // error message 
        } // no error

        stateMeasureZ   = IDLE;    // set the measuring state to IDLE
        Bridge_SerialPrintDelimiter() ;  // at the end of the task we pint a delimiter to extract the data from the PC FIFO

      }  // done was OK

    } // was active ACTIVE_RELOAD_CAL task

  }  // was repetitive task on 5ms (time for 5ms expired)    

} // End of the slow task