/*
---------------------------------------
Useful tips and tricks and other info, scratchpad
---------------------------------------

1. Install printHelpers library

The sci() function converts the double value passed in val
char * sci(double value, uint8_t decimals);  //  em == 1
//#include "printHelpers.h"               // routines to print double with scientific notation - use command sci()

get Gains and autogain info
uint32_t current_V_GAIN = SingleParamReadWrite_waitDone(CMD_VOLTAGE_GAIN     | CMND_READ_MASK, 0, 0, READ_MODE) & 0x03;  // store the V gain
uint32_t current_I_GAIN = SingleParamReadWrite_waitDone(CMD_CURRENT_GAIN     | CMND_READ_MASK, 0, 0, READ_MODE) & 0x03;  // store the I gain

Bridge_SerialPrintLn("AutoRange = disabled");  // avoid Warn: as in original as this will trigger reporting the warning 
//----- voltage and curr gains
Bridge_SerialPrint("voltGain = ");
Bridge_SerialPrintLn(String(current_V_GAIN));

Bridge_SerialPrint("currGain = ");
Bridge_SerialPrintLn(String(current_I_GAIN));

















*/