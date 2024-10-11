//================================================================
// ADMX2001B USB to SPI bridge
// Calibration supporting commands
// IDEX Biometrics UK
// Written by Luben Hristov
//
// 12-08-24 -- Creating the file with ANSI command definitions
//
//================================================================
#ifndef _CALIBRATE_SUPP_H
#define _CALIBRATE_SUPP_H
#include <Arduino.h>     // when we use definitions like byte, etc. we need to include it

double   ReadCalibrationDouble(int addrVal, int V_gain, int I_gain, const char custString[]);   // we read double data for Vgain/Igain
float    ReadCalibrationFloat( int addrVal, int V_gain, int I_gain, const char custString[]);   // we read float data for Vgain/Igain
uint32_t ReadCalibrationUInt32(int addrVal, int V_gain, int I_gain, const char custString[]);   // we read integer32 data for Vgain/Igain

#endif  // end _CALIBRATE_SUPP_H