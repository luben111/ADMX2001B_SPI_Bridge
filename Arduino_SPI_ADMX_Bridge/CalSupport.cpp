//================================================================
// ADMX2001B USB to SPI bridge
// Processing read/write CALIBRATION commands
// IDEX Biometrics UK
// Written by Luben Hristov
//
// 12-08-24 -- Starting the impelementation
//================================================================
#include <Arduino.h>
#include <stdio.h>           // print commands and other stuff
#include "SPI_cmnd.h"        // SPI commands definitions
#include "ANSI_cmnd.h"      // ANSI commands definitions
#include "CmndProcess.h"     // get some definitions from there
#include "SlowTask.h"

//================================================================
// Read calibration coefficients (double precision) for Vgain/Igain
//================================================================
double ReadCalibrationDouble(int addrVal, int V_gain, int I_gain, const char custString[])   // we read the data for Vgain/Igain
{
  uint32_t resultLSB, resultMSB;    // results as int32
  uint64_t resultINT64;             // 64bit value
  double val_dbl;                   // here we put the extracted coefficient
  char floatBuffer[SIZE_SUB_ARRAY]; // here we store the float shar

  resultLSB = SingleParamReadWrite_waitDone(CMD_CAL_READ, MASK_LSB_COEFFICIENT | (addrVal << SHIFT_ADDR_READ_CAL) | \
                                                        ((I_gain & 0x03) << 2) | (V_gain & 0x03), 0, READ_MODE);  // Request reading Ro coeff LSB

  resultMSB = SingleParamReadWrite_waitDone(CMD_CAL_READ, MASK_MSB_COEFFICIENT | (addrVal << SHIFT_ADDR_READ_CAL) | \
                                                        ((I_gain & 0x03) << 2) | (V_gain & 0x03), 0, READ_MODE);  // Request reading Ro coeff MSB

  resultINT64 = ((uint64_t)resultMSB << 32) | resultLSB;
  val_dbl = ConvInt64ToDouble(resultINT64);  // conversion to double

  Bridge_SerialPrint(custString); // this is the coefficient or other 
  sprintf(floatBuffer, "%.7e", val_dbl);
  Bridge_SerialPrintLn(floatBuffer);  // Output coeff

  return val_dbl;

} // end of ReadCalibrationDouble(void)


//================================================================
// Read calibration freq/temp (floating) for Vgain/Igain
//================================================================
float ReadCalibrationFloat(int addrVal, int V_gain, int I_gain, const char custString[])   // we read the data for Vgain/Igain
{
  uint32_t resultINT32;    // results as int32
  float val_float;              // here we put the extracted coefficient
  char floatBuffer[SIZE_SUB_ARRAY]; // here we store the float shar

  resultINT32 = SingleParamReadWrite_waitDone(CMD_CAL_READ, MASK_LSB_COEFFICIENT | (addrVal << SHIFT_ADDR_READ_CAL) | \
                                                        ((I_gain & 0x03) << 2) | (V_gain & 0x03), 0, READ_MODE);  // Request reading Ro coeff LSB
  val_float = ConvInt32ToFloat(resultINT32);  // conversion to double

  Bridge_SerialPrint(custString); // this is the coefficient or other 

  sprintf(floatBuffer, "%.7e", val_float);
  Bridge_SerialPrintLn(floatBuffer);  // Output coeff
 
  return val_float;

} // end of ReadCalibrationFloat(void)

//================================================================
// Read calibration status (UINT32) for Vgain/Igain
//================================================================
uint32_t ReadCalibrationUInt32(int addrVal, int V_gain, int I_gain, const char custString[])   // we read the data for Vgain/Igain
{
  uint32_t resultINT32;    // results as int32

  resultINT32 = SingleParamReadWrite_waitDone(CMD_CAL_READ, MASK_LSB_COEFFICIENT | (addrVal << SHIFT_ADDR_READ_CAL) | \
                                                        ((I_gain & 0x03) << 2) | (V_gain & 0x03), 0, READ_MODE);  // Request reading Ro coeff LSB
  Bridge_SerialPrint(custString); // this is the coefficient or other 
  Bridge_SerialPrintLn(String(resultINT32));  // Output coeff
 
  return resultINT32;

} // end of ReadCalibrationUInt32(void)



